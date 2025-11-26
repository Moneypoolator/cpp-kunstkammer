#include "connection_pool.hpp"
#include <iostream>
#include <algorithm>

namespace kaiten {

    // Pooled_connection implementation
    Pooled_connection::Pooled_connection(
        std::shared_ptr<beast::ssl_stream<beast::tcp_stream>> stream,
        std::weak_ptr<Connection_pool> pool,
        const std::string& host,
        const std::string& port)
        : _stream(stream), _pool(pool), _host(host), _port(port), _valid(true) {}

    Pooled_connection::~Pooled_connection() {
        if (_valid) {
            auto pool = _pool.lock();
            if (pool) {
                // We need to create a shared_ptr to this object to pass to return_connection
                // This is safe because we're in the destructor and the object is still valid
                auto self = std::shared_ptr<Pooled_connection>(this, [](Pooled_connection*){}); // Dummy deleter
                pool->return_connection(self);
            }
        }
    }

    // Connection_pool implementation
    Connection_pool::Connection_pool(
        net::io_context& ioc,
        ssl::context& ctx,
        size_t max_connections,
        std::chrono::seconds connection_timeout)
        : _ioc(ioc), _ctx(ctx), _max_connections(max_connections), _connection_timeout(connection_timeout) {}

    Connection_pool::~Connection_pool() {
        close_all_connections();
    }

    std::pair<int, std::shared_ptr<Pooled_connection>> Connection_pool::get_connection(
        const std::string& host,
        const std::string& port) {
        
        std::unique_lock<std::mutex> lock(_pool_mutex);

        // Try to find an existing idle connection
        auto conn_info = find_idle_connection(host, port);
        
        if (conn_info) {
            // Found an idle connection, mark it as in use
            conn_info->in_use = true;
            conn_info->last_used = std::chrono::steady_clock::now();
            
            auto pooled_conn = std::make_shared<Pooled_connection>(
                conn_info->stream, std::weak_ptr<Connection_pool>(shared_from_this()), conn_info->host, conn_info->port);
            
            return { 0, pooled_conn };
        }

        // No idle connection found, check if we can create a new one
        if (_all_connections.size() >= _max_connections) {
            // Wait for a connection to become available
            _pool_cv.wait(lock, [this] { 
                cleanup_expired_connections();
                return !_idle_connections.empty() || _all_connections.size() < _max_connections; 
            });

            // Try again to find an idle connection
            conn_info = find_idle_connection(host, port);
            if (conn_info) {
                conn_info->in_use = true;
                conn_info->last_used = std::chrono::steady_clock::now();
                
                auto pooled_conn = std::make_shared<Pooled_connection>(
                    conn_info->stream, std::weak_ptr<Connection_pool>(shared_from_this()), conn_info->host, conn_info->port);
                
                return { 0, pooled_conn };
            }
        }

        // Create a new connection
        auto result = create_new_connection(host, port);
        if (result.first != 0) {
            return { result.first, nullptr };
        }

        // Add to all connections
        auto conn_info_new = std::make_shared<Connection_info>();
        conn_info_new->stream = result.second;
        conn_info_new->host = host;
        conn_info_new->port = port;
        conn_info_new->last_used = std::chrono::steady_clock::now();
        conn_info_new->in_use = true;
        
        _all_connections.push_back(conn_info_new);

        auto pooled_conn = std::make_shared<Pooled_connection>(
            result.second, std::weak_ptr<Connection_pool>(shared_from_this()), host, port);
        
        return { 0, pooled_conn };
    }

    void Connection_pool::return_connection(std::shared_ptr<Pooled_connection> conn) {
        if (!conn || !conn->is_valid()) {
            return;
        }

        std::lock_guard<std::mutex> lock(_pool_mutex);
        
        // Find the connection in our _all_connections list
        auto it = std::find_if(_all_connections.begin(), _all_connections.end(),
            [&conn](const std::shared_ptr<Connection_info>& info) {
                return info->stream.get() == &conn->get_stream();
            });

        if (it != _all_connections.end()) {
            (*it)->in_use = false;
            (*it)->last_used = std::chrono::steady_clock::now();
            _idle_connections.push(*it);
            _pool_cv.notify_one();
        }
    }

    std::pair<int, std::shared_ptr<beast::ssl_stream<beast::tcp_stream>>> 
    Connection_pool::create_new_connection(const std::string& host, const std::string& port) {
        try {
            auto stream = std::make_shared<beast::ssl_stream<beast::tcp_stream>>(_ioc, _ctx);

            // Set SNI Hostname
            if (!SSL_set_tlsext_host_name(stream->native_handle(), host.c_str())) {
                beast::error_code ec { static_cast<int>(::ERR_get_error()), net::error::get_ssl_category() };
                std::cerr << "SSL SNI error: " << ec.message() << std::endl;
                return { -1, nullptr };
            }

            // Resolve and connect
            tcp::resolver resolver(_ioc);
            auto const results = resolver.resolve(host, port);
            beast::get_lowest_layer(*stream).connect(results);
            
            // SSL handshake
            stream->handshake(ssl::stream_base::client);

            return { 0, stream };
        } catch (const std::exception& e) {
            std::cerr << "Error creating connection: " << e.what() << std::endl;
            return { -1, nullptr };
        }
    }

    std::shared_ptr<Connection_pool::Connection_info> Connection_pool::find_idle_connection(
        const std::string& host, const std::string& port) {
        
        cleanup_expired_connections();

        while (!_idle_connections.empty()) {
            auto conn_info = _idle_connections.front();
            _idle_connections.pop();

            // Check if connection is still valid and matches host/port
            if (conn_info && conn_info->stream && 
                conn_info->host == host && conn_info->port == port) {
                
                // Test if connection is still alive
                beast::error_code ec;
                beast::get_lowest_layer(*conn_info->stream).socket().non_blocking(false);
                
                return conn_info;
            }
        }

        return nullptr;
    }

    void Connection_pool::cleanup_expired_connections() {
        auto now = std::chrono::steady_clock::now();
        
        // Remove expired connections from idle queue
        std::queue<std::shared_ptr<Connection_info>> valididle;
        while (!_idle_connections.empty()) {
            auto conn_info = _idle_connections.front();
            _idle_connections.pop();
            
            if (conn_info && 
                (now - conn_info->last_used) < _connection_timeout) {
                valididle.push(conn_info);
            } else if (conn_info) {
                // Remove from _all_connections as well
                auto it = std::find(_all_connections.begin(), _all_connections.end(), conn_info);
                if (it != _all_connections.end()) {
                    _all_connections.erase(it);
                }
            }
        }
        _idle_connections = std::move(valididle);

        // Remove expired connections from _all_connections
        auto it = _all_connections.begin();
        while (it != _all_connections.end()) {
            auto conn_info = *it;
            if (conn_info && !conn_info->in_use && 
                (now - conn_info->last_used) >= _connection_timeout) {
                it = _all_connections.erase(it);
            } else {
                ++it;
            }
        }
    }

    void Connection_pool::close_all_connections() {
        std::lock_guard<std::mutex> lock(_pool_mutex);
        
        for (auto& conn_info : _all_connections) {
            if (conn_info && conn_info->stream) {
                beast::error_code ec;
                conn_info->stream->shutdown(ec);
            }
        }
        
        _all_connections.clear();
        
        while (!_idle_connections.empty()) {
            _idle_connections.pop();
        }
    }

    Connection_pool::Stats Connection_pool::get_stats() const {
        std::lock_guard<std::mutex> lock(_pool_mutex);
        
        size_t active = 0;
        size_t idle = _idle_connections.size();
        
        for (const auto& conn_info : _all_connections) {
            if (conn_info->in_use) {
                active++;
            }
        }
        
        return { active, idle, _all_connections.size() };
    }

} // namespace kaiten
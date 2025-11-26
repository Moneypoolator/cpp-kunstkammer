#ifndef CONNECTION_POOL_HPP
#define CONNECTION_POOL_HPP

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <string>

namespace net = boost::asio;
namespace beast = boost::beast;
//namespace http = beast::http;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

namespace kaiten {

    // Forward declaration
    class Connection_pool;

    // Pooled connection wrapper
    class Pooled_connection : public std::enable_shared_from_this<Pooled_connection> {
    public:
        Pooled_connection(
            std::shared_ptr<beast::ssl_stream<beast::tcp_stream>> stream,
            std::weak_ptr<Connection_pool> pool,
            const std::string& host,
            const std::string& port);
        
        ~Pooled_connection();

        // Delete copy constructor and assignment operator
        Pooled_connection(const Pooled_connection&) = delete;
        Pooled_connection& operator=(const Pooled_connection&) = delete;

        // Move constructor and assignment operator
        Pooled_connection(Pooled_connection&&) = default;
        Pooled_connection& operator=(Pooled_connection&&) = default;

        beast::ssl_stream<beast::tcp_stream>& get_stream() { return *_stream; }
        const std::string& get_host() const { return _host; }
        const std::string& get_port() const { return _port; }
        bool is_valid() const { return _valid; }

        void invalidate() { _valid = false; }

    private:
        std::shared_ptr<beast::ssl_stream<beast::tcp_stream>> _stream;
        std::weak_ptr<Connection_pool> _pool;
        std::string _host;
        std::string _port;
        bool _valid;
    };

    // Connection pool class
    class Connection_pool : public std::enable_shared_from_this<Connection_pool> {
    public:
        Connection_pool(
            net::io_context& ioc,
            ssl::context& ctx,
            size_t max_connections = 10,
            std::chrono::seconds connection_timeout = std::chrono::seconds(300)); // 5 minutes

        ~Connection_pool();

        // Get a connection from the pool
        std::pair<int, std::shared_ptr<Pooled_connection>> get_connection(
            const std::string& host,
            const std::string& port);

        // Return a connection to the pool
        void return_connection(std::shared_ptr<Pooled_connection> conn);

        // Close all connections
        void close_all_connections();

        // Get pool statistics
        struct Stats {
            size_t active_connections;
            size_t idle_connections;
            size_t total_connections;
        };
        Stats get_stats() const;

    private:
        // Connection info structure
        struct Connection_info {
            std::shared_ptr<beast::ssl_stream<beast::tcp_stream>> stream;
            std::string host;
            std::string port;
            std::chrono::steady_clock::time_point last_used;
            bool in_use;
        };

        // Create a new connection
        std::pair<int, std::shared_ptr<beast::ssl_stream<beast::tcp_stream>>> 
        create_new_connection(const std::string& host, const std::string& port);

        // Find an existing idle connection
        std::shared_ptr<Connection_info> find_idle_connection(const std::string& host, const std::string& port);

        // Clean up expired connections
        void cleanup_expired_connections();

        // Member variables
        net::io_context& _ioc;
        ssl::context& _ctx;
        size_t _max_connections;
        std::chrono::seconds _connection_timeout;

        std::queue<std::shared_ptr<Connection_info>> _idle_connections;
        std::vector<std::shared_ptr<Connection_info>> _all_connections;
        mutable std::mutex _pool_mutex;
        std::condition_variable _pool_cv;
    };

} // namespace kaiten

#endif // CONNECTION_POOL_HPP
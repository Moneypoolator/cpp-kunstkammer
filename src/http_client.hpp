#ifndef HTTP_CLIENT_HPP
#define HTTP_CLIENT_HPP

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <cstdlib>
//#include <iostream>
#include <string>
#include <utility>
#include <memory>

#include "rate_limiter.hpp"
#include "connection_pool.hpp"

namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

class Http_client {
public:
    Http_client(net::io_context& ioc, ssl::context& ctx,
        kaiten::Rate_limiter* rate_limiter = &kaiten::global_rate_limiter);

    // Returns pair<status_code, response_body>
    std::pair<int, std::string> post(const std::string& host, const std::string& port, const std::string& target, const std::string& body, const std::string& token);
    std::pair<int, std::string> get(const std::string& host, const std::string& port, const std::string& target, const std::string& token);
    std::pair<int, std::string> patch(const std::string& host, const std::string& port, const std::string& target, const std::string& body, const std::string& token);

    // Установить кастомный rate limiter
    void set_rate_limiter(kaiten::Rate_limiter* limiter) {
        _rate_limiter = limiter;
    }

    // Включить/выключить rate limiting
    void set_rate_limiting_enabled(bool enabled) {
        if (_rate_limiter != nullptr) {
            _rate_limiter->set_enabled(enabled);
        }
    }

    // Get connection pool statistics
    kaiten::Connection_pool::Stats get_pool_stats() const {
        return _connection_pool->get_stats();
    }

private:
    void apply_rate_limiting() {
        if (_rate_limiter != nullptr) {
            _rate_limiter->wait_if_needed();
        }
    }

    net::io_context& _ioc;
    ssl::context& _ctx;
    kaiten::Rate_limiter* _rate_limiter;
    std::shared_ptr<kaiten::Connection_pool> _connection_pool;
};

#endif // HTTP_CLIENT_HPP
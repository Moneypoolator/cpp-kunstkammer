#ifndef HTTP_CLIENT_HPP
#define HTTP_CLIENT_HPP

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <string>
#include <utility>
#include <glog/logging.h>

#include "rate_limiter.hpp"
#include "error.hpp"

namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

struct HttpResponse {
    int status_code;
    std::string body;
    
    HttpResponse(int code, std::string resp_body) 
        : status_code(code), body(std::move(resp_body)) {}
};

class Http_client {
public:
    Http_client(net::io_context& ioc, ssl::context& ctx, 
        kaiten::Rate_limiter* rate_limiter = &kaiten::global_rate_limiter);

    // Returns Result with HttpResponse
    kaiten::Result<HttpResponse> post(
        const std::string& host, 
        const std::string& port, 
        const std::string& target, 
        const std::string& body, 
        const std::string& token);

    kaiten::Result<HttpResponse> get(
        const std::string& host, 
        const std::string& port, 
        const std::string& target, 
        const std::string& token);

    kaiten::Result<HttpResponse> patch(
        const std::string& host, 
        const std::string& port, 
        const std::string& target, 
        const std::string& body, 
        const std::string& token);

    // Set custom rate limiter
    void set_rate_limiter(kaiten::Rate_limiter* limiter) {
        _rate_limiter = limiter;
    }

private:
    kaiten::Result<HttpResponse> handle_error(
        const char* action, 
        beast::error_code ec, 
        const std::string& error_msg);

    kaiten::Result<HttpResponse> make_request(
        const std::string& host,
        const std::string& port,
        const std::string& target,
        http::verb method,
        const std::string& body,
        const std::string& token);

    net::io_context& _ioc;
    ssl::context& _ctx;
    kaiten::Rate_limiter* _rate_limiter;

    // Включить/выключить rate limiting
    void set_rate_limiting_enabled(bool enabled) {
        if (_rate_limiter != nullptr) {
            _rate_limiter->set_enabled(enabled);
        }
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
};

#endif // HTTP_CLIENT_HPP 
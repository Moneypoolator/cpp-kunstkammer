#ifndef HTTP_CLIENT_HPP
#define HTTP_CLIENT_HPP

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

class Http_client {
public:
    Http_client(net::io_context& ioc, ssl::context& ctx);

    // Returns pair<status_code, response_body>
    std::pair<int, std::string> post(const std::string& host, const std::string& port, const std::string& target, const std::string& body, const std::string& token);
    std::pair<int, std::string> get(const std::string& host, const std::string& port, const std::string& target, const std::string& token);
    std::pair<int, std::string> patch(const std::string& host, const std::string& port, const std::string& target, const std::string& body, const std::string& token);

private:
    net::io_context& _ioc;
    ssl::context& _ctx;
};

#endif // HTTP_CLIENT_HPP 
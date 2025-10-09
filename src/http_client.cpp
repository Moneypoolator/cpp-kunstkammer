#include "http_client.hpp"

namespace beast = boost::beast;
namespace http = beast::http;

Http_client::Http_client(net::io_context& ioc, ssl::context& ctx, kaiten::Rate_limiter* rate_limiter)
    : _ioc(ioc), _ctx(ctx), _rate_limiter(rate_limiter) {}

// Performs an HTTP POST and returns the response (status code, body)
std::pair<int, std::string> Http_client::post(const std::string& host, const std::string& port, const std::string& target, const std::string& body, const std::string& token)
{
    apply_rate_limiting(); // Применяем rate limiting

    try
    {
        tcp::resolver resolver(_ioc);
        beast::ssl_stream<beast::tcp_stream> stream(_ioc, _ctx);

        // Set SNI Hostname (many hosts need this to handshake successfully)
        if(! SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
        {
            beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
            throw beast::system_error{ec};
        }

        auto const results = resolver.resolve(host, port);
        beast::get_lowest_layer(stream).connect(results);
        stream.handshake(ssl::stream_base::client);

        http::request<http::string_body> req{http::verb::post, target, 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::content_type, "application/json");
        req.set(http::field::accept, "application/json");
        req.set(http::field::authorization, "Bearer " + token);
        req.body() = body;
        req.prepare_payload();
        std::cout << "Outgoing JSON body: " << body << std::endl;

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        http::read(stream, buffer, res);

        beast::error_code ec;
        stream.shutdown(ec);
        if(ec == net::error::eof) { ec = {}; }
        if(ec) { 
            throw beast::system_error{ec};
        }

        int status = static_cast<int>(res.result());
        std::string response_body = beast::buffers_to_string(res.body().data());
        return {status, response_body};
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return {-1, ""};
    }
}

// Performs an HTTP GET and returns the response (status code, body)
std::pair<int, std::string> Http_client::get(const std::string& host, const std::string& port, const std::string& target, const std::string& token)
{
    apply_rate_limiting(); // Применяем rate limiting

    try
    {
        std::cout << "GET Host: " << host << "\nGET Port: " << port << "\nGET Path: " << target << std::endl;

        tcp::resolver resolver(_ioc);
        beast::ssl_stream<beast::tcp_stream> stream(_ioc, _ctx);

        if(! SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
        {
            beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
            throw beast::system_error{ec};
        }

        auto const results = resolver.resolve(host, port);
        beast::get_lowest_layer(stream).connect(results);
        stream.handshake(ssl::stream_base::client);

        http::request<http::empty_body> req{http::verb::get, target, 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, "curl/7.81.0");
        req.set(http::field::accept, "application/json");
        req.set(http::field::connection, "close");
        if (!token.empty()) {
            req.set(http::field::authorization, "Bearer " + token);
        }

        std::cout << "GET Headers:\n";
        for (auto const& field : req) {
            std::cout << "  " << field.name_string() << ": " << field.value() << std::endl;
        }

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        http::read(stream, buffer, res);

        beast::error_code ec;
        stream.shutdown(ec);
        if(ec == net::error::eof) { ec = {}; }
        if(ec) { 
            throw beast::system_error{ec}; 
        }

        int status = static_cast<int>(res.result());
        std::string response_body = beast::buffers_to_string(res.body().data());
        return {status, response_body};
    }
    catch(std::exception const& e)
    {
        std::cerr << "GET Error: " << e.what() << std::endl;
        return {-1, ""};
    }
} 

// Performs an HTTP PATCH and returns the response (status code, body)
std::pair<int, std::string> Http_client::patch(const std::string& host, const std::string& port, const std::string& target, const std::string& body, const std::string& token)
{
    apply_rate_limiting(); // Применяем rate limiting
    
    try
    {
        tcp::resolver resolver(_ioc);
        beast::ssl_stream<beast::tcp_stream> stream(_ioc, _ctx);

        if(! SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
        {
            beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
            throw beast::system_error{ec};
        }

        auto const results = resolver.resolve(host, port);
        beast::get_lowest_layer(stream).connect(results);
        stream.handshake(ssl::stream_base::client);

        http::request<http::string_body> req{http::verb::patch, target, 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::content_type, "application/json");
        req.set(http::field::accept, "application/json");
        req.set(http::field::authorization, "Bearer " + token);
        req.body() = body;
        req.prepare_payload();

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        http::read(stream, buffer, res);

        beast::error_code ec;
        stream.shutdown(ec);
        if(ec == net::error::eof) { ec = {}; }
        if(ec) {
            throw beast::system_error{ec}; 
        }

        int status = static_cast<int>(res.result());
        std::string response_body = beast::buffers_to_string(res.body().data());
        return {status, response_body};
    }
    catch(std::exception const& e)
    {
        std::cerr << "PATCH Error: " << e.what() << std::endl;
        return {-1, ""};
    }
}
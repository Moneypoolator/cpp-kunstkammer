#include "http_client.hpp"

namespace beast = boost::beast;
namespace http = beast::http;

Http_client::Http_client(net::io_context& ioc, ssl::context& ctx, kaiten::Rate_limiter* rate_limiter):
    _ioc(ioc), _ctx(ctx), _rate_limiter(rate_limiter)
{
    _ctx.set_default_verify_paths();
    _ctx.set_verify_mode(ssl::verify_none);
}

kaiten::Result<HttpResponse> Http_client::handle_error(
    const char* action, 
    beast::error_code ec, 
    const std::string& error_msg)
{
    LOG(ERROR) << action << ": " << error_msg << " - " << ec.message();
    return kaiten::make_error<HttpResponse>(
        std::make_unique<kaiten::HttpError>(
            ec.value(), 
            error_msg + ": " + ec.message()));
}

kaiten::Result<HttpResponse> Http_client::make_request(
    const std::string& host,
    const std::string& port,
    const std::string& target,
    http::verb method,
    const std::string& body,
    const std::string& token)
{
    apply_rate_limiting();

    try {
        LOG(INFO) << method << " " << host << ":" << port << target;

        tcp::resolver resolver(_ioc);
        beast::ssl_stream<beast::tcp_stream> stream(_ioc, _ctx);

        if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
            beast::error_code ec{static_cast<int>(::ERR_get_error()), 
                               net::error::get_ssl_category()};
            return handle_error("SSL SNI", ec, "Failed to set SNI Hostname");
        }

        auto const results = resolver.resolve(host, port);
        beast::get_lowest_layer(stream).connect(results);
        stream.handshake(ssl::stream_base::client);

        http::request<http::string_body> req{method, target, 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::content_type, "application/json");
        req.set(http::field::accept, "application/json");
        req.set(http::field::authorization, "Bearer " + token);
        
        if (!body.empty()) {
            req.body() = body;
            req.prepare_payload();
        }

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        http::read(stream, buffer, res);

        beast::error_code ec;
        stream.shutdown(ec);
        if (ec == net::error::eof || ec == ssl::error::stream_truncated) {
            // These are expected
            ec = {};
        }
        if (ec) {
            LOG(WARNING) << "SSL shutdown: " << ec.message();
        }

        int status = static_cast<int>(res.result());
        std::string response_body = beast::buffers_to_string(res.body().data());

        // Handle specific HTTP status codes
        if (status == 401) {
            return kaiten::make_error<HttpResponse>(
                std::make_unique<kaiten::AuthError>(
                    status, "Unauthorized: Invalid or expired token"));
        }
        if (status == 429) {
            int retry_after = 60; // Default value
            if (auto retry = res[http::field::retry_after]) {
                retry_after = std::stoi(std::string(retry));
            }
            return kaiten::make_error<HttpResponse>(
                std::make_unique<kaiten::RateLimitError>(
                    status, "Rate limit exceeded", retry_after));
        }
        if (status >= 400) {
            return kaiten::make_error<HttpResponse>(
                std::make_unique<kaiten::HttpError>(
                    status, "HTTP error " + std::to_string(status)));
        }

        return kaiten::make_success(HttpResponse(status, response_body));
    }
    catch (const beast::system_error& e) {
        return handle_error("HTTP request", e.code(), 
                          std::string(method) + " request failed");
    }
    catch (const std::exception& e) {
        LOG(ERROR) << method << " error: " << e.what();
        return kaiten::make_error<HttpResponse>(
            std::make_unique<kaiten::HttpError>(
                -1, std::string(method) + " error: " + e.what()));
    }
}

kaiten::Result<HttpResponse> Http_client::post(
    const std::string& host, 
    const std::string& port, 
    const std::string& target, 
    const std::string& body, 
    const std::string& token)
{
    return make_request(host, port, target, http::verb::post, body, token);
}

kaiten::Result<HttpResponse> Http_client::get(
    const std::string& host, 
    const std::string& port, 
    const std::string& target, 
    const std::string& token)
{
    return make_request(host, port, target, http::verb::get, "", token);
}

kaiten::Result<HttpResponse> Http_client::patch(
    const std::string& host, 
    const std::string& port, 
    const std::string& target, 
    const std::string& body, 
    const std::string& token)
{
    return make_request(host, port, target, http::verb::patch, body, token);
}
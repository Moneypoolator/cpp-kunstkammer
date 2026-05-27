#include "http_client.hpp"
#include "logger.hpp"

namespace beast = boost::beast;
namespace http = beast::http;

Http_client::Http_client(net::io_context& ioc, ssl::context& ctx, kaiten::Rate_limiter* rate_limiter):
    _ioc(ioc), _ctx(ctx), _rate_limiter(rate_limiter)
{
    // Настраиваем SSL контекст для лучшей совместимости
    _ctx.set_default_verify_paths();
    _ctx.set_verify_mode(ssl::verify_none); // Отключаем проверку сертификата для тестирования
    
    // Initialize connection pool
    _connection_pool = std::make_shared<kaiten::Connection_pool>(ioc, ctx);
}

// Performs an HTTP POST and returns the response (status code, body)
std::pair<int, std::string> Http_client::post(const std::string& host, const std::string& port, const std::string& target, const std::string& body, const std::string& token)
{
    apply_rate_limiting(); // Применяем rate limiting

    try
    {
        LOG_DEBUG("POST Host: {} Port: {} Path: {}", host, port, target);

        // Get connection from pool
        auto conn_result = _connection_pool->get_connection(host, port);
        if (conn_result.first != 0) {
            LOG_ERROR("Failed to get connection from pool");
            return { -1, "" };
        }

        auto pooled_conn = conn_result.second;
        auto& stream = pooled_conn->get_stream();

        http::request<http::string_body> req { http::verb::post, target, 11 };
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::content_type, "application/json");
        req.set(http::field::accept, "application/json");
        req.set(http::field::authorization, "Bearer " + token);
        req.body() = body;
        req.prepare_payload();

        LOG_DEBUG("Outgoing request to: {}{}", host, target);
        LOG_TRACE("Request body: {}", body);

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        http::read(stream, buffer, res);

        int status = static_cast<int>(res.result());
        std::string response_body = beast::buffers_to_string(res.body().data());

        LOG_DEBUG("Response status: {}", status);
        // LOG_TRACE("Response body: {}", response_body);

        return { status, response_body };
    } catch (const beast::system_error& e)
    {
        LOG_ERROR("Beast system error in POST: {}", e.what());
        if (e.code().category() == net::error::get_ssl_category()) {
            LOG_ERROR("SSL error code: {}", e.code().value());
        }
        return { -1, "" };
    } catch (std::exception const& e)
    {
        LOG_ERROR("Error in POST: {}", e.what());
        return { -1, "" };
    }
}

// Performs an HTTP GET and returns the response (status code, body)
std::pair<int, std::string> Http_client::get(const std::string& host, const std::string& port, const std::string& target, const std::string& token)
{
    apply_rate_limiting(); // Применяем rate limiting

    try
    {
        LOG_DEBUG("GET Host: {} Port: {} Path: {}", host, port, target);

        // Get connection from pool
        auto conn_result = _connection_pool->get_connection(host, port);
        if (conn_result.first != 0) {
            LOG_ERROR("Failed to get connection from pool");
            return { -1, "" };
        }

        auto pooled_conn = conn_result.second;
        auto& stream = pooled_conn->get_stream();

        http::request<http::empty_body> req { http::verb::get, target, 11 };
        req.set(http::field::host, host);
        req.set(http::field::user_agent, "curl/7.81.0");
        req.set(http::field::accept, "application/json");
        if (!token.empty()) {
            req.set(http::field::authorization, "Bearer " + token);
        }

        LOG_TRACE("GET Headers:");
        for (auto const& field : req) {
            LOG_TRACE("  {}: {}", field.name_string(), field.value());
        }

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        http::read(stream, buffer, res);

        int status = static_cast<int>(res.result());
        std::string response_body = beast::buffers_to_string(res.body().data());
        
        return { status, response_body };
    } catch (std::exception const& e)
    {
        LOG_ERROR("GET Error: {}", e.what());
        return { -1, "" };
    }
}

// Performs an HTTP PATCH and returns the response (status code, body)
std::pair<int, std::string> Http_client::patch(const std::string& host, const std::string& port, const std::string& target, const std::string& body, const std::string& token)
{
    apply_rate_limiting(); // Применяем rate limiting

    try
    {
        // Get connection from pool
        auto conn_result = _connection_pool->get_connection(host, port);
        if (conn_result.first != 0) {
            LOG_ERROR("Failed to get connection from pool");
            return { -1, "" };
        }

        auto pooled_conn = conn_result.second;
        auto& stream = pooled_conn->get_stream();

        http::request<http::string_body> req { http::verb::patch, target, 11 };
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

        int status = static_cast<int>(res.result());
        std::string response_body = beast::buffers_to_string(res.body().data());

        return { status, response_body };
    } catch (std::exception const& e)
    {
        LOG_ERROR("PATCH Error: {}", e.what());
        return { -1, "" };
    }
}
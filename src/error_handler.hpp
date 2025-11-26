#ifndef KAITEN_ERROR_HANDLER_HPP
#define KAITEN_ERROR_HANDLER_HPP

#include <string>
#include <exception>

namespace kaiten {
namespace error_handler {

    // Error categories
    enum class Error_category {
        NETWORK,
        AUTHENTICATION,
        API,
        PARSING,
        VALIDATION,
        CONFIGURATION,
        FILESYSTEM,
        UNKNOWN
    };

    // Detailed error information
    struct Error_info {
        Error_category category;
        int http_status;
        std::string message;
        std::string details;
        std::string recovery_suggestion;
        std::string raw_response;
    };

    // Enhanced error logging with context
    void log_error(const Error_info& error, const std::string& context = "");
    
    // Generate user-friendly error messages
    std::string format_error_message(const Error_info& error);
    
    // Generate recovery suggestions based on error type
    std::string generate_recovery_suggestion(const Error_info& error);
    
    // Parse API error responses
    Error_info parse_api_error(int status, const std::string& response, const std::string& action);
    
    // Handle common HTTP errors
    Error_info handle_http_error(int status, const std::string& response, const std::string& action);
    
    // Handle JSON parsing errors
    Error_info handle_parsing_error(const std::exception& e, const std::string& context, const std::string& data = "");
    
    // Handle network errors
    Error_info handle_network_error(const std::exception& e, const std::string& operation);
    
    // Handle configuration errors
    Error_info handle_config_error(const std::string& message, const std::string& file_path = "");

} // namespace error_handler
} // namespace kaiten

#endif // KAITEN_ERROR_HANDLER_HPP
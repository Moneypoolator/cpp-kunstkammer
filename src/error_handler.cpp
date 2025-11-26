#include "error_handler.hpp"

#include <iostream>
#include <sstream>
#include "../third_party/nlohmann/json.hpp"

namespace kaiten {
namespace error_handler {

    void log_error(const Error_info& error, const std::string& context) {
        std::ostringstream oss;
        
        if (!context.empty()) {
            oss << "[" << context << "] ";
        }
        
        oss << "Error: ";
        
        switch (error.category) {
            case Error_category::NETWORK:
                oss << "[NETWORK] ";
                break;
            case Error_category::AUTHENTICATION:
                oss << "[AUTH] ";
                break;
            case Error_category::API:
                oss << "[API] ";
                break;
            case Error_category::PARSING:
                oss << "[PARSING] ";
                break;
            case Error_category::VALIDATION:
                oss << "[VALIDATION] ";
                break;
            case Error_category::CONFIGURATION:
                oss << "[CONFIG] ";
                break;
            case Error_category::FILESYSTEM:
                oss << "[FILESYSTEM] ";
                break;
            case Error_category::UNKNOWN:
                oss << "[UNKNOWN] ";
                break;
        }
        
        oss << error.message;
        
        if (error.http_status > 0) {
            oss << " (HTTP " << error.http_status << ")";
        }
        
        if (!error.details.empty()) {
            oss << " - " << error.details;
        }
        
        std::cerr << oss.str() << std::endl;
        
        if (!error.recovery_suggestion.empty()) {
            std::cerr << "Suggestion: " << error.recovery_suggestion << std::endl;
        }
    }
    
    std::string format_error_message(const Error_info& error) {
        std::ostringstream oss;
        
        switch (error.category) {
            case Error_category::NETWORK:
                oss << "Network error: ";
                break;
            case Error_category::AUTHENTICATION:
                oss << "Authentication error: ";
                break;
            case Error_category::API:
                oss << "API error: ";
                break;
            case Error_category::PARSING:
                oss << "Data parsing error: ";
                break;
            case Error_category::VALIDATION:
                oss << "Validation error: ";
                break;
            case Error_category::CONFIGURATION:
                oss << "Configuration error: ";
                break;
            case Error_category::FILESYSTEM:
                oss << "File system error: ";
                break;
            case Error_category::UNKNOWN:
                oss << "Error: ";
                break;
        }
        
        oss << error.message;
        
        if (error.http_status > 0) {
            oss << " (HTTP " << error.http_status << ")";
        }
        
        return oss.str();
    }
    
    std::string generate_recovery_suggestion(const Error_info& error) {
        switch (error.category) {
            case Error_category::NETWORK:
                return "Check your internet connection and try again. If the problem persists, check if the Kaiten API is accessible.";
                
            case Error_category::AUTHENTICATION:
                return "Verify your API token is correct and has the necessary permissions. Check your configuration file.";
                
            case Error_category::API:
                if (error.http_status == 400) {
                    return "The request was malformed. Check your input parameters and try again.";
                } else if (error.http_status == 401) {
                    return "Authentication failed. Check your API token and permissions.";
                } else if (error.http_status == 403) {
                    return "Access denied. You may not have permission to perform this action.";
                } else if (error.http_status == 404) {
                    return "The requested resource was not found. Check if the ID or identifier is correct.";
                } else if (error.http_status >= 500) {
                    return "The server encountered an error. Try again later or contact Kaiten support if the problem persists.";
                }
                return "An API error occurred. Check the error details and try again.";
                
            case Error_category::PARSING:
                return "Data parsing failed. This may indicate an issue with the API response format. Please report this issue.";
                
            case Error_category::VALIDATION:
                return "Input validation failed. Check your input values and try again.";
                
            case Error_category::CONFIGURATION:
                return "Configuration error detected. Check your configuration file and ensure all required fields are present.";
                
            case Error_category::FILESYSTEM:
                return "File system error. Check file permissions and available disk space.";
                
            case Error_category::UNKNOWN:
            default:
                return "An unexpected error occurred. Check the error details and try again.";
        }
    }
    
    Error_info parse_api_error(int status, const std::string& response, const std::string& action) {
        Error_info error;
        error.category = Error_category::API;
        error.http_status = status;
        error.message = action + " failed";
        error.raw_response = response;
        
        if (!response.empty()) {
            try {
                auto err = nlohmann::json::parse(response);
                if (err.is_object()) {
                    if (err.contains("message") && err["message"].is_string()) {
                        error.details = err["message"].get<std::string>();
                    }
                    if (err.contains("errors")) {
                        std::ostringstream oss;
                        oss << "Validation errors: " << err["errors"].dump();
                        error.details += (error.details.empty() ? "" : " - ") + oss.str();
                    }
                }
            } catch (...) {
                error.details = "Raw response: " + response;
            }
        }
        
        error.recovery_suggestion = generate_recovery_suggestion(error);
        return error;
    }
    
    Error_info handle_http_error(int status, const std::string& response, const std::string& action) {
        Error_info error;
        error.category = Error_category::API;
        error.http_status = status;
        error.message = action + " failed";
        error.raw_response = response;
        
        switch (status) {
            case 400:
                error.message += " - Bad Request";
                break;
            case 401:
                error.message += " - Unauthorized";
                error.category = Error_category::AUTHENTICATION;
                break;
            case 402:
                error.message += " - Payment Required";
                break;
            case 403:
                error.message += " - Forbidden";
                break;
            case 404:
                error.message += " - Not Found";
                break;
            case 429:
                error.message += " - Rate Limited";
                break;
            case 500:
                error.message += " - Internal Server Error";
                break;
            case 502:
                error.message += " - Bad Gateway";
                break;
            case 503:
                error.message += " - Service Unavailable";
                break;
            default:
                if (status >= 400 && status < 500) {
                    error.message += " - Client Error";
                } else if (status >= 500) {
                    error.message += " - Server Error";
                }
                break;
        }
        
        if (!response.empty()) {
            try {
                auto err = nlohmann::json::parse(response);
                if (err.is_object()) {
                    if (err.contains("message") && err["message"].is_string()) {
                        error.details = err["message"].get<std::string>();
                    }
                }
            } catch (...) {
                error.details = "Raw response: " + response;
            }
        }
        
        error.recovery_suggestion = generate_recovery_suggestion(error);
        return error;
    }
    
    Error_info handle_parsing_error(const std::exception& e, const std::string& context, const std::string& data) {
        Error_info error;
        error.category = Error_category::PARSING;
        error.message = "Failed to parse " + context;
        error.details = e.what();
        error.raw_response = data;
        error.recovery_suggestion = generate_recovery_suggestion(error);
        return error;
    }
    
    Error_info handle_network_error(const std::exception& e, const std::string& operation) {
        Error_info error;
        error.category = Error_category::NETWORK;
        error.message = "Network error during " + operation;
        error.details = e.what();
        error.recovery_suggestion = generate_recovery_suggestion(error);
        return error;
    }
    
    Error_info handle_config_error(const std::string& message, const std::string& file_path) {
        Error_info error;
        error.category = Error_category::CONFIGURATION;
        error.message = "Configuration error";
        error.details = message;
        if (!file_path.empty()) {
            error.details += " (file: " + file_path + ")";
        }
        error.recovery_suggestion = generate_recovery_suggestion(error);
        return error;
    }

} // namespace error_handler
} // namespace kaiten
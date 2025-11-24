#include "error_handler.hpp"

#include <iostream>
#include <sstream>
#include "../third_party/nlohmann/json.hpp"

namespace kaiten {
namespace error_handler {

    void log_error(const ErrorInfo& error, const std::string& context) {
        std::ostringstream oss;
        
        if (!context.empty()) {
            oss << "[" << context << "] ";
        }
        
        oss << "Error: ";
        
        switch (error.category) {
            case ErrorCategory::NETWORK:
                oss << "[NETWORK] ";
                break;
            case ErrorCategory::AUTHENTICATION:
                oss << "[AUTH] ";
                break;
            case ErrorCategory::API:
                oss << "[API] ";
                break;
            case ErrorCategory::PARSING:
                oss << "[PARSING] ";
                break;
            case ErrorCategory::VALIDATION:
                oss << "[VALIDATION] ";
                break;
            case ErrorCategory::CONFIGURATION:
                oss << "[CONFIG] ";
                break;
            case ErrorCategory::FILESYSTEM:
                oss << "[FILESYSTEM] ";
                break;
            case ErrorCategory::UNKNOWN:
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
    
    std::string format_error_message(const ErrorInfo& error) {
        std::ostringstream oss;
        
        switch (error.category) {
            case ErrorCategory::NETWORK:
                oss << "Network error: ";
                break;
            case ErrorCategory::AUTHENTICATION:
                oss << "Authentication error: ";
                break;
            case ErrorCategory::API:
                oss << "API error: ";
                break;
            case ErrorCategory::PARSING:
                oss << "Data parsing error: ";
                break;
            case ErrorCategory::VALIDATION:
                oss << "Validation error: ";
                break;
            case ErrorCategory::CONFIGURATION:
                oss << "Configuration error: ";
                break;
            case ErrorCategory::FILESYSTEM:
                oss << "File system error: ";
                break;
            case ErrorCategory::UNKNOWN:
                oss << "Error: ";
                break;
        }
        
        oss << error.message;
        
        if (error.http_status > 0) {
            oss << " (HTTP " << error.http_status << ")";
        }
        
        return oss.str();
    }
    
    std::string generate_recovery_suggestion(const ErrorInfo& error) {
        switch (error.category) {
            case ErrorCategory::NETWORK:
                return "Check your internet connection and try again. If the problem persists, check if the Kaiten API is accessible.";
                
            case ErrorCategory::AUTHENTICATION:
                return "Verify your API token is correct and has the necessary permissions. Check your configuration file.";
                
            case ErrorCategory::API:
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
                
            case ErrorCategory::PARSING:
                return "Data parsing failed. This may indicate an issue with the API response format. Please report this issue.";
                
            case ErrorCategory::VALIDATION:
                return "Input validation failed. Check your input values and try again.";
                
            case ErrorCategory::CONFIGURATION:
                return "Configuration error detected. Check your configuration file and ensure all required fields are present.";
                
            case ErrorCategory::FILESYSTEM:
                return "File system error. Check file permissions and available disk space.";
                
            case ErrorCategory::UNKNOWN:
            default:
                return "An unexpected error occurred. Check the error details and try again.";
        }
    }
    
    ErrorInfo parse_api_error(int status, const std::string& response, const std::string& action) {
        ErrorInfo error;
        error.category = ErrorCategory::API;
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
    
    ErrorInfo handle_http_error(int status, const std::string& response, const std::string& action) {
        ErrorInfo error;
        error.category = ErrorCategory::API;
        error.http_status = status;
        error.message = action + " failed";
        error.raw_response = response;
        
        switch (status) {
            case 400:
                error.message += " - Bad Request";
                break;
            case 401:
                error.message += " - Unauthorized";
                error.category = ErrorCategory::AUTHENTICATION;
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
    
    ErrorInfo handle_parsing_error(const std::exception& e, const std::string& context, const std::string& data) {
        ErrorInfo error;
        error.category = ErrorCategory::PARSING;
        error.message = "Failed to parse " + context;
        error.details = e.what();
        error.raw_response = data;
        error.recovery_suggestion = generate_recovery_suggestion(error);
        return error;
    }
    
    ErrorInfo handle_network_error(const std::exception& e, const std::string& operation) {
        ErrorInfo error;
        error.category = ErrorCategory::NETWORK;
        error.message = "Network error during " + operation;
        error.details = e.what();
        error.recovery_suggestion = generate_recovery_suggestion(error);
        return error;
    }
    
    ErrorInfo handle_config_error(const std::string& message, const std::string& file_path) {
        ErrorInfo error;
        error.category = ErrorCategory::CONFIGURATION;
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
#include "user_operations.hpp"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "cache.hpp"
#include "card.hpp"
#include "card_utils.hpp"
#include "http_client.hpp"
#include "pagination.hpp"
#include "error_handler.hpp"
#include "api_utils.hpp"

namespace kaiten {
namespace user_operations {

// Gets a specific user by ID
std::pair<int, User> get_user(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
    const std::string& api_path,
    const std::string& token,
    std::int64_t space_id,
    std::int64_t user_id)
{
    // Пробуем получить из кэша
    auto cached = Api_cache::user_cache().get(user_id);
    if (cached.has_value()) {
        std::cout << "Cache HIT for user ID: " << user_id << std::endl;
        return std::make_pair(200, cached.value());
    }

    std::cout << "Cache MISS for user ID: " << user_id << std::endl;

    std::string target = api_path + "/spaces/" + std::to_string(space_id) + "/users/" + std::to_string(user_id);

    auto [status, response] = client.get(host, port, target, token);
    if (status == 200) {
        try {
            auto j = nlohmann::json::parse(response);
            User user = j.get<User>();

            // Сохраняем в кэш
            Api_cache::user_cache().put(user.id, user);

            return std::make_pair(status, user);
        } catch (const std::exception& e) {
            auto error = kaiten::error_handler::handle_parsing_error(e, "user JSON", response);
            kaiten::error_handler::log_error(error, "get_user");
            return std::make_pair(status, User {});
        } catch (...) {
            auto error = kaiten::error_handler::ErrorInfo{
                kaiten::error_handler::ErrorCategory::PARSING,
                status,
                "Failed to parse user JSON",
                "Unknown parsing error",
                "Check the API response format and try again",
                response
            };
            kaiten::error_handler::log_error(error, "get_user");
            return std::make_pair(status, User {});
        }
    }

    kaiten::api_utils::log_api_error("Get user failed", status, response);

    return std::make_pair(status, User {});
}

// Gets the current authenticated user
std::pair<int, User> get_current_user(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
    const std::string& api_path,
    const std::string& token)
{
    std::string target = api_path + "/users/current";

    auto [status, response] = client.get(host, port, target, token);
    if (status == 200) {
        try {
            auto j = nlohmann::json::parse(response);
            User user = j.get<User>();
            return std::make_pair(status, user);
        } catch (const std::exception& e) {
            auto error = kaiten::error_handler::handle_parsing_error(e, "current user JSON", response);
            kaiten::error_handler::log_error(error, "get_current_user");
            return std::make_pair(status, User {});
        } catch (...) {
            auto error = kaiten::error_handler::ErrorInfo{
                kaiten::error_handler::ErrorCategory::PARSING,
                status,
                "Failed to parse current user JSON",
                "Unknown parsing error",
                "Check the API response format and try again",
                response
            };
            kaiten::error_handler::log_error(error, "get_current_user");
            return std::make_pair(status, User {});
        }
    }

    kaiten::api_utils::log_api_error("Get current user failed", status, response);

    return std::make_pair(status, User {});
}

// Gets users by email (filter). Returns array; caller can decide how to handle multiple matches
std::pair<int, std::vector<User>> get_users_by_email(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
    const std::string& api_path,
    const std::string& token,
    const std::string& email)
{
    std::string target = api_path + "/users?email=" + email; // assume email URL-safe; encode if needed

    auto [status, response] = client.get(host, port, target, token);
    if (status == 200) {
        try {
            auto j = nlohmann::json::parse(response);
            std::vector<User> users;
            if (j.is_array()) {
                users = j.get<std::vector<User>>();
            } else if (j.is_object() && j.contains("users") && j["users"].is_array()) {
                users = j["users"].get<std::vector<User>>();
            }
            return std::make_pair(status, users);
        } catch (const std::exception& e) {
            auto error = kaiten::error_handler::handle_parsing_error(e, "users-by-email JSON", response);
            kaiten::error_handler::log_error(error, "get_users_by_email");
            return std::make_pair(status, std::vector<User>{});
        } catch (...) {
            auto error = kaiten::error_handler::ErrorInfo{
                kaiten::error_handler::ErrorCategory::PARSING,
                status,
                "Failed to parse users-by-email JSON",
                "Unknown parsing error",
                "Check the API response format and try again",
                response
            };
            kaiten::error_handler::log_error(error, "get_users_by_email");
            return std::make_pair(status, std::vector<User>{});
        }
    }

    kaiten::api_utils::log_api_error("Get users by email failed", status, response);

    return std::make_pair(status, std::vector<User>{});
}

// Improved paginated users with correct pagination
Paginated_result<User> get_users_paginated(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
    const std::string& api_path,
    const std::string& token,
    const Pagination_params& pagination,
    const User_filter_params& filters)
{
    Paginated_result<User> result;

    // Убедимся, что limit не превышает максимум
    Pagination_params safe_pagination = pagination;
    if (safe_pagination.limit > 100) {
        safe_pagination.limit = 100;
    }

    std::string query = Query_builder::build(safe_pagination, filters);
    std::string target = api_path + "/users" + query;

    auto [status, response] = client.get(host, port, target, token);

    if (status != 200) {
        auto error = kaiten::error_handler::handle_http_error(status, response, "fetch users");
        kaiten::error_handler::log_error(error, "get_users_paginated");
        return result;
    }

    try {
        auto json = nlohmann::json::parse(response);

        if (json.is_array()) {
            result.items = json.get<std::vector<User>>();
        } else {
            auto error = kaiten::error_handler::ErrorInfo{
                kaiten::error_handler::ErrorCategory::API,
                status,
                "Unexpected response format for users",
                "Response is not an array",
                "Check the API response format and try again",
                json.dump(2)
            };
            kaiten::error_handler::log_error(error, "get_users_paginated");
            return result;
        }

        result.limit = safe_pagination.limit;
        result.offset = safe_pagination.offset;
        result.has_more = result.items.size() == static_cast<size_t>(safe_pagination.limit);

    } catch (const std::exception& e) {
        auto error = kaiten::error_handler::handle_parsing_error(e, "users paginated response", response);
        kaiten::error_handler::log_error(error, "get_users_paginated");
    }

    return result;
}

std::pair<int, std::vector<User>> get_all_users(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
    const std::string& api_path,
    const std::string& token,
    const User_filter_params& filters,
    int page_size)
{
    int last_status = 200;
    
    Pagination_params pagination;
    pagination.limit = (page_size > 100) ? 100 : page_size; // Kaiten API max limit is 100
    
    std::cout << "Starting fetch of all users..." << std::endl;
    
    // Fetch the first page to determine if there are more pages
    std::cout << "Fetching first page of users..." << std::endl;
    
    pagination.offset = 0;
    auto first_page_result = get_users_paginated(client, host, port, api_path, token, 
                                                pagination, filters);
    
    if (first_page_result.items.empty()) {
        std::cout << "No users found." << std::endl;
        return std::make_pair(last_status, std::vector<User>{});
    }
    
    // Store first page results
    std::vector<User> all_users = first_page_result.items;
    std::cout << "Page 0 (offset 0): " << first_page_result.items.size() << " users" << std::endl;
    
    // Fetch remaining pages if there are more
    if (first_page_result.has_more) {
        int current_offset = pagination.limit;
        
        std::cout << "Fetching additional pages..." << std::endl;
        
        // Continue fetching pages until no more results
        while (true) {
            pagination.offset = current_offset;
            auto page_result = get_users_paginated(client, host, port, api_path, token, 
                                                   pagination, filters);
            
            if (page_result.items.empty()) {
                break;
            }
            
            all_users.insert(all_users.end(), 
                           page_result.items.begin(), 
                           page_result.items.end());
            
            std::cout << "Page at offset " << current_offset << ": " << page_result.items.size() 
                      << " users, total: " << all_users.size() << std::endl;
            
            // If this page doesn't have more results, we're done
            if (!page_result.has_more) {
                break;
            }
            
            current_offset += pagination.limit;
        }
    }
    
    std::cout << "Finished fetching users. Total: " << all_users.size() << std::endl;
    
    return std::make_pair(last_status, all_users);
}

} // namespace user_operations
} // namespace kaiten
#ifndef USER_OPERATIONS_HPP
#define USER_OPERATIONS_HPP

#include <string>
#include <vector>
#include <utility>

#include "card.hpp"
#include "http_client.hpp"
#include "pagination.hpp"

namespace kaiten {
namespace user_operations {

// User operations
std::pair<int, User> get_user(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
    const std::string& api_path,
    const std::string& token,
    std::int64_t space_id,
    std::int64_t user_id);

// Gets the current authenticated user
std::pair<int, User> get_current_user(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
    const std::string& api_path,
    const std::string& token);

// Gets users by email filter (exact match). Returns zero, one, or many users
std::pair<int, std::vector<User>> get_users_by_email(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
    const std::string& api_path,
    const std::string& token,
    const std::string& email);

Paginated_result<User> get_users_paginated(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
    const std::string& api_path,
    const std::string& token,
    const Pagination_params& pagination = {},
    const User_filter_params& filters = {});

std::pair<int, std::vector<User>> get_all_users(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
    const std::string& api_path,
    const std::string& token,
    const User_filter_params& filters = {},
    int page_size = 100);

} // namespace user_operations
} // namespace kaiten

#endif // USER_OPERATIONS_HPP
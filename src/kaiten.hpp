#ifndef KAITEN_HPP
#define KAITEN_HPP

#include <string>
#include <vector>
#include <map>
#include <optional>

#include "card.hpp"
#include "http_client.hpp"
#include "pagination.hpp"

namespace kaiten {

// Basic card operations
Result<Card> create_card(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
    const std::string& api_path,
    const std::string& token,
    const Simple_card& desired);

Result<Card> update_card(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
    const std::string& api_path,
    const std::string& token,
    const std::string& id_or_number,
    const Simple_card& changes);

Result<Card> get_card(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
    const std::string& api_path,
    const std::string& token,
    const std::string& id_or_number);

// Card relationships
// Add an existing card as a child to a parent card
// POST { "card_id": <child_id> } to /cards/{parent_id}/children
Result<bool> add_child_card(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
    const std::string& api_path,
    const std::string& token,
    std::int64_t parent_card_id,
    std::int64_t child_card_id);

// Add tag to card
Result<bool> add_tag_to_card(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
    const std::string& api_path,
    const std::string& token,
    std::int64_t card_id,
    const std::string& tag);    

// Advanced paginated card operations
Paginated_result<Card> get_cards_paginated(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
    const std::string& api_path,
    const std::string& token,
    const Pagination_params& pagination = {},
    const Card_filter_params& filters = {});

Result<std::vector<Card>> get_all_cards(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
    const std::string& api_path,
    const std::string& token,
    const Card_filter_params& filters = {},
    int page_size = 100);

// User operations
Result<User> get_user(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
    const std::string& api_path,
    const std::string& token,
    std::int64_t space_id,
    std::int64_t user_id);

// Gets the current authenticated user
Result<User> get_current_user(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
    const std::string& api_path,
    const std::string& token);

// Gets users by email filter (exact match). Returns zero, one, or many users
Result<std::vector<User>> get_users_by_email(
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

Result<std::vector<User>> get_all_users(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
    const std::string& api_path,
    const std::string& token,
    const User_filter_params& filters = {},
    int page_size = 100);

// Board operations
Paginated_result<Board> get_boards_paginated(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
    const std::string& api_path,
    const std::string& token,
    const Pagination_params& pagination = {});



} // namespace kaiten

#endif // KAITEN_HPP
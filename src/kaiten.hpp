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
std::pair<int, Card> create_card(
    Http_client& client,
    const std::string& host,
    const std::string& api_path,
    const std::string& token,
    const Simple_card& desired);

std::pair<int, Card> update_card(
    Http_client& client,
    const std::string& host,
    const std::string& api_path,
    const std::string& token,
    const std::string& id_or_number,
    const Simple_card& changes);

std::pair<int, Card> get_card(
    Http_client& client,
    const std::string& host,
    const std::string& api_path,
    const std::string& token,
    const std::string& id_or_number);

// Advanced paginated card operations
PaginatedResult<Card> get_cards_paginated(
    Http_client& client,
    const std::string& host,
    const std::string& api_path,
    const std::string& token,
    const PaginationParams& pagination = {},
    const CardFilterParams& filters = {});

std::pair<int, std::vector<Card>> get_all_cards(
    Http_client& client,
    const std::string& host,
    const std::string& api_path,
    const std::string& token,
    const CardFilterParams& filters = {},
    int page_size = 100);

// User operations
std::pair<int, User> get_user(
    Http_client& client,
    const std::string& host,
    const std::string& api_path,
    const std::string& token,
    std::int64_t user_id);

PaginatedResult<User> get_users_paginated(
    Http_client& client,
    const std::string& host,
    const std::string& api_path,
    const std::string& token,
    const PaginationParams& pagination = {},
    const UserFilterParams& filters = {});

std::pair<int, std::vector<User>> get_all_users(
    Http_client& client,
    const std::string& host,
    const std::string& api_path,
    const std::string& token,
    const UserFilterParams& filters = {},
    int page_size = 100);

// Board operations
PaginatedResult<Board> get_boards_paginated(
    Http_client& client,
    const std::string& host,
    const std::string& api_path,
    const std::string& token,
    const PaginationParams& pagination = {});

// Utility functions for automatic pagination
template<typename T, typename Func>
PaginatedResult<T> fetch_all_pages(
    Http_client& client,
    const std::string& host,
    const std::string& api_path,
    const std::string& token,
    const std::string& endpoint,
    Func parser,
    const PaginationParams& initial_pagination = {})
{
    PaginatedResult<T> result;
    PaginationParams pagination = initial_pagination;
    
    do {
        auto page_result = fetch_single_page<T>(client, host, api_path, token, 
                                               endpoint, parser, pagination);
        
        if (page_result.items.empty()) {
            break;
        }
        
        result.items.insert(result.items.end(), 
                          page_result.items.begin(), 
                          page_result.items.end());
        result.total_count += page_result.items.size();
        
        // Обновляем метаданные из первой страницы
        if (pagination.page == 1) {
            result.per_page = page_result.per_page;
            result.total_pages = page_result.total_pages;
        }
        
        pagination.page++;
        result.has_more = page_result.has_more;
        
    } while (result.has_more && pagination.page <= result.total_pages);
    
    return result;
}

} // namespace kaiten

#endif // KAITEN_HPP
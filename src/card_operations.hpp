#ifndef CARD_OPERATIONS_HPP
#define CARD_OPERATIONS_HPP

#include <string>
#include <vector>
#include <utility>

#include "card.hpp"
#include "http_client.hpp"
#include "pagination.hpp"

namespace kaiten {
namespace card_operations {

// Basic card operations
std::pair<int, Card> create_card(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
    const std::string& api_path,
    const std::string& token,
    const Simple_card& desired);

std::pair<int, Card> update_card(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
    const std::string& api_path,
    const std::string& token,
    const std::string& id_or_number,
    const Simple_card& changes);

std::pair<int, Card> get_card(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
    const std::string& api_path,
    const std::string& token,
    const std::string& id_or_number);

// Card relationships
// Add an existing card as a child to a parent card
// POST { "card_id": <child_id> } to /cards/{parent_id}/children
std::pair<int, bool> add_child_card(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
    const std::string& api_path,
    const std::string& token,
    std::int64_t parent_card_id,
    std::int64_t child_card_id);

// Add tag to card
std::pair<int, bool> add_tag_to_card(
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

std::pair<int, std::vector<Card>> get_all_cards(
    Http_client& client,
    const std::string& host,
    const std::string& port,
    const std::string& api_path,
    const std::string& token,
    const Card_filter_params& filters = {},
    int page_size = 100);

// Batch-based implementation of get_all_cards using limited threading
std::pair<int, std::vector<Card>> get_all_cards_batched(
    Http_client& client,
    const std::string& host,
    const std::string& port,
    const std::string& api_path,
    const std::string& token,
    const Card_filter_params& filters = {},
    int page_size = 100);

} // namespace card_operations
} // namespace kaiten

#endif // CARD_OPERATIONS_HPP
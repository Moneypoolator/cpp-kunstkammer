#include "card_operations.hpp"

#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <future>
#include <nlohmann/json.hpp>

#include "cache.hpp"
#include "card.hpp"
#include "card_utils.hpp"
#include "http_client.hpp"
#include "pagination.hpp"
#include "error_handler.hpp"
#include "api_utils.hpp"

namespace kaiten {
namespace card_operations {


// Creates a new card using the Kaiten API.
// Required fields: title, column_id, lane_id.
// Optional fields used if present: type, size, tags.
// Returns: { http_status, Card (parsed if success, otherwise default) }
std::pair<int, Card> create_card(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
    const std::string& api_path,
    const std::string& token,
    const Simple_card& desired)
{
    std::string target = api_path + "/cards";

    nlohmann::json body = {
        { "title", desired.title },
        { "board_id", desired.board_id },
        { "column_id", desired.column_id },
        { "lane_id", desired.lane_id }
    };
    if (desired.type_id > 0) {
        body["type_id"] = desired.type_id;
    }
    if (desired.responsible_id > 0) {
        body["responsible_id"] = desired.responsible_id;
    }
    if (desired.size != 0) {
        std::ostringstream oss;
        oss << desired.size << " ч";
        body["size_text"] = oss.str();
    }
    if (!desired.properties.empty()) {
        nlohmann::json props_json = nlohmann::json::object();
        for (const auto& [key, value] : desired.properties) {
            props_json[key] = property_value_to_json(value);
        }
        body["properties"] = props_json;
    }

    std::cout << "Creating card with payload: " << body.dump(2) << std::endl;

    auto [status, response] = client.post(host, port, target, body.dump(), token);
    if (status == 200 || status == 201) {
        try {
            auto j = nlohmann::json::parse(response);
            Card created = j.get<Card>();
            return { status, created };
        } catch (const std::exception& e) {
            auto error = kaiten::error_handler::handle_parsing_error(e, "card creation response", response);
            kaiten::error_handler::log_error(error, "create_card");
            return { status, Card {} };
        } catch (...) {
            auto error = kaiten::error_handler::ErrorInfo{
                kaiten::error_handler::ErrorCategory::PARSING,
                status,
                "Create card: success status but failed to parse response",
                "Unknown parsing error",
                "Check the API response format and try again",
                response
            };
            kaiten::error_handler::log_error(error, "create_card");
            return { status, Card {} };
        }
    }

    kaiten::api_utils::log_api_error("Create card failed", status, response);

    return { status, Card {} };
}

// Updates an existing card. Target can be by numeric id or card number (string).
// Only non-empty/meaningful fields are sent.
// Returns: { http_status, Card (parsed if success) }
std::pair<int, Card> update_card(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
    const std::string& api_path,
    const std::string& token,
    const std::string& id_or_number,
    const Simple_card& changes)
{
    std::string target = api_path + "/cards/" + id_or_number;

    nlohmann::json body = nlohmann::json::object();
    if (!changes.title.empty()) {
        body["title"] = changes.title;
    }

    auto [status, response] = client.patch(host, port, target, body.dump(), token);
    if (status == 200) {
        try {
            auto j = nlohmann::json::parse(response);
            Card updated = j.get<Card>();
            return { status, updated };
        } catch (const std::exception& e) {
            auto error = kaiten::error_handler::handle_parsing_error(e, "card update response", response);
            kaiten::error_handler::log_error(error, "update_card");
            return { status, Card {} };
        } catch (...) {
            auto error = kaiten::error_handler::ErrorInfo{
                kaiten::error_handler::ErrorCategory::PARSING,
                status,
                "Update card: success status but failed to parse response",
                "Unknown parsing error",
                "Check the API response format and try again",
                response
            };
            kaiten::error_handler::log_error(error, "update_card");
            return { status, Card {} };
        }
    }

    kaiten::api_utils::log_api_error("Update card failed", status, response);

    return { status, Card {} };
}

std::pair<int, Card> get_card(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
    const std::string& api_path,
    const std::string& token,
    const std::string& id_or_number)
{
    // Пробуем получить из кэша по номеру
    if (id_or_number.find("CARD-") == 0 || id_or_number.find("card-") == 0) {
        auto cached = Api_cache::card_number_cache().get(id_or_number);
        if (cached.has_value()) {
            std::cout << "Cache HIT for card number: " << id_or_number << std::endl;
            return { 200, cached.value() };
        }
    } else {
        // Пробуем получить из кэша по ID
        try {
            std::int64_t card_id = std::stoll(id_or_number);
            auto cached = Api_cache::card_cache().get(card_id);
            if (cached.has_value()) {
                std::cout << "Cache HIT for card ID: " << card_id << std::endl;
                return { 200, cached.value() };
            }
        } catch (...) {
            // Не число, продолжаем как строку
        }
    }

    std::cout << "Cache MISS for: " << id_or_number << std::endl;

    std::string target = api_path + "/cards/" + id_or_number;

    auto [status, response] = client.get(host, port, target, token);
    if (status == 200) {
        try {
            auto j = nlohmann::json::parse(response);
            Card card = j.get<Card>();

            // Сохраняем в кэш
            Api_cache::card_cache().put(card.id, card);
            Api_cache::card_number_cache().put(card.number, card);

            print_card_details(card, true);
            return { status, card };
        } catch (const std::exception& e) {
            auto error = kaiten::error_handler::handle_parsing_error(e, "card JSON", response);
            kaiten::error_handler::log_error(error, "get_card");
            return { status, Card {} };
        } catch (...) {
            auto error = kaiten::error_handler::ErrorInfo{
                kaiten::error_handler::ErrorCategory::PARSING,
                status,
                "Failed to parse card JSON",
                "Unknown parsing error",
                "Check the API response format and try again",
                response
            };
            kaiten::error_handler::log_error(error, "get_card");
            return { status, Card {} };
        }
    }

    kaiten::api_utils::log_api_error("Get card failed", status, response);

    return { status, Card {} };
}

// Add child card relationship
std::pair<int, bool> add_child_card(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
    const std::string& api_path,
    const std::string& token,
    std::int64_t parent_card_id,
    std::int64_t child_card_id)
{
    std::string target = api_path + "/cards/" + std::to_string(parent_card_id) + "/children";
    nlohmann::json body = {
        {"card_id", child_card_id}
    };

    auto [status, response] = client.post(host, port, target, body.dump(), token);

    if (status == 200 || status == 201) {
        return { status, true };
    }

    kaiten::api_utils::log_api_error("Add child failed", status, response);

    return { status, false };
}

// Add tag to card
std::pair<int, bool> add_tag_to_card(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
    const std::string& api_path,
    const std::string& token,
    std::int64_t card_id,
    const std::string& tag)
{
    std::string target = api_path + "/cards/" + std::to_string(card_id) + "/tags";
    nlohmann::json body = {
        {"name", tag}
    };

    auto [status, response] = client.post(host, port, target, body.dump(), token);

    if (status == 200 || status == 201) {
        return { status, true };
    }

    kaiten::api_utils::log_api_error("Add tag failed", status, response);

    return { status, false };
}

// Кэширование для списков с ключом на основе параметров
std::string generate_cache_key(const std::string& endpoint,
    const Pagination_params& pagination,
    const Card_filter_params& filters)
{
    std::stringstream key;
    key << endpoint << "_page_" << pagination.page() << "_size_" << pagination.per_page();

    // Добавляем фильтры в ключ
    if (filters.board_id) {
        key << "_b" << *filters.board_id;
    }
    if (filters.lane_id) {
        key << "_l" << *filters.lane_id;
    }
    if (filters.column_id) {
        key << "_c" << *filters.column_id;
    }
    if (filters.state) {
        key << "_s" << *filters.state;
    }
    if (filters.type_name) {
        key << "_t" << *filters.type_name;
    }

    return key.str();
}

// Improved paginated cards with correct Kaiten API pagination
Paginated_result<Card> get_cards_paginated(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
    const std::string& api_path,
    const std::string& token,
    const Pagination_params& pagination,
    const Card_filter_params& filters)
{
    Paginated_result<Card> result;
    
    // Убедимся, что limit не превышает максимум Kaiten API
    Pagination_params safe_pagination = pagination;
    if (safe_pagination.limit > 100) {
        safe_pagination.limit = 100;
        std::cout << "Warning: Kaiten API limit max is 100, using limit=100" << std::endl;
    }
    
    std::string query = Query_builder::build(safe_pagination, filters);
    std::string target = api_path + "/cards" + query;
    
    std::cout << "API Request: " << target << std::endl;
    
    auto [status, response] = client.get(host, port, target, token);
    
    if (status != 200) {
        auto error = kaiten::error_handler::handle_http_error(status, response, "fetch cards");
        kaiten::error_handler::log_error(error, "get_cards_paginated");
        return result;
    }
    
    try {
        auto json = nlohmann::json::parse(response);
        
        // Парсим элементы
        if (json.is_array()) {
            result.items = json.get<std::vector<Card>>();
        } else {
            auto error = kaiten::error_handler::ErrorInfo{
                kaiten::error_handler::ErrorCategory::API,
                status,
                "Unexpected response format for cards",
                "Response is not an array",
                "Check the API response format and try again",
                json.dump(2)
            };
            kaiten::error_handler::log_error(error, "get_cards_paginated");
            return result;
        }
        
        // Устанавливаем пагинационные данные
        result.limit = safe_pagination.limit;
        result.offset = safe_pagination.offset;
        result.total_count = static_cast<int>(result.items.size()); // Временное значение
        
        // Определяем, есть ли следующая страница
        result.has_more = result.items.size() == static_cast<size_t>(safe_pagination.limit);
        
        std::cout << "Fetched " << result.items.size() << " cards "
                  << "(offset=" << result.offset << ", limit=" << result.limit << ")"
                  << ", has_more: " << (result.has_more ? "yes" : "no") << std::endl;
        
    } catch (const std::exception& e) {
        auto error = kaiten::error_handler::handle_parsing_error(e, "cards paginated response", response);
        kaiten::error_handler::log_error(error, "get_cards_paginated");
    }
    
    return result;
}

// Helper function to fetch a single page (used in map phase)
namespace {
std::vector<Card> fetch_page(
    Http_client& client,
    const std::string& host,
    const std::string& port,
    const std::string& api_path,
    const std::string& token,
    const Card_filter_params& filters,
    int offset,
    int limit)
{
    Pagination_params pagination;
    pagination.offset = offset;
    pagination.limit = limit;
    
    auto page_result = get_cards_paginated(client, host, port, api_path, token, 
                                          pagination, filters);
    
    std::cout << "Thread: Fetched page at offset " << offset 
              << ": " << page_result.items.size() << " cards" << std::endl;
    
    return page_result.items;
}
} // namespace

// Get all cards with automatic pagination using offset/limit
// Map-Reduce implementation: Each page is fetched in a separate thread
std::pair<int, std::vector<Card>> get_all_cards(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
    const std::string& api_path,
    const std::string& token,
    const Card_filter_params& filters,
    int page_size)
{
    int last_status = 200;
    
    Pagination_params pagination;
    pagination.limit = (page_size > 100) ? 100 : page_size; // Kaiten API max limit is 100
    
    std::cout << "Starting multithreaded fetch of all cards using map-reduce approach..." << std::endl;
    
    // MAP PHASE: Discover pages and spawn threads for parallel fetching
    // First, fetch the first page to determine if there are more pages
    std::cout << "Fetching first page to determine total pages..." << std::endl;
    
    pagination.offset = 0;
    auto first_page_result = get_cards_paginated(client, host, port, api_path, token, 
                                                 pagination, filters);
    
    if (first_page_result.items.empty()) {
        std::cout << "No cards found." << std::endl;
        return {last_status, {}};
    }
    
    // Store first page results
    std::vector<Card> all_cards = first_page_result.items;
    std::cout << "Page 0 (offset 0): " << first_page_result.items.size() << " cards" << std::endl;
    
    // MAP PHASE: Spawn threads for remaining pages
    std::vector<std::future<std::vector<Card>>> futures;
    
    if (first_page_result.has_more) {
        // MAP PHASE: Spawn threads for remaining pages
        // Since we don't know the exact total count upfront, we use a reasonable estimate
        // Each page will be fetched in a separate thread (map phase)
        // Results will be combined in the reduce phase
        
        // Use a reasonable maximum number of pages to fetch
        // This prevents spawning too many threads while covering most use cases
        int max_pages = 500; // Maximum pages to fetch (safety limit)
        int current_offset = pagination.limit;
        
        std::cout << "Spawning threads for pages 1 to " << max_pages << "..." << std::endl;
        
        // MAP: Spawn threads for all pages (each page in a separate thread)
        for (int page = 1; page <= max_pages; ++page) {
            futures.push_back(
                std::async(std::launch::async, fetch_page,
                          std::ref(client), host, port, api_path, token,
                          std::ref(filters), current_offset, pagination.limit)
            );
            current_offset += pagination.limit;
        }
        
        std::cout << "Spawned " << futures.size() << " threads for parallel fetching" << std::endl;
    }
    
    // REDUCE PHASE: Collect results from all futures and combine
    std::cout << "Collecting results from " << futures.size() << " threads..." << std::endl;
    
    int pages_collected = 0;
    int empty_pages_found = 0;
    
    for (auto& future : futures) {
        try {
            auto page_cards = future.get();
            if (!page_cards.empty()) {
                all_cards.insert(all_cards.end(),
                               page_cards.begin(),
                               page_cards.end());
                pages_collected++;
            } else {
                empty_pages_found++;
                // If we find an empty page, we've likely reached the end
                // Continue collecting remaining pages but note that we're done
            }
        } catch (const std::exception& e) {
            auto error = kaiten::error_handler::handle_network_error(e, "fetching page");
            kaiten::error_handler::log_error(error, "get_all_cards");
        } catch (...) {
            auto error = kaiten::error_handler::ErrorInfo{
                kaiten::error_handler::ErrorCategory::NETWORK,
                0,
                "Unknown error fetching page",
                "Unknown error",
                "Check your network connection and try again",
                ""
            };
            kaiten::error_handler::log_error(error, "get_all_cards");
        }
    }
    
    std::cout << "Collected " << pages_collected << " non-empty pages, " 
              << empty_pages_found << " empty pages" << std::endl;
    std::cout << "Finished fetching cards. Total: " << all_cards.size() << std::endl;
    
    return {last_status, all_cards};
}

} // namespace card_operations
} // namespace kaiten
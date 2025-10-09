#include <iostream>
#include <string>
#include <utility>
#include <nlohmann/json.hpp>

#include "kaiten.hpp"

#include "cache.hpp"
#include "card.hpp"
#include "card_utils.hpp"
#include "http_client.hpp"

namespace kaiten {

    // Временная функция для отладки структуры ответа
void debug_api_response(const std::string& response) {
    try {
        auto json = nlohmann::json::parse(response);
        std::cout << "=== API Response Structure ===" << std::endl;
        std::cout << "Is array: " << json.is_array() << std::endl;
        std::cout << "Keys: ";
        for (auto it = json.begin(); it != json.end(); ++it) {
            std::cout << it.key() << " ";
        }
        std::cout << std::endl;
        
        if (json.contains("pagination")) {
            std::cout << "Pagination: " << json["pagination"].dump(2) << std::endl;
        }
        if (json.contains("meta")) {
            std::cout << "Meta: " << json["meta"].dump(2) << std::endl;
        }
        if (json.contains("cards")) {
            std::cout << "Cards count: " << json["cards"].size() << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Debug parse error: " << e.what() << std::endl;
    }
}

// Creates a new card using the Kaiten API.
// Required fields: title, column_id, lane_id.
// Optional fields used if present: type, size, tags.
// Returns: { http_status, Card (parsed if success, otherwise default) }
std::pair<int, Card> create_card(
    Http_client& client,
    const std::string& host,
    const std::string& api_path,
    const std::string& token,
    const Simple_card& desired)
{
    std::string target = api_path + "/cards";

    nlohmann::json body = {
        { "title", desired.title },
        { "column_id", desired.column_id },
        { "lane_id", desired.lane_id }
    };
    if (!desired.type.empty()) {
        body["type"] = desired.type;
    }
    if (desired.size != 0) {
        body["size"] = desired.size;
    }
    if (!desired.tags.empty()) {
        body["tags"] = desired.tags;
    }

    auto [status, response] = client.post(host, "443", target, body.dump(), token);
    if (status == 200 || status == 201) {
        try {
            auto j = nlohmann::json::parse(response);
            Card created = j.get<Card>();
            return { status, created };
        } catch (...) {
            std::cerr << "Create card: success status but failed to parse response." << std::endl;
            return { status, Card {} };
        }
    }

    // Handle known error statuses with informative messages
    switch (status) {
        case 400:
            std::cerr << "Create card failed: 400 Bad Request. Check required fields (title, column_id, lane_id) and payload format." << std::endl;
            break;
        case 401:
            std::cerr << "Create card failed: 401 Unauthorized. Check Bearer token." << std::endl;
            break;
        case 402:
            std::cerr << "Create card failed: 402 Payment Required (plan/limits)." << std::endl;
            break;
        case 403:
            std::cerr << "Create card failed: 403 Forbidden. Insufficient permissions for board/column/lane." << std::endl;
            break;
        case 404:
            std::cerr << "Create card failed: 404 Not Found. Verify API path or IDs (column/lane)." << std::endl;
            break;
        default:
            std::cerr << "Create card failed: Unsupported status " << status << "." << std::endl;
            break;
    }

    // Try to print error details from JSON if available
    try {
        auto err = nlohmann::json::parse(response);
        if (err.is_object()) {
            if (err.contains("message")) {
                std::cerr << "Error message: " << err["message"].dump() << std::endl;
            }
            if (err.contains("errors")) {
                std::cerr << "Errors: " << err["errors"].dump() << std::endl;
            }
        }
    } catch (...) {
        if (!response.empty()) {
            std::cerr << "Error body: " << response << std::endl;
        }
    }

    return { status, Card {} };
}

// Updates an existing card. Target can be by numeric id or card number (string).
// Only non-empty/meaningful fields are sent.
// Returns: { http_status, Card (parsed if success) }
std::pair<int, Card> update_card(
    Http_client& client,
    const std::string& host,
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
    if (!changes.type.empty()) {
        body["type"] = changes.type;
    }
    if (changes.size != 0) {
        body["size"] = changes.size;
    }
    if (changes.column_id != 0) {
        body["column_id"] = changes.column_id;
    }
    if (changes.lane_id != 0) {
        body["lane_id"] = changes.lane_id;
    }
    if (!changes.tags.empty()) {
        body["tags"] = changes.tags;
    }

    auto [status, response] = client.patch(host, "443", target, body.dump(), token);
    if (status == 200) {
        try {
            auto j = nlohmann::json::parse(response);
            Card updated = j.get<Card>();
            return { status, updated };
        } catch (...) {
            std::cerr << "Update card: success status but failed to parse response." << std::endl;
            return { status, Card {} };
        }
    }

    switch (status) {
        case 400:
            std::cerr << "Update card failed: 400 Bad Request. Check payload fields and constraints." << std::endl;
            break;
        case 401:
            std::cerr << "Update card failed: 401 Unauthorized. Check Bearer token." << std::endl;
            break;
        case 402:
            std::cerr << "Update card failed: 402 Payment Required (plan/limits)." << std::endl;
            break;
        case 403:
            std::cerr << "Update card failed: 403 Forbidden. Insufficient permissions." << std::endl;
            break;
        case 404:
            std::cerr << "Update card failed: 404 Not Found. Verify card id/number and path." << std::endl;
            break;
        default:
            std::cerr << "Update card failed: Unsupported status " << status << "." << std::endl;
            break;
    }

    try {
        auto err = nlohmann::json::parse(response);
        if (err.is_object()) {
            if (err.contains("message")) {
                std::cerr << "Error message: " << err["message"].dump() << std::endl;
            }
            if (err.contains("errors")) {
                std::cerr << "Errors: " << err["errors"].dump() << std::endl;
            }
        }
    } catch (...) {
        if (!response.empty()) {
            std::cerr << "Error body: " << response << std::endl;
        }
    }

    return { status, Card {} };
}

// kaiten.hpp - добавить после update_card
std::pair<int, Card> get_card(
    Http_client& client,
    const std::string& host,
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

    auto [status, response] = client.get(host, "443", target, token);
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
            std::cerr << "Failed to parse card JSON: " << e.what() << std::endl;
            std::cerr << "Raw response: " << response << std::endl;
            return { status, Card {} };
        }
    }

    // Обработка ошибок
    switch (status) {
        case 401:
            std::cerr << "Get card failed: 401 Unauthorized. Check Bearer token." << std::endl;
            break;
        case 403:
            std::cerr << "Get card failed: 403 Forbidden. Insufficient permissions." << std::endl;
            break;
        case 404:
            std::cerr << "Get card failed: 404 Not Found. Card not found." << std::endl;
            break;
        default:
            std::cerr << "Get card failed: Status " << status << "." << std::endl;
            break;
    }

    return { status, Card {} };
}

// Gets a specific user by ID
std::pair<int, User> get_user(
    Http_client& client,
    const std::string& host,
    const std::string& api_path,
    const std::string& token,
    std::int64_t user_id)
{
    // Пробуем получить из кэша
    auto cached = Api_cache::user_cache().get(user_id);
    if (cached.has_value()) {
        std::cout << "Cache HIT for user ID: " << user_id << std::endl;
        return { 200, cached.value() };
    }

    std::cout << "Cache MISS for user ID: " << user_id << std::endl;

    std::string target = api_path + "/users/" + std::to_string(user_id);

    auto [status, response] = client.get(host, "443", target, token);
    if (status == 200) {
        try {
            auto j = nlohmann::json::parse(response);
            User user = j.get<User>();

            // Сохраняем в кэш
            Api_cache::user_cache().put(user.id, user);

            return { status, user };
        } catch (const std::exception& e) {
            std::cerr << "Failed to parse user JSON: " << e.what() << std::endl;
            std::cerr << "Raw response: " << response << std::endl;
            return { status, User {} };
        }
    }

    // Обработка ошибок
    switch (status) {
        case 401:
            std::cerr << "Get user failed: 401 Unauthorized. Check Bearer token." << std::endl;
            break;
        case 403:
            std::cerr << "Get user failed: 403 Forbidden. Insufficient permissions." << std::endl;
            break;
        case 404:
            std::cerr << "Get user failed: 404 Not Found. User not found." << std::endl;
            break;
        default:
            std::cerr << "Get user failed: Status " << status << "." << std::endl;
            break;
    }

    return { status, User {} };
}

// Кэширование для списков с ключом на основе параметров
std::string generate_cache_key(const std::string& endpoint,
    const Pagination_params& pagination,
    const Card_filter_params& filters)
{
    std::stringstream key;
    key << endpoint << "_page_" << pagination.page << "_size_" << pagination.per_page;

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

// Improved paginated cards with metadata support
Paginated_result<Card> get_cards_paginated_0(
    Http_client& client,
    const std::string& host,
    const std::string& api_path,
    const std::string& token,
    const Pagination_params& pagination,
    const Card_filter_params& filters)
{
    // Генерируем ключ кэша
    std::string cache_key = generate_cache_key("cards", pagination, filters);
    
    // Пробуем получить из кэша списков
    auto cached = Api_cache::list_cache().get(cache_key);
    if (cached.has_value()) {
        std::cout << "Cache HIT for cards list: " << cache_key << std::endl;
        try {
            Paginated_result<Card> result;
            result.items = cached->get<std::vector<Card>>();
            // Для списков не кэшируем полные метаданные пагинации
            result.page = pagination.page;
            result.per_page = pagination.per_page;
            result.total_count = result.items.size();
            result.has_more = result.items.size() == static_cast<size_t>(pagination.per_page);
            return result;
        } catch (...) {
            // В случае ошибки парсинга продолжаем обычным путем
        }
    }
    
    std::cout << "Cache MISS for cards list: " << cache_key << std::endl;

    Paginated_result<Card> result;

    std::string query = Query_builder::build(pagination, filters);
    std::string target = api_path + "/cards" + query;

    auto [status, response] = client.get(host, "443", target, token);

    if (status != 200) {
        std::cerr << "Failed to fetch cards page " << pagination.page
                  << ". Status: " << status << std::endl;
        return result;
    }

    try {
        auto json = nlohmann::json::parse(response);

        // Парсим элементы
        if (json.is_array()) {
            result.items = json.get<std::vector<Card>>();
        } else if (json.contains("cards") && json["cards"].is_array()) {
            result.items = json["cards"].get<std::vector<Card>>();
        } else {
            std::cerr << "Unexpected response format for cards" << std::endl;
            return result;
        }

        // Сохраняем в кэш списков
        nlohmann::json cache_data = result.items;
        Api_cache::list_cache().put(cache_key, cache_data);
        
        // Кэшируем отдельные карточки
        for (const auto& card : result.items) {
            Api_cache::card_cache().put(card.id, card);
            Api_cache::card_number_cache().put(card.number, card);
        }

        // Парсим метаданные пагинации
        result.page = pagination.page;
        result.per_page = pagination.per_page;
        result.total_count = result.items.size(); // временно

        if (json.contains("pagination")) {
            const auto& pagination_info = json["pagination"];
            if (pagination_info.contains("total")) {
                result.total_count = pagination_info["total"].get<int>();
            }
            if (pagination_info.contains("per_page")) {
                result.per_page = pagination_info["per_page"].get<int>();
            }
            if (pagination_info.contains("page")) {
                result.page = pagination_info["page"].get<int>();
            }
            if (pagination_info.contains("pages")) {
                result.total_pages = pagination_info["pages"].get<int>();
            }
        }

        // Определяем, есть ли следующая страница
        result.has_more = result.items.size() == static_cast<size_t>(pagination.per_page) && result.page < result.total_pages;

    } catch (const std::exception& e) {
        std::cerr << "Failed to parse cards paginated response: " << e.what() << std::endl;
        std::cerr << "Raw response: " << response << std::endl;
    }

    return result;
}

// Get all cards with automatic pagination
std::pair<int, std::vector<Card>> get_all_cards_0(
    Http_client& client,
    const std::string& host,
    const std::string& api_path,
    const std::string& token,
    const Card_filter_params& filters,
    int page_size)
{
    std::vector<Card> all_cards;
    int last_status = 200;

    Pagination_params pagination;
    pagination.per_page = page_size;
    pagination.page = 1;

    int total_pages_fetched = 0;
    int max_pages = 1000; // защита от бесконечного цикла

    while (total_pages_fetched < max_pages) {
        auto page_result = get_cards_paginated(client, host, api_path, token,
            pagination, filters);

        if (page_result.items.empty()) {
            break;
        }

        all_cards.insert(all_cards.end(),
            page_result.items.begin(),
            page_result.items.end());

        std::cout << "Fetched page " << pagination.page
                  << " (" << page_result.items.size() << " cards)"
                  << ", total: " << all_cards.size() << std::endl;

        if (!page_result.has_more) {
            break;
        }

        pagination.page++;
        total_pages_fetched++;
    }

    std::cout << "Total cards fetched: " << all_cards.size() << std::endl;
    return { last_status, all_cards };
}

// Improved paginated cards with better metadata handling
Paginated_result<Card> get_cards_paginated(
    Http_client& client,
    const std::string& host,
    const std::string& api_path,
    const std::string& token,
    const Pagination_params& pagination,
    const Card_filter_params& filters)
{
    Paginated_result<Card> result;
    
    std::string query = Query_builder::build(pagination, filters);
    std::string target = api_path + "/cards" + query;
    
    std::cout << "API Request: " << target << std::endl;
    
    auto [status, response] = client.get(host, "443", target, token);
    
    if (status != 200) {
        std::cerr << "Failed to fetch cards page " << pagination.page 
                  << ". Status: " << status << std::endl;
        if (!response.empty()) {
            std::cerr << "Response: " << response << std::endl;
        }
        return result;
    }
    
    try {
        auto json = nlohmann::json::parse(response);
        
        // Парсим элементы
        if (json.is_array()) {
            result.items = json.get<std::vector<Card>>();
        } else if (json.contains("cards") && json["cards"].is_array()) {
            result.items = json["cards"].get<std::vector<Card>>();
        } else if (json.contains("data") && json["data"].is_array()) {
            result.items = json["data"].get<std::vector<Card>>();
        } else {
            std::cerr << "Unexpected response format for cards" << std::endl;
            std::cerr << "Response: " << json.dump(2) << std::endl;
            return result;
        }
        
        // Парсим метаданные пагинации
        result.page = pagination.page;
        result.per_page = pagination.per_page;
        result.total_count = result.items.size(); // временно
        
        // Пытаемся извлечь метаданные пагинации из различных возможных мест
        if (json.contains("pagination")) {
            const auto& pagination_info = json["pagination"];
            if (pagination_info.contains("total")) {
                result.total_count = pagination_info["total"].get<int>();
            }
            if (pagination_info.contains("per_page")) {
                result.per_page = pagination_info["per_page"].get<int>();
            }
            if (pagination_info.contains("page")) {
                result.page = pagination_info["page"].get<int>();
            }
            if (pagination_info.contains("pages")) {
                result.total_pages = pagination_info["pages"].get<int>();
            }
            if (pagination_info.contains("has_more")) {
                result.has_more = pagination_info["has_more"].get<bool>();
            }
        } else if (json.contains("meta")) {
            const auto& meta_info = json["meta"];
            if (meta_info.contains("total")) {
                result.total_count = meta_info["total"].get<int>();
            }
            if (meta_info.contains("per_page")) {
                result.per_page = meta_info["per_page"].get<int>();
            }
            if (meta_info.contains("current_page")) {
                result.page = meta_info["current_page"].get<int>();
            }
            if (meta_info.contains("last_page")) {
                result.total_pages = meta_info["last_page"].get<int>();
            }
        }
        
        // Если не нашли метаданные, вычисляем has_more на основе размера страницы
        if (result.total_pages == 0) {
            result.total_pages = result.page; // временно
            result.has_more = result.items.size() == static_cast<size_t>(pagination.per_page);
        }
        
        std::cout << "Page " << result.page << "/" << result.total_pages 
                  << ", items: " << result.items.size() 
                  << ", per_page: " << result.per_page 
                  << ", has_more: " << (result.has_more ? "yes" : "no") << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse cards paginated response: " << e.what() << std::endl;
        std::cerr << "Raw response: " << response << std::endl;
    }
    
    return result;
}

// Get all cards with automatic pagination (улучшенная версия)
std::pair<int, std::vector<Card>> get_all_cards(
    Http_client& client,
    const std::string& host,
    const std::string& api_path,
    const std::string& token,
    const Card_filter_params& filters,
    int page_size)
{
    std::vector<Card> all_cards;
    int last_status = 200;
    
    Pagination_params pagination;
    pagination.per_page = page_size;
    pagination.page = 1;
    
    int total_pages_fetched = 0;
    int max_pages = 1000; // защита от бесконечного цикла
    
    std::cout << "Starting to fetch all cards..." << std::endl;
    
    while (total_pages_fetched < max_pages) {
        std::cout << "Fetching page " << pagination.page << "..." << std::endl;
        
        auto page_result = get_cards_paginated(client, host, api_path, token, 
                                             pagination, filters);
        
        if (page_result.items.empty()) {
            std::cout << "No cards found on page " << pagination.page << std::endl;
            break;
        }
        
        all_cards.insert(all_cards.end(), 
                        page_result.items.begin(), 
                        page_result.items.end());
        
        std::cout << "Page " << pagination.page 
                  << ": " << page_result.items.size() << " cards" 
                  << ", total: " << all_cards.size() << std::endl;
        
        // Проверяем условия завершения
        bool should_break = false;
        
        // Если API возвращает информацию о total_pages
        if (page_result.total_pages > 0 && pagination.page >= page_result.total_pages) {
            std::cout << "Reached last page (" << page_result.total_pages << ")" << std::endl;
            should_break = true;
        }
        // Если на странице меньше карточек, чем размер страницы - это последняя страница
        else if (page_result.items.size() < static_cast<size_t>(pagination.per_page)) {
            std::cout << "Last page detected (fewer items than page size)" << std::endl;
            should_break = true;
        }
        // Если флаг has_more установлен в false
        else if (!page_result.has_more) {
            std::cout << "No more pages available" << std::endl;
            should_break = true;
        }
        
        if (should_break) {
            break;
        }
        
        pagination.page++;
        total_pages_fetched++;
        
        // Задержка для соблюдения rate limits
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    std::cout << "Finished fetching cards. Total: " << all_cards.size() << std::endl;
    return {last_status, all_cards};
}


// Improved paginated users
Paginated_result<User> get_users_paginated(
    Http_client& client,
    const std::string& host,
    const std::string& api_path,
    const std::string& token,
    const Pagination_params& pagination,
    const User_filter_params& filters)
{
    Paginated_result<User> result;

    std::string query = Query_builder::build(pagination, filters);
    std::string target = api_path + "/users" + query;

    auto [status, response] = client.get(host, "443", target, token);

    if (status != 200) {
        std::cerr << "Failed to fetch users page " << pagination.page
                  << ". Status: " << status << std::endl;
        return result;
    }

    try {
        auto json = nlohmann::json::parse(response);

        if (json.is_array()) {
            result.items = json.get<std::vector<User>>();
        } else {
            std::cerr << "Unexpected response format for users" << std::endl;
            return result;
        }

        // Для users API может не возвращать метаданные пагинации
        result.page = pagination.page;
        result.per_page = pagination.per_page;
        result.total_count = result.items.size();
        result.has_more = result.items.size() == static_cast<size_t>(pagination.per_page);

    } catch (const std::exception& e) {
        std::cerr << "Failed to parse users paginated response: " << e.what() << std::endl;
        std::cerr << "Raw response: " << response << std::endl;
    }

    return result;
}

// Get all users with automatic pagination
std::pair<int, std::vector<User>> get_all_users(
    Http_client& client,
    const std::string& host,
    const std::string& api_path,
    const std::string& token,
    const User_filter_params& filters,
    int page_size)
{
    std::vector<User> all_users;
    int last_status = 200;

    Pagination_params pagination;
    pagination.per_page = page_size;
    pagination.page = 1;

    int total_pages_fetched = 0;
    int max_pages = 100;

    while (total_pages_fetched < max_pages) {
        auto page_result = get_users_paginated(client, host, api_path, token,
            pagination, filters);

        if (page_result.items.empty()) {
            break;
        }

        all_users.insert(all_users.end(),
            page_result.items.begin(),
            page_result.items.end());

        std::cout << "Fetched users page " << pagination.page
                  << " (" << page_result.items.size() << " users)"
                  << ", total: " << all_users.size() << std::endl;

        if (!page_result.has_more) {
            break;
        }

        pagination.page++;
        total_pages_fetched++;
    }

    std::cout << "Total users fetched: " << all_users.size() << std::endl;
    return { last_status, all_users };
}

// Boards pagination
Paginated_result<Board> get_boards_paginated(
    Http_client& client,
    const std::string& host,
    const std::string& api_path,
    const std::string& token,
    const Pagination_params& pagination)
{
    Paginated_result<Board> result;

    std::string query = Query_builder::build(pagination);
    std::string target = api_path + "/boards" + query;

    auto [status, response] = client.get(host, "443", target, token);

    if (status != 200) {
        std::cerr << "Failed to fetch boards page " << pagination.page
                  << ". Status: " << status << std::endl;
        return result;
    }

    try {
        auto json = nlohmann::json::parse(response);

        if (json.is_array()) {
            result.items = json.get<std::vector<Board>>();
        } else {
            std::cerr << "Unexpected response format for boards" << std::endl;
            return result;
        }

        result.page = pagination.page;
        result.per_page = pagination.per_page;
        result.total_count = result.items.size();
        result.has_more = result.items.size() == static_cast<size_t>(pagination.per_page);

    } catch (const std::exception& e) {
        std::cerr << "Failed to parse boards paginated response: " << e.what() << std::endl;
    }

    return result;
}

} // namespace kaiten

#include <iostream>
#include <string>
#include <utility>
#include <nlohmann/json.hpp>

#include "kaiten.hpp"

#include "cache.hpp"
#include "card.hpp"
#include "card_utils.hpp"
#include "http_client.hpp"
#include "pagination.hpp"

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
    // if (!desired.tags.empty()) {
    //     body["tags"] = desired.tags;
    // }
    if (!desired.properties.empty()) {
        body["properties"] = desired.properties;
    }

    std::cout << "Creating card with payload: " << body.dump(2) << std::endl;

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
    if (changes.board_id != 0) {
        body["board_id"] = changes.board_id;
    }
    if (changes.type_id > 0) {
        body["type_id"] = changes.type_id;
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
    if (!changes.properties.empty()) {
        body["properties"] = changes.properties;
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
    const std::string& space_id,
    std::int64_t user_id)
{
    // Пробуем получить из кэша
    auto cached = Api_cache::user_cache().get(user_id);
    if (cached.has_value()) {
        std::cout << "Cache HIT for user ID: " << user_id << std::endl;
        return { 200, cached.value() };
    }

    std::cout << "Cache MISS for user ID: " << user_id << std::endl;

    std::string target = api_path + "/spaces/" + space_id + "/users/" + std::to_string(user_id);

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

// Gets the current authenticated user
std::pair<int, User> get_current_user(
    Http_client& client,
    const std::string& host,
    const std::string& api_path,
    const std::string& token)
{
    std::string target = api_path + "/users/current";

    auto [status, response] = client.get(host, "443", target, token);
    if (status == 200) {
        try {
            auto j = nlohmann::json::parse(response);
            User user = j.get<User>();
            return { status, user };
        } catch (const std::exception& e) {
            std::cerr << "Failed to parse current user JSON: " << e.what() << std::endl;
            std::cerr << "Raw response: " << response << std::endl;
            return { status, User {} };
        }
    }

    switch (status) {
        case 401:
            std::cerr << "Get current user failed: 401 Unauthorized. Check Bearer token." << std::endl;
            break;
        case 403:
            std::cerr << "Get current user failed: 403 Forbidden." << std::endl;
            break;
        default:
            std::cerr << "Get current user failed: Status " << status << "." << std::endl;
            break;
    }

    return { status, User {} };
}

// Gets users by email (filter). Returns array; caller can decide how to handle multiple matches
std::pair<int, std::vector<User>> get_users_by_email(
    Http_client& client,
    const std::string& host,
    const std::string& api_path,
    const std::string& token,
    const std::string& email)
{
    std::string target = api_path + "/users?email=" + email; // assume email URL-safe; encode if needed

    auto [status, response] = client.get(host, "443", target, token);
    if (status == 200) {
        try {
            auto j = nlohmann::json::parse(response);
            std::vector<User> users;
            if (j.is_array()) {
                users = j.get<std::vector<User>>();
            } else if (j.is_object() && j.contains("users") && j["users"].is_array()) {
                users = j["users"].get<std::vector<User>>();
            }
            return { status, users };
        } catch (const std::exception& e) {
            std::cerr << "Failed to parse users-by-email JSON: " << e.what() << std::endl;
            std::cerr << "Raw response: " << response << std::endl;
            return { status, {} };
        }
    }

    switch (status) {
        case 401:
            std::cerr << "Get users by email failed: 401 Unauthorized." << std::endl;
            break;
        case 403:
            std::cerr << "Get users by email failed: 403 Forbidden." << std::endl;
            break;
        default:
            std::cerr << "Get users by email failed: Status " << status << "." << std::endl;
            break;
    }

    return { status, {} };
}

// Add child card relationship
std::pair<int, bool> add_child_card(
    Http_client& client,
    const std::string& host,
    const std::string& api_path,
    const std::string& token,
    std::int64_t parent_card_id,
    std::int64_t child_card_id)
{
    std::string target = api_path + "/cards/" + std::to_string(parent_card_id) + "/children";
    nlohmann::json body = {
        {"card_id", child_card_id}
    };

    auto [status, response] = client.post(host, "443", target, body.dump(), token);

    if (status == 200 || status == 201) {
        return { status, true };
    }

    switch (status) {
        case 400:
            std::cerr << "Add child failed: 400 Bad Request. Check parent/child IDs." << std::endl;
            break;
        case 401:
            std::cerr << "Add child failed: 401 Unauthorized. Check Bearer token." << std::endl;
            break;
        case 403:
            std::cerr << "Add child failed: 403 Forbidden. Insufficient permissions." << std::endl;
            break;
        case 404:
            std::cerr << "Add child failed: 404 Not Found. Verify parent/child exist." << std::endl;
            break;
        default:
            std::cerr << "Add child failed: Status " << status << "." << std::endl;
            break;
    }

    if (!response.empty()) {
        try {
            auto err = nlohmann::json::parse(response);
            if (err.contains("message")) {
                std::cerr << "Error message: " << err["message"].dump() << std::endl;
            }
            if (err.contains("errors")) {
                std::cerr << "Errors: " << err["errors"].dump() << std::endl;
            }
        } catch (...) {
            std::cerr << "Error body: " << response << std::endl;
        }
    }

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
    
    auto [status, response] = client.get(host, "443", target, token);
    
    if (status != 200) {
        std::cerr << "Failed to fetch cards. Status: " << status << std::endl;
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
        } else {
            std::cerr << "Unexpected response format for cards" << std::endl;
            std::cerr << "Response: " << json.dump(2) << std::endl;
            return result;
        }
        
        // Устанавливаем пагинационные данные
        result.limit = safe_pagination.limit;
        result.offset = safe_pagination.offset;
        result.total_count = result.items.size(); // Временное значение
        
        // Определяем, есть ли следующая страница
        result.has_more = result.items.size() == static_cast<size_t>(safe_pagination.limit);
        
        std::cout << "Fetched " << result.items.size() << " cards "
                  << "(offset=" << result.offset << ", limit=" << result.limit << ")"
                  << ", has_more: " << (result.has_more ? "yes" : "no") << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse cards paginated response: " << e.what() << std::endl;
        std::cerr << "Raw response: " << response << std::endl;
    }
    
    return result;
}

// Get all cards with automatic pagination using offset/limit
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
    pagination.limit = (page_size > 100) ? 100 : page_size; // Kaiten API max limit is 100
    
    int total_requests = 0;
    int max_requests = 1000; // защита от бесконечного цикла
    
    std::cout << "Starting to fetch all cards using offset/limit pagination..." << std::endl;
    
    while (total_requests < max_requests) {
        std::cout << "Fetching offset " << pagination.offset << ", limit " << pagination.limit << "..." << std::endl;
        
        auto page_result = get_cards_paginated(client, host, api_path, token, 
                                             pagination, filters);
        
        if (page_result.items.empty()) {
            std::cout << "No cards found at offset " << pagination.offset << std::endl;
            break;
        }
        
        all_cards.insert(all_cards.end(), 
                        page_result.items.begin(), 
                        page_result.items.end());
        
        std::cout << "Offset " << pagination.offset 
                  << ": " << page_result.items.size() << " cards" 
                  << ", total: " << all_cards.size() << std::endl;
        
        // Проверяем, есть ли следующая страница
        if (!page_result.has_more) {
            std::cout << "No more cards available" << std::endl;
            break;
        }
        
        // Увеличиваем offset для следующего запроса
        pagination.offset += pagination.limit;
        total_requests++;
        
        // Задержка для соблюдения rate limits
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    std::cout << "Finished fetching cards. Total: " << all_cards.size() << std::endl;
    return {last_status, all_cards};
}

// Improved paginated users with correct pagination
Paginated_result<User> get_users_paginated(
    Http_client& client,
    const std::string& host,
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

    auto [status, response] = client.get(host, "443", target, token);

    if (status != 200) {
        std::cerr << "Failed to fetch users. Status: " << status << std::endl;
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

        result.limit = safe_pagination.limit;
        result.offset = safe_pagination.offset;
        result.has_more = result.items.size() == static_cast<size_t>(safe_pagination.limit);

    } catch (const std::exception& e) {
        std::cerr << "Failed to parse users paginated response: " << e.what() << std::endl;
        std::cerr << "Raw response: " << response << std::endl;
    }

    return result;
}

// Boards pagination with correct implementation
Paginated_result<Board> get_boards_paginated(
    Http_client& client,
    const std::string& host,
    const std::string& api_path,
    const std::string& token,
    const Pagination_params& pagination)
{
    Paginated_result<Board> result;

    Pagination_params safe_pagination = pagination;
    if (safe_pagination.limit > 100) {
        safe_pagination.limit = 100;
    }

    std::string query = Query_builder::build(safe_pagination, Card_filter_params());
    std::string target = api_path + "/boards" + query;

    auto [status, response] = client.get(host, "443", target, token);

    if (status != 200) {
        std::cerr << "Failed to fetch boards. Status: " << status << std::endl;
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

        result.limit = safe_pagination.limit;
        result.offset = safe_pagination.offset;
        result.has_more = result.items.size() == static_cast<size_t>(safe_pagination.limit);

    } catch (const std::exception& e) {
        std::cerr << "Failed to parse boards paginated response: " << e.what() << std::endl;
    }

    return result;
}



} // namespace kaiten

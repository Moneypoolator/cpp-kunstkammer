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
            auto error = kaiten::error_handler::Error_info{
                kaiten::error_handler::Error_category::PARSING,
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
            auto error = kaiten::error_handler::Error_info{
                kaiten::error_handler::Error_category::PARSING,
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
            auto error = kaiten::error_handler::Error_info{
                kaiten::error_handler::Error_category::PARSING,
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
            auto error = kaiten::error_handler::Error_info{
                kaiten::error_handler::Error_category::API,
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
// This now uses the batched implementation for better resource management
std::pair<int, std::vector<Card>> get_all_cards(
    Http_client& client,
    const std::string& host,
    const std::string& port,
    const std::string& api_path,
    const std::string& token,
    const Card_filter_params& filters,
    int page_size)
{
    // Delegate to the batched implementation for better resource management
    return get_all_cards_batched(client, host, port, api_path, token, filters, page_size);
}

// Вспомогательная функция для получения одной страницы (используется в batch processing)
namespace {
kaiten::Paginated_result<Card> fetch_page_result(
    Http_client& client,
    const std::string& host,
    const std::string& port,
    const std::string& api_path,
    const std::string& token,
    const kaiten::Pagination_params& params)
{
    return get_cards_paginated(client, host, port, api_path, token, params);
}
} // namespace

// Get all cards with batch-based multithreading approach
// This implementation uses a batch processing model similar to handle_cards_list
std::pair<int, std::vector<Card>> get_all_cards_batched(
    Http_client& client,
    const std::string& host,
    const std::string& port,
    const std::string& api_path,
    const std::string& token,
    const Card_filter_params& filters,
    int page_size)
{
    int last_status = 200;
    
    Pagination_params params;
    params.limit = (page_size > 100) ? 100 : page_size; // Kaiten API max limit is 100
    params.offset = 0;
    
    std::vector<Card> all_cards;
    std::mutex cards_mutex; // Для потокобезопасного доступа к all_cards
    
    std::cout << "Starting batched fetch of all cards using offset/limit approach..." << std::endl;
    
    // Получаем первую страницу, чтобы определить, есть ли еще страницы
    std::cout << "Fetching first page (offset 0, limit " << params.limit << ")..." << std::endl;
    
    auto first_page_result = get_cards_paginated(client, host, port, api_path, token, params, filters);
    
    if (first_page_result.items.empty()) {
        std::cout << "No cards found." << std::endl;
        return {last_status, {}};
    }
    
    // Добавляем первую страницу в общий список
    all_cards.insert(all_cards.end(),
        first_page_result.items.begin(),
        first_page_result.items.end());
    
    std::cout << "Page 0 (offset 0): " << first_page_result.items.size()
              << " cards, total: " << all_cards.size() << std::endl;
    
    // Если есть еще страницы, начинаем batch processing
    if (first_page_result.has_more) {
        int current_offset = params.limit;
        bool has_more_pages = true;
        
        // Определяем количество доступных потоков с ограничением
        unsigned int max_threads = std::thread::hardware_concurrency();
        if (max_threads == 0) {
            max_threads = 4; // Fallback если hardware_concurrency() вернул 0
        }
        // Ограничиваем количество потоков для экономии ресурсов
        max_threads = std::min(max_threads, 6U); // Максимум 6 потоков
        std::cout << "Using " << max_threads << " threads for parallel fetching" << std::endl;
        
        while (has_more_pages) {
            // Создаем батч запросов на основе количества доступных потоков
            std::vector<std::future<kaiten::Paginated_result<Card>>> futures;
            std::vector<int> batch_offsets;
            
            // Генерируем запросы для текущего батча (на основе количества потоков)
            for (unsigned int i = 0; i < max_threads && has_more_pages; ++i) {
                kaiten::Pagination_params page_params = params;
                page_params.offset = current_offset;
                
                batch_offsets.push_back(current_offset);
                futures.push_back(
                    std::async(std::launch::async, fetch_page_result,
                              std::ref(client), host, port, api_path, token, page_params)
                );
                
                current_offset += params.limit;
            }
            
            if (!futures.empty()) {
                std::cout << "Processing batch of " << futures.size() << " pages (offsets starting from "
                          << batch_offsets[0] << ")..." << std::endl;
            }
            
            // Ожидаем завершения всех запросов в батче и обрабатываем результаты
            has_more_pages = false;
            for (size_t i = 0; i < futures.size(); ++i) {
                try {
                    auto page_result = futures[i].get();
                    
                    if (page_result.items.empty()) {
                        std::cout << "Page at offset " << batch_offsets[i] << ": empty, reached end" << std::endl;
                        // Если нашли пустую страницу, прекращаем дальнейшую обработку
                        has_more_pages = false;
                        break;
                    }
                    
                    // Потокобезопасное добавление карточек
                    {
                        std::lock_guard<std::mutex> lock(cards_mutex);
                        all_cards.insert(all_cards.end(),
                            page_result.items.begin(),
                            page_result.items.end());
                    }
                    
                    std::cout << "Page at offset " << batch_offsets[i] << ": " << page_result.items.size()
                              << " cards, total: " << all_cards.size() << std::endl;
                    
                    // Если хотя бы одна страница указывает, что есть еще страницы, продолжаем
                    if (page_result.has_more) {
                        has_more_pages = true;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error fetching page at offset " << batch_offsets[i] << ": " << e.what() << std::endl;
                    last_status = 500; // Internal server error
                }
            }
            
            // Если в батче не было страниц с has_more=true, прекращаем загрузку
            if (!has_more_pages) {
                std::cout << "No more pages available, stopping." << std::endl;
                break;
            }
        }
    }
    
    std::cout << "Finished fetching cards. Total: " << all_cards.size() << std::endl;
    
    return {last_status, all_cards};
}

} // namespace card_operations
} // namespace kaiten
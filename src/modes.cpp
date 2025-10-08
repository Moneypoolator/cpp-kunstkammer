#include "modes.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>

#include "card.hpp"
#include "kaiten.hpp"
#include "card_utils.hpp"
#include "pagination.hpp"

// Улучшенная функция пагинации с поддержкой всех параметров
namespace {

template <typename T, typename Client, typename Fetcher, typename Handler>
bool paginate_with_metadata(
    Client& client,
    const std::string& host,
    const std::string& api_path,
    const std::string& token,
    Fetcher fetcher,
    Handler handle_items,
    const kaiten::PaginationParams& initial_params = {},
    int max_pages = 1000)
{
    kaiten::PaginationParams params = initial_params;
    int pages_processed = 0;
    int total_items = 0;
    
    while (pages_processed < max_pages) {
        auto page_result = fetcher(client, host, api_path, token, params);
        
        if (page_result.items.empty()) {
            break;
        }
        
        // Обрабатываем элементы
        handle_items(page_result.items, page_result);
        
        total_items += page_result.items.size();
        std::cout << "Page " << params.page << ": " << page_result.items.size() 
                  << " items, total: " << total_items;
        
        if (page_result.total_count > 0) {
            std::cout << " / " << page_result.total_count;
        }
        std::cout << std::endl;
        
        // Проверяем, есть ли следующая страница
        if (!page_result.has_more || params.page >= page_result.total_pages) {
            break;
        }
        
        params.page++;
        pages_processed++;
    }
    
    std::cout << "Completed: " << total_items << " total items" << std::endl;
    return total_items > 0;
}

} // namespace

// Реализация handle_get_card
int handle_get_card(Http_client& client, const std::string& host,
    const std::string& api_path, const std::string& token,
    const std::string& card_number)
{
    std::cout << "Fetching card: " << card_number << std::endl;

    auto [status, card] = kaiten::get_card(client, host, api_path, token, card_number);

    if (status == 200) {
        std::cout << "\n=== Card Retrieved Successfully ===" << std::endl;
        std::cout << "Number: #" << card.number << std::endl;
        std::cout << "Title: " << card.title << std::endl;
        std::cout << "Type: " << card.type << std::endl;
        std::cout << "State: " << card.state << std::endl;
        std::cout << "Board: " << card.board.title << std::endl;
        std::cout << "Column: " << card.column.title << std::endl;
        std::cout << "Lane: " << card.lane.title << std::endl;
        std::cout << "Owner: " << card.owner.full_name << std::endl;
        return 0;
    } 
    
    std::cerr << "Failed to get card. Status: " << status << std::endl;
    return 1;
}

// Реализация handle_cards_list
int handle_cards_list(Http_client& client, const std::string& host, 
                     const std::string& api_path, const std::string& token)
{
    kaiten::PaginationParams params;
    params.per_page = 100;
    params.sort_by = "updated";
    params.sort_order = "desc";
    
    auto card_fetcher = [](Http_client& client, const std::string& host,
                          const std::string& api_path, const std::string& token,
                          const kaiten::PaginationParams& pagination) {
        return kaiten::get_cards_paginated(client, host, api_path, token, pagination);
    };
    
    auto card_handler = [](const std::vector<Card>& cards, 
                          const kaiten::PaginatedResult<Card>& result) {
        for (const auto& card : cards) {
            std::cout << "#" << card.number << " [" << card.id << "] " 
                      << card.title << " (" << card.type 
                      << ", size=" << card.size 
                      << ", updated: " << card.updated.toIso8601() 
                      << ", state: " << card.state << ")" << std::endl;
        }
    };
    
    std::cout << "Fetching cards with pagination..." << std::endl;
    return paginate_with_metadata<Card>(client, host, api_path, token, 
                                       card_fetcher, card_handler, params) ? 0 : 1;
}

// Реализация handle_cards_filter
int handle_cards_filter(Http_client& client, const std::string& host,
    const std::string& api_path, const std::string& token,
    const std::map<std::string, std::string>& filters)
{
    kaiten::CardFilterParams filter_params;
    kaiten::PaginationParams pagination;
    pagination.per_page = 100;
    
    // Преобразуем простые фильтры в структурированные
    for (const auto& [key, value] : filters) {
        if (key == "board_id") filter_params.board_id = std::stoll(value);
        else if (key == "lane_id") filter_params.lane_id = std::stoll(value);
        else if (key == "column_id") filter_params.column_id = std::stoll(value);
        else if (key == "owner_id") filter_params.owner_id = std::stoll(value);
        else if (key == "member_id") filter_params.member_id = std::stoll(value);
        else if (key == "type_id") filter_params.type_id = std::stoll(value);
        else if (key == "type") filter_params.type_name = value;
        else if (key == "state") filter_params.state = value;
        else if (key == "archived") filter_params.archived = (value == "true");
        else if (key == "blocked") filter_params.blocked = (value == "true");
        else if (key == "asap") filter_params.asap = (value == "true");
        else if (key == "search") filter_params.search = value;
        else if (key == "created_after") filter_params.created_after = value;
        else if (key == "created_before") filter_params.created_before = value;
        else if (key == "updated_after") filter_params.updated_after = value;
        else if (key == "updated_before") filter_params.updated_before = value;
        else filter_params.custom_filters[key] = value;
    }
    
    std::cout << "Fetching filtered cards with pagination..." << std::endl;
    std::cout << "Applied filters:" << std::endl;
    for (const auto& [key, value] : filters) {
        std::cout << "  " << key << ": " << value << std::endl;
    }
    
    auto [status, cards] = kaiten::get_all_cards(client, host, api_path, token, 
                                                filter_params, pagination.per_page);
    
    if (status == 200) {
        std::cout << "\n=== Filtered Cards Results ===" << std::endl;
        std::cout << "Total cards found: " << cards.size() << std::endl;
        
        // Группируем по типу для статистики
        std::map<std::string, int> type_stats;
        std::map<std::string, int> state_stats;
        
        for (const auto& card : cards) {
            type_stats[card.type]++;
            state_stats[card.state]++;
            
            std::cout << "#" << card.number << " [" << card.id << "] " 
                      << card.title << " (" << card.type 
                      << ", size=" << card.size 
                      << ", state=" << card.state 
                      << ", owner=" << card.owner.full_name << ")" << std::endl;
        }
        
        // Выводим статистику
        std::cout << "\n=== Statistics ===" << std::endl;
        std::cout << "By type:" << std::endl;
        for (const auto& [type, count] : type_stats) {
            std::cout << "  " << type << ": " << count << std::endl;
        }
        
        std::cout << "By state:" << std::endl;
        for (const auto& [state, count] : state_stats) {
            std::cout << "  " << state << ": " << count << std::endl;
        }
        
        return 0;
    }
    
    std::cerr << "Failed to get filtered cards. Status: " << status << std::endl;
    return 1;
}

// Реализация handle_users_list
int handle_users_list(Http_client& client, const std::string& host,
    const std::string& api_path, const std::string& token)
{
    kaiten::PaginationParams params;
    params.per_page = 100;
    
    auto user_fetcher = [](Http_client& client, const std::string& host,
                          const std::string& api_path, const std::string& token,
                          const kaiten::PaginationParams& pagination) {
        kaiten::UserFilterParams user_filters;
        return kaiten::get_users_paginated(client, host, api_path, token, pagination, user_filters);
    };
    
    auto user_handler = [](const std::vector<User>& users, 
                          const kaiten::PaginatedResult<User>& result) {
        for (const auto& user : users) {
            std::cout << "[" << user.id << "] " << user.full_name 
                      << " (" << user.email << ")" 
                      << " - " << user.username
                      << (user.activated ? " [ACTIVE]" : " [INACTIVE]")
                      << (user.virtual_user ? " [VIRTUAL]" : "")
                      << std::endl;
        }
    };
    
    std::cout << "Fetching users with pagination..." << std::endl;
    return paginate_with_metadata<User>(client, host, api_path, token, 
                                       user_fetcher, user_handler, params) ? 0 : 1;
}

// Реализация handle_get_user
int handle_get_user(Http_client& client, const std::string& host,
    const std::string& api_path, const std::string& token,
    const std::string& user_id)
{
    try {
        std::int64_t user_id_num = std::stoll(user_id);
        auto [status, user] = kaiten::get_user(client, host, api_path, token, user_id_num);
        
        if (status == 200) {
            std::cout << "\n=== User Details ===" << std::endl;
            std::cout << "ID: " << user.id << std::endl;
            std::cout << "UID: " << user.uid << std::endl;
            std::cout << "Full Name: " << user.full_name << std::endl;
            std::cout << "Email: " << user.email << std::endl;
            std::cout << "Username: " << user.username << std::endl;
            std::cout << "Avatar Type: " << user.avatar_type << std::endl;
            if (!user.avatar_uploaded_url.empty()) {
                std::cout << "Avatar URL: " << user.avatar_uploaded_url << std::endl;
            }
            std::cout << "Theme: " << user.theme << std::endl;
            std::cout << "Language: " << user.lng << std::endl;
            std::cout << "Timezone: " << user.timezone << std::endl;
            std::cout << "UI Version: " << user.ui_version << std::endl;
            std::cout << "Activated: " << (user.activated ? "Yes" : "No") << std::endl;
            std::cout << "Virtual User: " << (user.virtual_user ? "Yes" : "No") << std::endl;
            std::cout << "Created: " << user.created.toIso8601() << std::endl;
            std::cout << "Updated: " << user.updated.toIso8601() << std::endl;
            return 0;
        }
        
        std::cerr << "Failed to get user. Status: " << status << std::endl;
        return 1;
        
    } catch (const std::exception& e) {
        std::cerr << "Invalid user ID: " << user_id << " - " << e.what() << std::endl;
        return 1;
    }
}

// Реализация handle_tasks
int handle_tasks(Http_client& client, const std::string& host, const std::string& api_path, 
                const std::string& token, const Config& config, const std::string& tasks_file_path)
{
    std::ifstream tasks_file(tasks_file_path);
    if (!tasks_file) {
        std::cerr << "Could not open tasks file: " << tasks_file_path << std::endl;
        return 1;
    }
    
    nlohmann::json tasks_json;
    try {
        tasks_file >> tasks_json;
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse tasks JSON: " << e.what() << std::endl;
        return 1;
    }
    
    // Поддерживаем различные форматы tasks JSON
    nlohmann::json tasks_array;
    if (tasks_json.contains("schedule") && tasks_json["schedule"].contains("tasks")) {
        tasks_array = tasks_json["schedule"]["tasks"];
    } else if (tasks_json.contains("tasks") && tasks_json["tasks"].is_array()) {
        tasks_array = tasks_json["tasks"];
    } else if (tasks_json.is_array()) {
        tasks_array = tasks_json;
    } else {
        std::cerr << "Invalid tasks JSON structure. Expected array or object with 'tasks' or 'schedule.tasks' field." << std::endl;
        return 1;
    }
    
    if (!tasks_array.is_array()) {
        std::cerr << "Tasks data is not an array." << std::endl;
        return 1;
    }
    
    std::cout << "Found " << tasks_array.size() << " tasks to process" << std::endl;
    
    int success_count = 0;
    int error_count = 0;
    
    for (size_t i = 0; i < tasks_array.size(); ++i) {
        const auto& task = tasks_array[i];
        
        Simple_card desired;
        desired.title = task.value("title", "");
        desired.column_id = std::stoll(config.columnId);
        desired.lane_id = std::stoll(config.laneId);
        desired.type = task.value("type", "");
        desired.size = task.value("size", 0);
        
        // Обработка тегов
        if (task.contains("tags") && task["tags"].is_array()) {
            for (const auto& tag : task["tags"]) {
                if (tag.is_string()) {
                    desired.tags.push_back(tag.get<std::string>());
                }
            }
        }
        
        // Добавляем теги из конфигурации
        if (!config.tags.empty()) {
            desired.tags.insert(desired.tags.end(), config.tags.begin(), config.tags.end());
        }
        
        // Убираем дубликаты тегов
        std::sort(desired.tags.begin(), desired.tags.end());
        desired.tags.erase(std::unique(desired.tags.begin(), desired.tags.end()), desired.tags.end());
        
        if (desired.title.empty()) {
            std::cerr << "Task " << (i + 1) << " has empty title, skipping" << std::endl;
            error_count++;
            continue;
        }
        
        std::cout << "Creating card " << (i + 1) << "/" << tasks_array.size() 
                  << ": '" << desired.title << "'" 
                  << " (type: " << desired.type 
                  << ", size: " << desired.size 
                  << ", tags: " << desired.tags.size() << ")" << std::endl;
        
        auto [status, created] = kaiten::create_card(client, host, api_path, token, desired);
        
        if (status == 200 || status == 201) {
            std::cout << "✓ Created card #" << created.number << " [" << created.id << "] '" << created.title << "'" << std::endl;
            success_count++;
        } else {
            std::cerr << "✗ Failed to create card. Status: " << status << std::endl;
            error_count++;
        }
        
        // Небольшая задержка между созданиями карточек
        if (i < tasks_array.size() - 1) {
            //std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
    
    std::cout << "\n=== Tasks Processing Complete ===" << std::endl;
    std::cout << "Success: " << success_count << std::endl;
    std::cout << "Errors: " << error_count << std::endl;
    std::cout << "Total: " << tasks_array.size() << std::endl;
    
    return error_count > 0 ? 1 : 0;
}

// Реализация handle_create_card
int handle_create_card(Http_client& client, const std::string& host, const std::string& api_path, 
                      const std::string& token, const Config& config, const std::string& title, 
                      const std::string& type, int size, const std::vector<std::string>& tags)
{
    if (title.empty()) {
        std::cerr << "Error: Card title cannot be empty" << std::endl;
        return 1;
    }
    
    Simple_card desired;
    desired.title = title;
    desired.column_id = std::stoll(config.columnId);
    desired.lane_id = std::stoll(config.laneId);
    desired.type = type;
    desired.size = size;
    desired.tags = tags;
    
    // Добавляем теги из конфигурации
    if (!config.tags.empty()) {
        desired.tags.insert(desired.tags.end(), config.tags.begin(), config.tags.end());
    }
    
    // Убираем дубликаты тегов
    std::sort(desired.tags.begin(), desired.tags.end());
    desired.tags.erase(std::unique(desired.tags.begin(), desired.tags.end()), desired.tags.end());
    
    std::cout << "Creating single card..." << std::endl;
    std::cout << "Title: " << desired.title << std::endl;
    std::cout << "Column ID: " << desired.column_id << std::endl;
    std::cout << "Lane ID: " << desired.lane_id << std::endl;
    std::cout << "Type: " << desired.type << std::endl;
    std::cout << "Size: " << desired.size << std::endl;
    std::cout << "Tags: ";
    for (size_t i = 0; i < desired.tags.size(); ++i) {
        std::cout << desired.tags[i];
        if (i < desired.tags.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
    
    auto [status, created] = kaiten::create_card(client, host, api_path, token, desired);
    
    if (status == 200 || status == 201) {
        std::cout << "\n✓ Card created successfully!" << std::endl;
        std::cout << "Number: #" << created.number << std::endl;
        std::cout << "ID: " << created.id << std::endl;
        std::cout << "Title: " << created.title << std::endl;
        std::cout << "Type: " << created.type << std::endl;
        std::cout << "Board: " << created.board.title << std::endl;
        std::cout << "Column: " << created.column.title << std::endl;
        std::cout << "Lane: " << created.lane.title << std::endl;
        
        if (!created.tags.empty()) {
            std::cout << "Tags: ";
            for (size_t i = 0; i < created.tags.size(); ++i) {
                std::cout << created.tags[i].name;
                if (i < created.tags.size() - 1) std::cout << ", ";
            }
            std::cout << std::endl;
        }
        
        return 0;
    }
    
    std::cerr << "✗ Failed to create card. Status: " << status << std::endl;
    return 1;
}

// Новая функция для получения досок
int handle_boards_list(Http_client& client, const std::string& host,
                      const std::string& api_path, const std::string& token)
{
    kaiten::PaginationParams params;
    params.per_page = 50;
    
    auto board_fetcher = [](Http_client& client, const std::string& host,
                           const std::string& api_path, const std::string& token,
                           const kaiten::PaginationParams& pagination) {
        return kaiten::get_boards_paginated(client, host, api_path, token, pagination);
    };
    
    auto board_handler = [](const std::vector<Board>& boards, 
                           const kaiten::PaginatedResult<Board>& result) {
        for (const auto& board : boards) {
            std::cout << "[" << board.id << "] " << board.title;
            if (board.external_id.has_value()) {
                std::cout << " (ext: " << board.external_id.value() << ")";
            }
            std::cout << std::endl;
        }
    };
    
    std::cout << "Fetching boards with pagination..." << std::endl;
    return paginate_with_metadata<Board>(client, host, api_path, token, 
                                        board_fetcher, board_handler, params) ? 0 : 1;
}
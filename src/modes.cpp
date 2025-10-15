#include "modes.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>

#include "card.hpp"
#include "card_utils.hpp"
#include "kaiten.hpp"
#include "pagination.hpp"

// Улучшенная функция пагинации с поддержкой offset/limit
namespace {

template <typename T, typename Client, typename Fetcher, typename Handler>
bool paginate_with_metadata(
    Client& client,
    const std::string& host,
    const std::string& api_path,
    const std::string& token,
    Fetcher fetcher,
    Handler handle_items,
    const kaiten::Pagination_params& initial_params = {},
    int max_pages = 1000)
{
    kaiten::Pagination_params params = initial_params;
    int pages_processed = 0;
    int total_items = 0;

    // Вычисляем начальный offset на основе limit (для обратной совместимости)
    // Если offset уже установлен, используем его, иначе вычисляем из page-based логики
    int current_offset = params.offset;

    // Для обратной совместимости: если хотим начать с определенной "страницы",
    // можно установить offset соответствующим образом
    // current_offset = (desired_page - 1) * params.limit;

    while (pages_processed < max_pages) {
        // Устанавливаем текущий offset
        params.offset = current_offset;

        auto page_result = fetcher(client, host, api_path, token, params);

        if (page_result.items.empty()) {
            break;
        }

        // Обрабатываем элементы
        handle_items(page_result.items, page_result);

        total_items += page_result.items.size();

        // Вычисляем текущую "страницу" для вывода (для удобства)
        int current_page = (current_offset / params.limit) + 1;

        std::cout << "Page " << current_page << " (offset " << current_offset << "): "
                  << page_result.items.size() << " items, total: " << total_items;

        if (page_result.total_count > 0) {
            std::cout << " / " << page_result.total_count;
        }
        std::cout << std::endl;

        // Проверяем, есть ли следующая страница
        if (!page_result.has_more) {
            break;
        }

        // Увеличиваем offset для следующего запроса
        current_offset += params.limit;
        pages_processed++;

        // Небольшая задержка между запросами
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Completed: " << total_items << " total items" << std::endl;
    return total_items > 0;
}

// Новая функция для явной работы с offset/limit (рекомендуется)
template <typename T, typename Client, typename Fetcher, typename Handler>
bool paginate_with_offset_limit(
    Client& client,
    const std::string& host,
    const std::string& api_path,
    const std::string& token,
    Fetcher fetcher,
    Handler handle_items,
    const kaiten::Pagination_params& initial_params = {},
    int max_requests = 1000)
{
    kaiten::Pagination_params params = initial_params;
    int requests_processed = 0;
    int total_items = 0;

    while (requests_processed < max_requests) {
        auto page_result = fetcher(client, host, api_path, token, params);

        if (page_result.items.empty()) {
            break;
        }

        // Обрабатываем элементы
        handle_items(page_result.items, page_result);

        total_items += page_result.items.size();

        std::cout << "Offset " << params.offset << ", limit " << params.limit << ": "
                  << page_result.items.size() << " items, total: " << total_items;

        if (page_result.total_count > 0) {
            std::cout << " / " << page_result.total_count;
        }
        std::cout << std::endl;

        // Проверяем, есть ли следующая страница
        if (!page_result.has_more) {
            break;
        }

        // Увеличиваем offset для следующего запроса
        params.offset += params.limit;
        requests_processed++;

        // Небольшая задержка между запросами
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
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

// Реализация handle_cards_list с правильной пагинацией offset/limit
int handle_cards_list(Http_client& client, const std::string& host,
    const std::string& api_path, const std::string& token)
{
    kaiten::Pagination_params params;
    params.limit = 100; // Kaiten API максимум
    params.sort_by = "updated";
    params.sort_order = "desc";

    std::vector<Card> all_cards;
    int offset = 0;

    std::cout << "Fetching all cards using Kaiten API pagination (offset/limit)..." << std::endl;

    while (true) {
        params.offset = offset;
        std::cout << "Fetching offset " << offset << ", limit " << params.limit << "..." << std::endl;

        auto page_result = kaiten::get_cards_paginated(client, host, api_path, token, params);

        if (page_result.items.empty()) {
            std::cout << "No more cards found." << std::endl;
            break;
        }

        // Добавляем карточки в общий список
        all_cards.insert(all_cards.end(),
            page_result.items.begin(),
            page_result.items.end());

        std::cout << "Offset " << offset << ": " << page_result.items.size()
                  << " cards, total: " << all_cards.size() << std::endl;

        // Проверяем, есть ли следующая страница
        if (!page_result.has_more) {
            break;
        }

        // Увеличиваем offset для следующего запроса
        offset += params.limit;

        // Небольшая задержка между запросами
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "\n=== All Cards Results ===" << std::endl;
    std::cout << "Total cards fetched: " << all_cards.size() << std::endl;

    // Выводим статистику по типам и состояниям
    std::map<std::string, int> type_stats;
    std::map<std::string, int> state_stats;
    std::map<std::string, int> board_stats;

    for (const auto& card : all_cards) {
        type_stats[card.type.empty() ? "Unknown" : card.type]++;
        state_stats[card.state.empty() ? "Unknown" : card.state]++;
        board_stats[card.board.title.empty() ? "Unknown" : card.board.title]++;
    }

    // Выводим карточки
    for (const auto& card : all_cards) {
        std::cout << "#" << card.number << " [" << card.id << "] "
                  << card.title << " (" << card.type
                  << ", size=" << card.size
                  << ", state=" << card.state
                  << ", board=" << card.board.title
                  << ", updated: " << card.updated.toIso8601() << ")" << std::endl;
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

    std::cout << "By board:" << std::endl;
    for (const auto& [board, count] : board_stats) {
        std::cout << "  " << board << ": " << count << std::endl;
    }

    return all_cards.empty() ? 1 : 0;
}


// Реализация handle_backlog (пакетное создание задач по JSON описанию)
int handle_backlog(Http_client& client, const std::string& host,
    const std::string& api_path, const std::string& token,
    const Config& config, const std::string& backlog_file_path)
{
    std::ifstream f(backlog_file_path);
    if (!f) {
        std::cerr << "Could not open backlog file: " << backlog_file_path << std::endl;
        return 1;
    }

    nlohmann::json j;
    try {
        f >> j;
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse backlog JSON: " << e.what() << std::endl;
        return 1;
    }

    if (!j.contains("backlog") || !j["backlog"].is_array()) {
        std::cerr << "Invalid backlog JSON: missing 'backlog' array" << std::endl;
        return 1;
    }

    int success = 0;
    int errors = 0;

    auto [status, current_user] = kaiten::get_current_user(client, host, api_path, config.token);
    if (status == 200) {
        std::cout << "id=" << current_user.id << " " << current_user.full_name << " <" << current_user.email << ">" << std::endl;
    }

    for (const auto& entry : j["backlog"]) {
        const std::string parent = entry.value("parent", std::string());
        const std::string responsible = entry.value("responsible", std::string());
        const std::string role = entry.value("role", std::string());

        std::int64_t parent_card_id = 0;
        if (!parent.empty()) {
            parent_card_id = std::stoll(parent);
        }

        std::vector<std::string> base_tags;
        if (entry.contains("tags") && entry["tags"].is_array()) {
            for (const auto& t : entry["tags"]) {
                if (t.is_string()) { 
                    base_tags.push_back(t.get<std::string>());
                }
            }
        }

        if (!entry.contains("tasks") || !entry["tasks"].is_array()) {
            std::cerr << "Backlog entry has no 'tasks' array, skipping." << std::endl;
            continue;
        }

        for (const auto& t : entry["tasks"]) {
            Simple_card desired;
            desired.title = t.value("title", std::string());
            desired.size = t.value("size", 0);
            desired.board_id = std::stoll(config.boardId);
            desired.column_id = std::stoll(config.columnId);
            desired.lane_id = std::stoll(config.laneId);
            desired.type_id = 6;

            // Тип по умолчанию, если в системе требуется
            if (t.contains("type_id")) {
                try { desired.type_id = t.at("type_id").get<std::int64_t>(); } catch (...) {}
            }

            // Merge tags from entry and task

            // desired.tags = base_tags;
            // if (t.contains("tags") && t["tags"].is_array()) {
            //     for (const auto& tg : t["tags"]) {
            //         if (tg.is_string()) {
            //             desired.tags.push_back(tg.get<std::string>());
            //         }
            //     }
            // }
            // if (!config.tags.empty()) {
            //     desired.tags.insert(desired.tags.end(), config.tags.begin(), config.tags.end());
            // }
            // std::sort(desired.tags.begin(), desired.tags.end());
            // desired.tags.erase(std::unique(desired.tags.begin(), desired.tags.end()), desired.tags.end());

            // Сохраняем контекст в properties (если на сервере есть маппинг пользовательских полей)


            if (!responsible.empty()) {

                 auto [status, users] = kaiten::get_users_by_email(client, host, api_path, config.token, responsible);
                 if (status == 200 && !users.empty()) {
                     for (const auto& u : users) {
                         std::cout << "id=" << u.id << " " << u.full_name << " <" << u.email << ">" << std::endl;

                         if (u.email == responsible && u.id > 0) {
                            current_user.id = u.id;
                         }
                     }
                 }
            }
            desired.responsible_id = current_user.id;

            if (!role.empty()) {
                desired.set_role_id(role);
            }

            std::cout << "Creating backlog card: '" << desired.title << "' size=" << desired.size;
            if (!parent.empty()) {
                std::cout << ", parent=" << parent;
            }
            if (!responsible.empty()) {
                std::cout << ", responsible=" << responsible;
            }
            if (!role.empty()) {
                std::cout << ", role=" << role;
            }
            std::cout << std::endl;

            auto [status, created] = kaiten::create_card(client, host, api_path, token, desired);
            if (status == 200 || status == 201) {
                std::cout << "✓ Created card #" << created.number << " [" << created.id << "] '" << created.title << "'" << std::endl;
                success++;

                if (parent_card_id > 0) {

                    std::int64_t child_card_id = created.id;

                    auto [status, ok] = kaiten::add_child_card(client, host, api_path, token, parent_card_id, child_card_id);
                    if (status == 200 || status == 201) {
                        std::cout << "Child linked successfully\n";
                    }
                }

            } else {
                std::cerr << "✗ Failed to create card. Status: " << status << std::endl;
                errors++;
            }
        }
    }

    std::cout << "Backlog processing done. Success: " << success << ", Errors: " << errors << std::endl;
    return errors > 0 ? 1 : 0;
}



// Реализация handle_cards_filter
int handle_cards_filter(Http_client& client, const std::string& host,
    const std::string& api_path, const std::string& token,
    const std::map<std::string, std::string>& filters)
{
    kaiten::Card_filter_params filter_params;
    kaiten::Pagination_params pagination(100);
    // pagination.per_page = 100;

    // Преобразуем простые фильтры в структурированные
    for (const auto& [key, value] : filters) {
        if (key == "board_id") {
            filter_params.board_id = std::stoll(value);
        } else if (key == "lane_id") {
            filter_params.lane_id = std::stoll(value);
        } else if (key == "column_id") {
            filter_params.column_id = std::stoll(value);
        } else if (key == "owner_id") {
            filter_params.owner_id = std::stoll(value);
        } else if (key == "member_id") {
            filter_params.member_id = std::stoll(value);
        } else if (key == "type_id") {
            filter_params.type_id = std::stoll(value);
        } else if (key == "type") {
            filter_params.type_name = value;
        } else if (key == "state") {
            filter_params.state = value;
        } else if (key == "archived") {
            filter_params.archived = (value == "true");
        } else if (key == "blocked") {
            filter_params.blocked = (value == "true");
        } else if (key == "asap") {
            filter_params.asap = (value == "true");
        } else if (key == "search") {
            filter_params.search = value;
        } else if (key == "created_after") {
            filter_params.created_after = value;
        } else if (key == "created_before") {
            filter_params.created_before = value;
        } else if (key == "updated_after") {
            filter_params.updated_after = value;
        } else if (key == "updated_before") {
            filter_params.updated_before = value;
        } else {
            filter_params.custom_filters[key] = value;
        }
    }

    std::cout << "Fetching filtered cards with pagination..." << std::endl;
    std::cout << "Applied filters:" << std::endl;
    for (const auto& [key, value] : filters) {
        std::cout << "  " << key << ": " << value << std::endl;
    }

    auto [status, cards] = kaiten::get_all_cards(client, host, api_path, token,
        filter_params, pagination.per_page());

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



// Реализация handle_get_user
int handle_get_user(Http_client& client, const std::string& host,
    const std::string& api_path, const std::string& token,
    const std::string& space_id,
    const std::string& user_id)
{
    try {
        std::int64_t user_id_num = std::stoll(user_id);
        auto [status, user] = kaiten::get_user(client, host, api_path, token, space_id, user_id_num);

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
        desired.board_id = std::stoll(config.boardId);
        desired.column_id = std::stoll(config.columnId);
        desired.lane_id = std::stoll(config.laneId);
        desired.type_id = 6; // Task Delivery - 6 //task.value("type", "6");
        desired.size = task.value("size", 0);

        // Обработка тегов
        if (task.contains("tags") && task["tags"].is_array()) {
            for (const auto& tag : task["tags"]) {
                if (tag.is_string()) {
                    desired.tags.push_back(tag.get<std::string>());
                }
            }
        }

        // Обработка properties
        if (task.contains("properties") && task["properties"].is_object()) {
            for (auto it = task["properties"].begin(); it != task["properties"].end(); ++it) {
                if (it.value().is_string()) {
                    desired.properties[it.key()] = it.value().get<std::string>();
                } else if (it.value().is_number()) {
                    // Конвертируем числа в строки
                    desired.properties[it.key()] = std::to_string(it.value().get<int>());
                } else if (it.value().is_boolean()) {
                    // Конвертируем булевы значения в строки
                    desired.properties[it.key()] = it.value().get<bool>() ? "true" : "false";
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
                  << " (type_id: " << desired.type_id
                  << ", board_id: " << desired.board_id
                  << ", size: " << desired.size
                  << ", tags: " << desired.tags.size()
                  << ", properties: " << desired.properties.size() << ")" << std::endl;

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
            // std::this_thread::sleep_for(std::chrono::milliseconds(500));
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
    desired.board_id = std::stoll(config.boardId);
    desired.column_id = std::stoll(config.columnId);
    desired.lane_id = std::stoll(config.laneId);
    desired.type_id = std::stoll(type);
    desired.size = size;
    desired.tags = tags;
    desired.properties["id_19"] = "1";

    // Добавляем теги из конфигурации
    if (!config.tags.empty()) {
        desired.tags.insert(desired.tags.end(), config.tags.begin(), config.tags.end());
    }

    // Убираем дубликаты тегов
    std::sort(desired.tags.begin(), desired.tags.end());
    desired.tags.erase(std::unique(desired.tags.begin(), desired.tags.end()), desired.tags.end());

    std::cout << "Creating single card..." << std::endl;
    std::cout << "Title: " << desired.title << std::endl;
    std::cout << "Board ID: " << desired.board_id << std::endl;
    std::cout << "Column ID: " << desired.column_id << std::endl;
    std::cout << "Lane ID: " << desired.lane_id << std::endl;
    std::cout << "Type: " << desired.type_id << std::endl;
    std::cout << "Size: " << desired.size << std::endl;
    std::cout << "Tags: ";
    for (size_t i = 0; i < desired.tags.size(); ++i) {
        std::cout << desired.tags[i];
        if (i < desired.tags.size() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << std::endl;

    // Выводим properties если они есть
    if (!desired.properties.empty()) {
        std::cout << "Properties: ";
        for (const auto& [key, value] : desired.properties) {
            std::cout << key << "=" << value << " ";
        }
        std::cout << std::endl;
    }

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
                if (i < created.tags.size() - 1) {
                    std::cout << ", ";
                }
            }
            std::cout << std::endl;
        }

        return 0;
    }

    std::cerr << "✗ Failed to create card. Status: " << status << std::endl;
    return 1;
}



// Реализация handle_users_list с новой пагинацией
int handle_users_list(Http_client& client, const std::string& host,
    const std::string& api_path, const std::string& token)
{
    kaiten::Pagination_params params;
    params.limit = 100; // Kaiten API максимум

    auto user_fetcher = [](Http_client& client, const std::string& host,
                            const std::string& api_path, const std::string& token,
                            const kaiten::Pagination_params& pagination) {
        kaiten::User_filter_params user_filters;
        return kaiten::get_users_paginated(client, host, api_path, token, pagination, user_filters);
    };

    auto user_handler = [](const std::vector<User>& users,
                            const kaiten::Paginated_result<User>& result) {
        for (const auto& user : users) {
            std::cout << "[" << user.id << "] " << user.full_name
                      << " (" << user.email << ")"
                      << " - " << user.username
                      << (user.activated ? " [ACTIVE]" : " [INACTIVE]")
                      << (user.virtual_user ? " [VIRTUAL]" : "")
                      << std::endl;
        }
    };

    std::cout << "Fetching users with offset/limit pagination..." << std::endl;
    return paginate_with_offset_limit<User>(client, host, api_path, token,
               user_fetcher, user_handler, params)
             ? 0
             : 1;
}

// Реализация handle_boards_list с новой пагинацией
int handle_boards_list(Http_client& client, const std::string& host,
    const std::string& api_path, const std::string& token)
{
    kaiten::Pagination_params params;
    params.limit = 50; // Kaiten API максимум

    auto board_fetcher = [](Http_client& client, const std::string& host,
                             const std::string& api_path, const std::string& token,
                             const kaiten::Pagination_params& pagination) {
        return kaiten::get_boards_paginated(client, host, api_path, token, pagination);
    };

    auto board_handler = [](const std::vector<Board>& boards,
                             const kaiten::Paginated_result<Board>& result) {
        for (const auto& board : boards) {
            std::cout << "[" << board.id << "] " << board.title;
            if (board.external_id.has_value()) {
                std::cout << " (ext: " << board.external_id.value() << ")";
            }
            std::cout << std::endl;
        }
    };

    std::cout << "Fetching boards with offset/limit pagination..." << std::endl;
    return paginate_with_offset_limit<Board>(client, host, api_path, token,
               board_fetcher, board_handler, params)
             ? 0
             : 1;
}

// Альтернативная реализация handle_cards_list с использованием новой пагинации
int handle_cards_list_simple(Http_client& client, const std::string& host,
    const std::string& api_path, const std::string& token)
{
    kaiten::Card_filter_params no_filters;
    int page_size = 100;

    std::cout << "Fetching all cards with offset/limit pagination..." << std::endl;

    auto [status, all_cards] = kaiten::get_all_cards(client, host, api_path, token,
        no_filters, page_size);

    if (status != 200) {
        std::cerr << "Failed to fetch cards. Status: " << status << std::endl;
        return 1;
    }

    std::cout << "\n=== All Cards Results ===" << std::endl;
    std::cout << "Total cards fetched: " << all_cards.size() << std::endl;

    // Выводим статистику
    std::map<std::string, int> type_stats;
    std::map<std::string, int> state_stats;
    std::map<std::string, int> board_stats;

    for (const auto& card : all_cards) {
        type_stats[card.type.empty() ? "Unknown" : card.type]++;
        state_stats[card.state.empty() ? "Unknown" : card.state]++;
        board_stats[card.board.title.empty() ? "Unknown" : card.board.title]++;

        // Выводим информацию о карточке
        std::cout << "#" << card.number << " [" << card.id << "] "
                  << card.title << " (" << card.type
                  << ", size=" << card.size
                  << ", state=" << card.state
                  << ", board=" << card.board.title
                  << ", updated: " << card.updated.toIso8601() << ")" << std::endl;
    }

    // Выводим итоговую статистику
    std::cout << "\n=== Final Statistics ===" << std::endl;

    std::cout << "By type:" << std::endl;
    for (const auto& [type, count] : type_stats) {
        std::cout << "  " << type << ": " << count << std::endl;
    }

    std::cout << "By state:" << std::endl;
    for (const auto& [state, count] : state_stats) {
        std::cout << "  " << state << ": " << count << std::endl;
    }

    std::cout << "By board:" << std::endl;
    for (const auto& [board, count] : board_stats) {
        std::cout << "  " << board << ": " << count << std::endl;
    }

    return all_cards.empty() ? 1 : 0;
}

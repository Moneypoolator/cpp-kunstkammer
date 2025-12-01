#include "modes.hpp"

#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <future>
#include <mutex>

#include "card.hpp"
#include "card_utils.hpp"
#include "kaiten.hpp"
#include "pagination.hpp"
#include "rate_limiter.hpp"
// #include "card_operations.hpp"
// #include "user_operations.hpp"
// #include "board_operations.hpp"

// Улучшенная функция пагинации с поддержкой offset/limit
namespace {

template <typename T, typename Client, typename Fetcher, typename Handler>
bool paginate_with_metadata(
    Client& client,
    const std::string& host,
    const std::string& port,
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

        auto page_result = fetcher(client, host, port, api_path, token, params);

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
        kaiten::global_rate_limiter.wait_if_needed();
    }

    std::cout << "Completed: " << total_items << " total items" << std::endl;
    return total_items > 0;
}

// Новая функция для явной работы с offset/limit (рекомендуется)
template <typename T, typename Client, typename Fetcher, typename Handler>
bool paginate_with_offset_limit(
    Client& client,
    const std::string& host,
    const std::string& port,
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
        auto page_result = fetcher(client, host, port, api_path, token, params);

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
        kaiten::global_rate_limiter.wait_if_needed();
    }

    std::cout << "Completed: " << total_items << " total items" << std::endl;
    return total_items > 0;
}

} // namespace

// Реализация handle_get_card
int handle_get_card(Http_client& client, const std::string& host, const std::string& port,
    const std::string& api_path, const std::string& token,
    const std::string& card_number)
{
    std::cout << "Fetching card: " << card_number << std::endl;

    auto [status, card] = kaiten::get_card(client, host, port, api_path, token, card_number);

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

        if (!card.properties.empty()) {
            std::cout << "Properties: ";
            for (const auto& value : card.properties) {
                std::cout << (value) << " ";
            }
            std::cout << std::endl;
        }
        return 0;
    }

    std::cerr << "Failed to get card. Status: " << status << std::endl;
    return 1;
}

// Вспомогательная функция для получения одной страницы карточек (используется в потоках)
namespace {
kaiten::Paginated_result<Card> fetch_page_result(
    Http_client& client,
    const std::string& host,
    const std::string& port,
    const std::string& api_path,
    const std::string& token,
    const kaiten::Pagination_params& params)
{
    return kaiten::get_cards_paginated(client, host, port, api_path, token, params);
}
} // namespace

// Реализация handle_cards_list с использованием нового batched подхода
int handle_cards_list(Http_client& client, const std::string& host, const std::string& port,
    const std::string& api_path, const std::string& token)
{
    kaiten::Card_filter_params no_filters;
    kaiten::Pagination_params pagination_params;
    pagination_params.sort_by = "updated";
    pagination_params.sort_order = "desc";
    pagination_params.limit = 50; // Используем лимит 50 для более плавной загрузки
    
    std::cout << "Fetching all cards using batched Kaiten API pagination with sorting..." << std::endl;
    
    // Используем новый batched подход для получения всех карточек с сортировкой
    std::vector<Card> all_cards;
    int current_offset = 0;
    bool has_more = true;
    int total_fetched = 0;
    
    // Выводим статистику по типам и состояниям
    std::map<std::string, int> type_stats;
    std::map<std::string, int> state_stats;
    std::map<std::string, int> board_stats;
    
    while (has_more) {
        pagination_params.offset = current_offset;
        
        auto page_result = kaiten::get_cards_paginated(client, host, port, api_path, token,
            pagination_params, no_filters);
        
        if (page_result.items.empty()) {
            break;
        }
        
        // Добавляем карточки в общий список
        all_cards.insert(all_cards.end(), page_result.items.begin(), page_result.items.end());
        
        // Обновляем статистику
        for (const auto& card : page_result.items) {
            type_stats[card.type.empty() ? "Unknown" : card.type]++;
            state_stats[card.state.empty() ? "Unknown" : card.state]++;
            board_stats[card.board.title.empty() ? "Unknown" : card.board.title]++;
        }
        
        total_fetched += page_result.items.size();
        std::cout << "Fetched " << page_result.items.size() << " cards (offset " << current_offset << ")" << std::endl;
        
        // Проверяем, есть ли еще страницы
        has_more = page_result.has_more;
        current_offset += pagination_params.limit;
        
        // Небольшая задержка между запросами
        kaiten::global_rate_limiter.wait_if_needed();
    }
    
    if (all_cards.empty()) {
        std::cout << "No cards found." << std::endl;
        return 1;
    }
    
    std::cout << "\n=== All Cards Results ===" << std::endl;
    std::cout << "Total cards fetched: " << all_cards.size() << std::endl;
    
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
    
    return 0;
}

// Вспомогательные функции для обработки бэклога

/**
 * Получение информации о родительской карточке
 */
bool fetch_parent_card_info(
    Http_client& client,
    const std::string& host,
    const std::string& port,
    const std::string& api_path,
    const std::string& token,
    const std::string& parent_id_str,
    std::string& sprint_number,
    std::string& product_code,
    std::string& work_code)
{
    if (parent_id_str.empty()) {
        return false;
    }

    try {
        std::int64_t parent_card_id = std::stoll(parent_id_str);
        if (parent_card_id <= 0) {
            return false;
        }

        std::cout << "Fetching parent card: " << parent_card_id << std::endl;
        auto [status, card] = kaiten::get_card(client, host, port, api_path, token, std::to_string(parent_card_id));

        if (status != 200) {
            return false;
        }

        // Получаем номер спринта из свойств
        auto sprint_val = find_property_value_by_name(card, std::string(Simple_card::sprint_number_property));
        if (sprint_val.has_value()) {
            sprint_number = *sprint_val;
            std::cout << "Parent sprint number: " << sprint_number << std::endl;
        }

        // Извлекаем work code из заголовка
        if (!card.title.empty()) {
            std::string product;
            std::string work_code_temp;
            std::string error;
            if (extract_work_code(card.title, product, work_code_temp, error)) {
                product_code = product;
                work_code = work_code_temp;
                std::cout << "Work code: " << work_code << ", product: " << product << std::endl;
            } else {
                std::cout << "Extract Work Code error: " << error << std::endl;
                return false;
            }
        } else {
            std::cout << "Parent card title is empty" << std::endl;
            return false;
        }

        return true;

    } catch (const std::exception& e) {
        std::cerr << "Error processing parent card: " << e.what() << std::endl;
        return false;
    }
}

/**
 * Поиск пользователя по email для назначения ответственного
 */
std::int64_t find_responsible_user_id(
    Http_client& client,
    const std::string& host,
    const std::string& port,
    const std::string& api_path,
    const std::string& token,
    const std::string& responsible_email)
{
    if (responsible_email.empty()) {
        return 0;
    }

    auto [status, users] = kaiten::get_users_by_email(client, host, port, api_path, token, responsible_email);
    if (status == 200 && !users.empty()) {
        for (const auto& user : users) {
            if (user.email == responsible_email && user.id > 0) {
                std::cout << "Found responsible user: " << user.full_name << " <" << user.email << ">" << std::endl;
                return user.id;
            }
        }
    }

    std::cout << "Responsible user not found for email: " << responsible_email << std::endl;
    return 0;
}

/**
 * Создание базовой карточки из конфигурации
 */
Simple_card create_base_card_from_config(
    const Config& config,
    const std::string& sprint_number,
    const std::string& role)
{
    Simple_card base_card;
    base_card = config; // Применяем основную конфигурацию

    // Настраиваем свойства спринта
    if (!sprint_number.empty()) {
        try {
            int sprint_num = std::stoi(sprint_number);
            base_card.set_property_number(std::string(Simple_card::sprint_number_property), sprint_num);
        } catch (const std::exception&) {
            base_card.set_property_string(std::string(Simple_card::sprint_number_property), sprint_number);
        }
    } else {
        base_card.set_property_null(std::string(Simple_card::sprint_number_property));
    }

    // Устанавливаем роль
    if (!role.empty()) {
        base_card.set_role_id(role);
    }

    return base_card;
}

/**
 * Создание конкретной задачи из шаблона
 */
Simple_card parse_task_card_from_backlog(
    const Simple_card& base_card,
    const nlohmann::json& task_json,
    std::int64_t responsible_user_id)
{
    Simple_card task_card = base_card; // Копируем базовую конфигурацию

    // Устанавливаем заголовок и размер
    task_card.set_content(
        task_json.value("title", ""),
        task_json.value("size", 0));

    // Назначаем ответственного
    if (responsible_user_id > 0) {
        task_card.responsible_id = responsible_user_id;
    }

    // Переопределяем тип если указан в задаче
    if (task_json.contains("type_id")) {
        try {
            task_card.type_id = task_json.at("type_id").get<std::int64_t>();
        } catch (...) {
            // Оставляем тип из конфигурации
        }
    }

    return task_card;
}

/**
 * Добавление тегов к карточке из JSON
 */
void add_tags_from_json(
    Simple_card& card,
    const nlohmann::json& tags_json)
{
    if (tags_json.is_array()) {
        std::vector<std::string> entry_tags;
        for (const auto& tag : tags_json) {
            if (tag.is_string()) {
                entry_tags.push_back(tag.get<std::string>());
            }
        }
        card.add_tags(entry_tags);
    }
}

/**
 * Создание карточки в системе
 */
std::pair<int, Card> create_card_in_system(
    Http_client& client,
    const std::string& host,
    const std::string& port,
    const std::string& api_path,
    const std::string& token,
    const Simple_card& card_data)
{
    std::cout << "Creating card: '" << card_data.title << "' size=" << card_data.size;
    if (card_data.responsible_id > 0) {
        std::cout << ", responsible_id=" << card_data.responsible_id;
    }
    std::cout << std::endl;

    return kaiten::create_card(client, host, port, api_path, token, card_data);
}

/**
 * Обновление заголовка карточки с work code
 */
bool update_card_title_with_work_code(
    Http_client& client,
    const std::string& host,
    const std::string& port,
    const std::string& api_path,
    const std::string& token,
    std::int64_t card_id,
    const std::string& product_code,
    const std::string& work_code,
    const std::string& original_title)
{
    std::ostringstream updated_title;
    updated_title << "[" << product_code << "]:TS." << work_code << "." << card_id << ". " << original_title;

    Simple_card changes;
    changes.title = updated_title.str();

    auto [status, updated_card] = kaiten::update_card(
        client, host, port, api_path, token, std::to_string(card_id), changes);

    if (status == 200 || status == 201) {
        std::cout << "Card title updated successfully with work code" << std::endl;
        return true;
    }

    std::cerr << "Failed to update card title" << std::endl;
    return false;
}

/**
 * Добавление тегов к созданной карточке
 */
void add_tags_to_created_card(
    Http_client& client,
    const std::string& host,
    const std::string& port,
    const std::string& api_path,
    const std::string& token,
    std::int64_t card_id,
    const std::vector<std::string>& tags)
{
    for (const auto& tag : tags) {
        auto [status, ok] = kaiten::add_tag_to_card(client, host, port, api_path, token, card_id, tag);
        if (status == 200 || status == 201) {
            std::cout << "Tag '" << tag << "' added successfully" << std::endl;
        } else {
            std::cerr << "Failed to add tag '" << tag << "'" << std::endl;
        }
    }
}

/**
 * Связывание карточки с родителем
 */
bool link_card_to_parent(
    Http_client& client,
    const std::string& host,
    const std::string& port,
    const std::string& api_path,
    const std::string& token,
    std::int64_t parent_card_id,
    std::int64_t child_card_id)
{
    if (parent_card_id <= 0) {
        return false;
    }

    auto [status, ok] = kaiten::add_child_card(client, host, port, api_path, token, parent_card_id, child_card_id);
    if (status == 200 || status == 201) {
        std::cout << "Child linked successfully to parent" << std::endl;
        return true;
    }

    std::cerr << "Failed to link child to parent" << std::endl;
    return false;
}

/**
 * Создание карточки и выполнение дополнительных действий
 */
std::optional<Card> create_card_with_postprocessing(
    Http_client& client,
    const std::string& host,
    const std::string& port,
    const std::string& api_path,
    const std::string& token,
    const Simple_card& card_data,
    std::int64_t parent_card_id,
    const std::string& product_code,
    const std::string& work_code,
    bool update_title)
{
    auto [status, created_card] = create_card_in_system(client, host, port, api_path, token, card_data);

    if (status != 200 && status != 201) {
        std::cerr << "✗ Failed to create card. Status: " << status << std::endl;
        return std::nullopt;
    }

    std::cout << "✓ Created card #" << created_card.number << " [" << created_card.id
              << "] '" << created_card.title << "'" << std::endl;

    const std::int64_t child_card_id = created_card.id;

    if (parent_card_id > 0) {
        link_card_to_parent(client, host, port, api_path, token, parent_card_id, child_card_id);
    }

    if (!card_data.tags.empty()) {
        add_tags_to_created_card(client, host, port, api_path, token, child_card_id, card_data.tags);
    }

    if (update_title && !product_code.empty() && !work_code.empty()) {
        update_card_title_with_work_code(
            client, host, port, api_path, token, child_card_id, product_code, work_code, card_data.title);
    }

    return created_card;
}

/**
 * Обработка одной записи бэклога
 */
std::pair<int, int> process_backlog_entry(
    Http_client& client,
    const std::string& host,
    const std::string& port,
    const std::string& api_path,
    const std::string& token,
    const Config& config,
    const nlohmann::json& entry,
    std::int64_t current_user_id)
{
    int success_count = 0;
    int error_count = 0;

    // Получаем данные из entry
    const std::string parent_id_str = entry.value("parent", "");
    const std::string responsible_email = entry.value("responsible", "");
    std::string role = entry.value("role", config.role);

    // Получаем информацию о родительской карточке
    std::string sprint_number;
    std::string product_code = "CAD";
    std::string work_code = "XXX.XX";

    std::int64_t parent_card_id = 0;
    if (!parent_id_str.empty()) {
        parent_card_id = std::stoll(parent_id_str);
        fetch_parent_card_info(client, host, port, api_path, token, parent_id_str,
            sprint_number, product_code, work_code);
    }

    // Находим ID ответственного пользователя
    std::int64_t responsible_user_id = current_user_id;
    if (!responsible_email.empty()) {
        std::int64_t found_user_id = find_responsible_user_id(client, host, port, api_path, token, responsible_email);
        if (found_user_id > 0) {
            responsible_user_id = found_user_id;
        }
    }

    // Создаем базовую карточку
    Simple_card base_card = create_base_card_from_config(config, sprint_number, role);

    // Добавляем теги из entry
    if (entry.contains("tags") && entry["tags"].is_array()) {
        add_tags_from_json(base_card, entry["tags"]);
    }

    // Проверяем наличие задач
    if (!entry.contains("tasks") || !entry["tasks"].is_array()) {
        std::cerr << "Backlog entry has no 'tasks' array, skipping." << std::endl;
        return { 0, 1 };
    }

    // Обрабатываем каждую задачу
    for (const auto& task : entry["tasks"]) {
        // Создаем конкретную задачу
        Simple_card task_card = parse_task_card_from_backlog(base_card, task, responsible_user_id);

        const bool should_update_title = !product_code.empty() && !work_code.empty();
        auto created_card = create_card_with_postprocessing(
            client,
            host,
            port,
            api_path,
            token,
            task_card,
            parent_card_id,
            product_code,
            work_code,
            should_update_title);

        if (created_card) {
            success_count++;
        } else {
            error_count++;
        }
    }

    return { success_count, error_count };
}

// Реализация handle_backlog (пакетное создание задач по JSON описанию)
int handle_backlog(Http_client& client, const std::string& host, const std::string& port,
    const std::string& api_path, const std::string& token,
    const Config& config, const std::string& backlog_file_path)
{
    // Загрузка и валидация JSON файла
    std::ifstream f(backlog_file_path);
    if (!f) {
        std::cerr << "Could not open backlog file: " << backlog_file_path << std::endl;
        return 1;
    }

    nlohmann::json backlog_json;
    try {
        f >> backlog_json;
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse backlog JSON: " << e.what() << std::endl;
        return 1;
    }

    if (!backlog_json.contains("backlog") || !backlog_json["backlog"].is_array()) {
        std::cerr << "Invalid backlog JSON: missing 'backlog' array" << std::endl;
        return 1;
    }

    // Получаем информацию о текущем пользователе
    auto [status, current_user] = kaiten::get_current_user(client, host, port, api_path, config.token);
    std::int64_t current_user_id = 0;
    if (status == 200) {
        current_user_id = current_user.id;
        std::cout << "Current user id=" << current_user.id << " " << current_user.full_name
                  << " <" << current_user.email << ">" << std::endl;
    }

    // Обрабатываем каждую запись в бэклоге
    int total_success = 0;
    int total_errors = 0;

    for (const auto& entry : backlog_json["backlog"]) {
        auto [success, errors] = process_backlog_entry(
            client, host, port, api_path, token, config, entry, current_user_id);

        total_success += success;
        total_errors += errors;
    }

    // Вывод итоговой статистики
    std::cout << "Backlog processing done. Success: " << total_success << ", Errors: " << total_errors << std::endl;

    return total_errors > 0 ? 1 : 0;
}





// Карта для преобразования фильтров
using FilterHandler = std::function<void(kaiten::Card_filter_params&, const std::string&)>;

static std::map<std::string, FilterHandler> filter_handlers()
{
    return {
        { "board_id", [](kaiten::Card_filter_params& params, const std::string& value) {
             try {
                 params.board_id = std::stoll(value);
             } catch (const std::exception& e) {
                 std::cerr << "Warning: Invalid board_id format: " << value << std::endl;
             }
         } },

        { "lane_id", [](kaiten::Card_filter_params& params, const std::string& value) {
             try {
                 params.lane_id = std::stoll(value);
             } catch (const std::exception& e) {
                 std::cerr << "Warning: Invalid lane_id format: " << value << std::endl;
             }
         } },

        { "column_id", [](kaiten::Card_filter_params& params, const std::string& value) {
             try {
                 params.column_id = std::stoll(value);
             } catch (const std::exception& e) {
                 std::cerr << "Warning: Invalid column_id format: " << value << std::endl;
             }
         } },

        { "owner_id", [](kaiten::Card_filter_params& params, const std::string& value) {
             try {
                 params.owner_id = std::stoll(value);
             } catch (const std::exception& e) {
                 std::cerr << "Warning: Invalid owner_id format: " << value << std::endl;
             }
         } },

        { "member_id", [](kaiten::Card_filter_params& params, const std::string& value) {
             try {
                 params.member_id = std::stoll(value);
             } catch (const std::exception& e) {
                 std::cerr << "Warning: Invalid member_id format: " << value << std::endl;
             }
         } },

        { "type_id", [](kaiten::Card_filter_params& params, const std::string& value) {
             try {
                 params.type_id = std::stoll(value);
             } catch (const std::exception& e) {
                 std::cerr << "Warning: Invalid type_id format: " << value << std::endl;
             }
         } },

        { "type", [](kaiten::Card_filter_params& params, const std::string& value) {
             params.type_name = value;
         } },

        { "state", [](kaiten::Card_filter_params& params, const std::string& value) {
             params.state = value;
         } },

        { "archived", [](kaiten::Card_filter_params& params, const std::string& value) {
             params.archived = (value == "true" || value == "1" || value == "yes");
         } },

        { "blocked", [](kaiten::Card_filter_params& params, const std::string& value) {
             params.blocked = (value == "true" || value == "1" || value == "yes");
         } },

        { "asap", [](kaiten::Card_filter_params& params, const std::string& value) {
             params.asap = (value == "true" || value == "1" || value == "yes");
         } },

        { "search", [](kaiten::Card_filter_params& params, const std::string& value) {
             params.search = value;
         } },

        { "created_after", [](kaiten::Card_filter_params& params, const std::string& value) {
             params.created_after = value;
         } },

        { "created_before", [](kaiten::Card_filter_params& params, const std::string& value) {
             params.created_before = value;
         } },

        { "updated_after", [](kaiten::Card_filter_params& params, const std::string& value) {
             params.updated_after = value;
         } },

        { "updated_before", [](kaiten::Card_filter_params& params, const std::string& value) {
             params.updated_before = value;
         } },

        { "condition", [](kaiten::Card_filter_params& params, const std::string& value) {
             try {
                 params.condition = std::stoi(value);
             } catch (const std::exception& e) {
                 std::cerr << "Warning: Invalid condition format: " << value << std::endl;
             }
         } },

        { "number", [](kaiten::Card_filter_params& params, const std::string& value) {
             params.number = value;
         } }
    };
}

/**
 * Применение фильтров к параметрам через карту обработчиков
 */
void apply_filters(kaiten::Card_filter_params& filter_params, const std::map<std::string, std::string>& filters)
{
    auto filters_container = filter_handlers();

    for (const auto& [key, value] : filters) {
        auto handler_it = filters_container.find(key);
        if (handler_it != filters_container.end()) {
            try {
                handler_it->second(filter_params, value);
            } catch (const std::exception& e) {
                std::cerr << "Error applying filter '" << key << "': " << e.what() << std::endl;
            }
        } else {
            // Неизвестный фильтр - добавляем как кастомный
            filter_params.custom_filters[key] = value;
            std::cout << "Note: Using custom filter '" << key << "'" << std::endl;
        }
    }
}

/**
 * Вывод статистики по карточкам
 */
void print_cards_statistics(const std::vector<Card>& cards)
{
    if (cards.empty()) {
        std::cout << "No cards to display statistics" << std::endl;
        return;
    }

    std::map<std::string, int> type_stats;
    std::map<std::string, int> state_stats;
    std::map<std::string, int> board_stats;

    for (const auto& card : cards) {
        type_stats[card.type.empty() ? "Unknown" : card.type]++;
        state_stats[card.state.empty() ? "Unknown" : card.state]++;
        board_stats[card.board.title.empty() ? "Unknown" : card.board.title]++;
    }

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
}

/**
 * Вывод списка карточек
 */
void print_cards_list(const std::vector<Card>& cards)
{
    for (const auto& card : cards) {
        std::cout << "#" << card.number << " [" << card.id << "] "
                  << card.title << " (" << card.type
                  << ", size=" << card.size
                  << ", state=" << card.state
                  << ", owner=" << card.owner.full_name << ")" << std::endl;
    }
}

/**
 * Вывод информации о примененных фильтрах
 */
void print_applied_filters(const std::map<std::string, std::string>& filters)
{
    if (filters.empty()) {
        std::cout << "No filters applied" << std::endl;
        return;
    }

    std::cout << "Applied filters:" << std::endl;
    for (const auto& [key, value] : filters) {
        std::cout << "  " << key << ": " << value << std::endl;
    }
}


// Реализация handle_cards_filter (get cards with filters)
int handle_cards_filter(Http_client& client, const std::string& host, const std::string& port,
    const std::string& api_path, const std::string& token,
    const std::map<std::string, std::string>& filters)
{
    // Инициализация параметров
    kaiten::Card_filter_params filter_params;
    kaiten::Pagination_params pagination(100);

    // Применяем фильтры через карту обработчиков
    apply_filters(filter_params, filters);

    // Выводим информацию о фильтрах
    std::cout << "Fetching filtered cards with pagination..." << std::endl;
    print_applied_filters(filters);

    // Получаем карточки с примененными фильтрами
    auto [status, cards] = kaiten::get_all_cards(client, host, port, api_path, token,
        filter_params, pagination.per_page());

    if (status != 200) {
        std::cerr << "Failed to get filtered cards. Status: " << status << std::endl;
        return 1;
    }

    // Выводим результаты
    std::cout << "\n=== Filtered Cards Results ===" << std::endl;
    std::cout << "Total cards found: " << cards.size() << std::endl;

    if (!cards.empty()) {
        print_cards_list(cards);
        print_cards_statistics(cards);
    } else {
        std::cout << "No cards matching the specified filters were found." << std::endl;
    }

    return 0;
}



// Реализация handle_get_user
int handle_get_user(Http_client& client, const std::string& host, const std::string& port,
    const std::string& api_path, const std::string& token,
    const std::int64_t& space_id,
    const std::string& user_id)
{
    try {
        std::int64_t user_id_num = std::stoll(user_id);
        auto [status, user] = kaiten::get_user(client, host, port, api_path, token, space_id, user_id_num);

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



// Реализация handle_create_card
int handle_create_card(Http_client& client, const std::string& host, const std::string& port, const std::string& api_path,
    const std::string& token, const Config& config, const std::string& title,
    int size, std::int64_t parent_card_id, const std::vector<std::string>& tags)
{
    if (title.empty()) {
        std::cerr << "Error: Card title cannot be empty" << std::endl;
        return 1;
    }

    // Получаем информацию о текущем пользователе
    auto [status, current_user] = kaiten::get_current_user(client, host, port, api_path, config.token);
    std::int64_t current_user_id = 0;
    if (status == 200) {
        current_user_id = current_user.id;
        std::cout << "Current user id=" << current_user.id << " " << current_user.full_name
                  << " <" << current_user.email << ">" << std::endl;
    }

    // Получаем информацию о родительской карточке
    bool update_title = false;
    std::string sprint_number;
    std::string product_code = "CAD";
    std::string work_code = "XXX.XX";

    if (parent_card_id > 0) {
        std::string parent_id_str = std::to_string(parent_card_id);
        fetch_parent_card_info(client, host, port, api_path, token, parent_id_str,
            sprint_number, product_code, work_code);
        update_title = true;
    }

    std::string role = config.role;

    // Создаем базовую карточку
    Simple_card desired = create_base_card_from_config(config, sprint_number, role);

    desired.title = title;

    // Назначаем ответственного
    if (current_user_id > 0) {
        desired.responsible_id = current_user_id;
    }
    
    // Переопределяем размер, если указан явно
    if (size > 0) {
        desired.size = size;
    }
    
    // Добавляем дополнительные теги
    if (!tags.empty()) {
        desired.tags.clear();
        desired.add_tags(tags);
    }


    // desired.title = title;
    // desired.board_id = config.boardId;
    // desired.column_id = config.columnId;
    // desired.lane_id = config.laneId;
    // desired.type_id = std::stoll(type);
    // desired.size = size;
    // desired.tags = tags;
    // //desired.properties["id_19"] = "1";
    // desired.set_role_id("C++");

    // // Добавляем теги из конфигурации
    // if (!config.tags.empty()) {
    //     desired.tags.insert(desired.tags.end(), config.tags.begin(), config.tags.end());
    // }

    // // Убираем дубликаты тегов
    // std::sort(desired.tags.begin(), desired.tags.end());
    // desired.tags.erase(std::unique(desired.tags.begin(), desired.tags.end()), desired.tags.end());

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
            std::cout << key << "=" << property_value_to_json(value) << " ";
        }
        std::cout << std::endl;
    }

    if (parent_card_id > 0) {
        std::cout << "Parent card ID: " << parent_card_id << std::endl;
    }

    auto created_card = create_card_with_postprocessing(
        client, host, port, api_path, token, desired, parent_card_id, product_code, work_code, update_title);

    if (!created_card) {
        return 1;
    }

    const Card& created = *created_card;

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



// Реализация handle_users_list с новой пагинацией
int handle_users_list(Http_client& client, const std::string& host, const std::string& port,
    const std::string& api_path, const std::string& token)
{
    kaiten::Pagination_params params;
    params.limit = 50; // Уменьшаем лимит для более плавной загрузки

    auto user_fetcher = [](Http_client& client, const std::string& host, const std::string& port,
                            const std::string& api_path, const std::string& token,
                            const kaiten::Pagination_params& pagination) {
        kaiten::User_filter_params user_filters;
        return kaiten::get_users_paginated(client, host, port, api_path, token, pagination, user_filters);
    };

    auto user_handler = [](const std::vector<User>& users,
                            const kaiten::Paginated_result<User>& /*result*/) {
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
    return paginate_with_offset_limit<User>(client, host, port, api_path, token,
               user_fetcher, user_handler, params)
             ? 0
             : 1;
}

// Реализация handle_boards_list с новой пагинацией
int handle_boards_list(Http_client& client, const std::string& host, const std::string& port,
    const std::string& api_path, const std::string& token)
{
    kaiten::Pagination_params params;
    params.limit = 50; // Kaiten API максимум

    auto board_fetcher = [](Http_client& client, const std::string& host, const std::string& port,
                             const std::string& api_path, const std::string& token,
                             const kaiten::Pagination_params& pagination) {
        return kaiten::get_boards_paginated(client, host, port, api_path, token, pagination);
    };

    auto board_handler = [](const std::vector<Board>& boards,
                             const kaiten::Paginated_result<Board>& /*result*/) {
        for (const auto& board : boards) {
            std::cout << "[" << board.id << "] " << board.title;
            if (board.external_id.has_value()) {
                std::cout << " (ext: " << board.external_id.value() << ")";
            }
            std::cout << std::endl;
        }
    };

    std::cout << "Fetching boards with offset/limit pagination..." << std::endl;
    return paginate_with_offset_limit<Board>(client, host, port, api_path, token,
               board_fetcher, board_handler, params)
             ? 0
             : 1;
}

// Альтернативная реализация handle_cards_list с использованием новой пагинации
int handle_cards_list_simple(Http_client& client, const std::string& host, const std::string& port,
    const std::string& api_path, const std::string& token)
{
    kaiten::Card_filter_params no_filters;
    int page_size = 100;

    std::cout << "Fetching all cards with offset/limit pagination..." << std::endl;

    auto [status, all_cards] = kaiten::get_all_cards(client, host, port, api_path, token,
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

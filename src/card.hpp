// card.hpp
#ifndef CARD_HPP
#define CARD_HPP

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <optional>

#include "date.hpp"


// Альтернативный упрощенный подход
struct Simple_card {
    std::int64_t id = 0;
    std::string number;
    std::string title;
    std::int64_t type_id;
    int size = 0;
    bool archived = false;
    std::int64_t board_id = 0;
    std::int64_t column_id = 0;
    std::int64_t lane_id = 0;
    Card_date created;
    Card_date updated;
    std::vector<std::string> tags;
    std::map<std::string, std::string> properties;
};

// Структура для пользователя
struct User {
    std::int64_t id = 0;
    std::string uid;
    std::string full_name;
    std::string email;
    std::string username;
    std::string avatar_type;
    std::string avatar_uploaded_url;
    std::string avatar_initials_url;
    std::string theme;
    std::string lng;
    std::string timezone;
    int ui_version = 0;
    bool activated = false;
    bool virtual_user = false;
    std::optional<std::string> email_blocked;
    std::optional<std::string> email_blocked_reason;
    std::optional<Card_date> delete_requested_at;
    Card_date created;
    Card_date updated;

    // Для членов карточки
    std::optional<std::int64_t> card_id;
    std::optional<int> type; // 2 для members
};

// Структура для доски
struct Board {
    std::int64_t id = 0;
    std::string title;
    std::optional<std::string> external_id;
    // card_properties можно добавить при необходимости
};

// Структура для колонки
struct Column {
    std::int64_t id = 0;
    std::string title;
    std::string uid;
    std::int64_t board_id = 0;
    std::string type;
    int sort_order = 0;
    int col_count = 0;
    int rules = 0;
    bool pause_sla = false;
    std::optional<std::int64_t> column_id;
};

// Структура для лейна
struct Lane {
    std::int64_t id = 0;
    std::string title;
    std::int64_t board_id = 0;
    int condition = 0;
    int sort_order = 0;
    std::optional<std::int64_t> default_card_type_id;
    std::optional<std::string> external_id;
};

// Структура для типа карточки
struct Card_type {
    std::int64_t id = 0;
    std::string name;
    std::string letter;
    int color = 0;
    bool archived = false;
    std::int64_t company_id = 0;
    nlohmann::json properties; // свойства типа карточки
};

// Структура для тега
struct Tag {
    std::int64_t id = 0;
    std::int64_t tag_id = 0;
    std::optional<std::int64_t> card_id;
    std::string name;
    int color = 0;
};

// Структура для родительской карточки (упрощенная)
struct Parent_card {
    std::int64_t id = 0;
    std::int64_t card_id = 0;
    std::string title;
    std::string number;
    bool archived = false;
    bool asap = false;
    bool blocked = false;
    bool blocking_card = false;
    std::int64_t board_id = 0;
    std::int64_t column_id = 0;
    std::int64_t lane_id = 0;
    int state = 0;
    int size = 0;
    int children_count = 0;
    int children_done = 0;
    Card_date created;
    Card_date updated;
    Card_date column_changed_at;
    Card_date lane_changed_at;
    Card_date last_moved_at;
    Card_date last_moved_to_done_at;
    std::optional<std::int64_t> owner_id;
    std::optional<nlohmann::json> children_ids;
    std::optional<nlohmann::json> children_number_properties_sum;
};

// Структура для разрешений карточки
struct Card_permissions {
    bool comment = false;
    bool create = false;
    bool delete_perm = false;
    bool move = false;
    bool properties = false;
    bool read = false;
    bool read_own = false;
    bool update = false;
};

// Структура для path_data
struct Path_data {
    struct Path_item {
        std::int64_t id = 0;
        std::string title;
    };
    
    Path_item board;
    Path_item column;
    Path_item lane;
    Path_item space;
};

// Основная структура Card
struct Card {
    // Базовые поля
    std::int64_t id = 0;
    std::string number;
    std::string title;
    std::string uid;
    int version = 0;
    
    // Флаги состояния
    bool archived = false;
    bool asap = false;
    bool blocked = false;
    bool blocking_card = false;
    bool description_filled = false;
    bool due_date_time_present = false;
    bool expires_later = false;
    bool has_access_to_space = false;
    bool has_blocked_children = false;
    bool locked = false;
    bool public_card = false;
    bool sd_new_comment = false;
    
    // ID связей
    std::int64_t board_id = 0;
    std::int64_t column_id = 0;
    std::int64_t lane_id = 0;
    std::int64_t owner_id = 0;
    std::int64_t type_id = 0;
    std::int64_t space_id = 0;
    std::optional<std::int64_t> sprint_id;
    std::optional<std::int64_t> service_id;
    std::optional<std::int64_t> depends_on_card_id;
    std::optional<std::int64_t> import_id;
    
    // Вложенные объекты
    Board board;
    Column column;
    Lane lane;
    User owner;
    Card_type card_type;
    Card_permissions permissions;
    std::optional<Path_data> path_data;
    
    // Даты
    Card_date created;
    Card_date updated;
    Card_date column_changed_at;
    Card_date lane_changed_at;
    Card_date last_moved_at;
    Card_date last_moved_to_done_at;
    Card_date first_moved_to_in_progress_at;
    Card_date completed_at;
    Card_date counters_recalculated_at;
    Card_date comment_last_added_at;
    
    // Массивы
    std::vector<User> members;
    std::vector<Parent_card> parents;
    std::vector<Tag> tags;
    std::vector<std::int64_t> parent_ids;
    std::vector<std::int64_t> children_ids;
    std::vector<nlohmann::json> external_links;
    std::vector<nlohmann::json> files;
    std::vector<nlohmann::json> children;
    
    // Дополнительные поля
    std::string type; // строка для обратной совместимости
    std::string description;
    std::string state;
    std::string source;
    std::string email;
    std::string size_text;
    std::string size_unit;
    std::string share_id;
    std::string fts_version;
    
    // Числовые поля
    int size = 0;
    int condition = 0;
    int cardRole = 0;
    int comments_total = 0;
    int children_count = 0;
    int children_done = 0;
    int goals_done = 0;
    int goals_total = 0;
    int estimate_workload = 0;
    int time_spent_sum = 0;
    int time_blocked_sum = 0;
    double sort_order = 0.0;
    std::optional<double> fifo_order;
    
    // JSON объекты
    nlohmann::json properties;
    nlohmann::json share_settings;
    nlohmann::json children_number_properties_sum;
    nlohmann::json parent_checklist_ids;
    std::optional<nlohmann::json> external_id;
    std::optional<nlohmann::json> external_user_emails;
    std::optional<nlohmann::json> calculated_planned_end;
    std::optional<nlohmann::json> calculated_planned_start;
    std::optional<nlohmann::json> planned_end;
    std::optional<nlohmann::json> planned_start;
    std::optional<nlohmann::json> completed_on_time;
};

// Сериализация (базовая)
inline void to_json(nlohmann::json& j, const Card& c) {
    j = nlohmann::json{
        {"id", c.id},
        {"number", c.number},
        {"title", c.title},
        {"type", c.type},
        {"size", c.size},
        {"archived", c.archived},
        {"board_id", c.board_id},
        {"column_id", c.column_id},
        {"lane_id", c.lane_id},
        {"owner_id", c.owner_id},
        {"type_id", c.type_id},
        {"created", c.created},
        {"updated", c.updated},
        {"description", c.description}
    };
}

// Десериализация
void from_json(const nlohmann::json& j, Card& c);

// Реализации для вложенных структур
void from_json(const nlohmann::json& j, User& u);
void from_json(const nlohmann::json& j, Board& b);
void from_json(const nlohmann::json& j, Column& col);
void from_json(const nlohmann::json& j, Lane& l);
void from_json(const nlohmann::json& j, Card_type& ct);
void from_json(const nlohmann::json& j, Tag& t);
void from_json(const nlohmann::json& j, Parent_card& pc);
void from_json(const nlohmann::json& j, Card_permissions& cp);
void from_json(const nlohmann::json& j, Path_data& pd);
void from_json(const nlohmann::json& j, Path_data::Path_item& pi);

#endif // CARD_HPP
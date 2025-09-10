// card.cpp
#include "card.hpp"
#include <iostream>

// Вспомогательная функция для безопасного получения значений с обработкой null
template<typename T>
T get_optional(const nlohmann::json& j, const std::string& key, const T& default_value) {
    if (j.contains(key) && !j.at(key).is_null()) {
        try {
            return j.at(key).get<T>();
        } catch (const nlohmann::json::type_error& e) {
            std::cerr << "Warning: Type error for key '" << key << "': " << e.what() << std::endl;
            return default_value;
        } catch (...) {
            return default_value;
        }
    }
    return default_value;
}

// Специализация для std::string чтобы обрабатывать null как пустую строку
template<>
std::string get_optional<std::string>(const nlohmann::json& j, const std::string& key, const std::string& default_value) {
    if (j.contains(key)) {
        if (j.at(key).is_null()) {
            return default_value;
        }
        try {
            return j.at(key).get<std::string>();
        } catch (...) {
            return default_value;
        }
    }
    return default_value;
}

// Специализация для bool
template<>
bool get_optional<bool>(const nlohmann::json& j, const std::string& key, const bool& default_value) {
    if (j.contains(key) && !j.at(key).is_null()) {
        try {
            return j.at(key).get<bool>();
        } catch (...) {
            return default_value;
        }
    }
    return default_value;
}

// Функция для безопасного получения числовых значений с обработкой null и строк
template<typename T>
T get_numeric_optional(const nlohmann::json& j, const std::string& key, const T& default_value) {
    if (j.contains(key) && !j.at(key).is_null()) {
        try {
            if (j.at(key).is_number()) {
                return j.at(key).get<T>();
            } else if (j.at(key).is_string()) {
                // Попробуем преобразовать строку в число
                std::string str_val = j.at(key).get<std::string>();
                if constexpr (std::is_integral_v<T>) {
                    return static_cast<T>(std::stoll(str_val));
                } else if constexpr (std::is_floating_point_v<T>) {
                    return static_cast<T>(std::stod(str_val));
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Cannot convert '" << key << "' to number: " << e.what() << std::endl;
        }
    }
    return default_value;
}

// Функция для безопасного получения int64_t
std::int64_t get_int64_optional(const nlohmann::json& j, const std::string& key, std::int64_t default_value) {
    return get_numeric_optional<std::int64_t>(j, key, default_value);
}

// Функция для безопасного получения int
int get_int_optional(const nlohmann::json& j, const std::string& key, int default_value) {
    return get_numeric_optional<int>(j, key, default_value);
}

// Функция для безопасного получения double
double get_double_optional(const nlohmann::json& j, const std::string& key, double default_value) {
    return get_numeric_optional<double>(j, key, default_value);
}

// Реализации десериализации
void from_json(const nlohmann::json& j, User& u) {
    u.id = get_int64_optional(j, "id", 0LL);
    u.uid = get_optional(j, "uid", std::string());
    u.full_name = get_optional(j, "full_name", std::string());
    u.email = get_optional(j, "email", std::string());
    u.username = get_optional(j, "username", std::string());
    u.avatar_type = get_optional(j, "avatar_type", std::string());
    u.avatar_uploaded_url = get_optional(j, "avatar_uploaded_url", std::string());
    u.avatar_initials_url = get_optional(j, "avatar_initials_url", std::string());
    u.theme = get_optional(j, "theme", std::string());
    u.lng = get_optional(j, "lng", std::string());
    u.timezone = get_optional(j, "timezone", std::string());
    u.ui_version = get_int_optional(j, "ui_version", 0);
    u.activated = get_optional(j, "activated", false);
    u.virtual_user = get_optional(j, "virtual", false);
    
    if (j.contains("card_id") && !j.at("card_id").is_null()) {
        u.card_id = get_int64_optional(j, "card_id", 0LL);
    }
    if (j.contains("type") && !j.at("type").is_null()) {
        u.type = get_int_optional(j, "type", 0);
    }
}

void from_json(const nlohmann::json& j, Board& b) {
    b.id = get_int64_optional(j, "id", 0LL);
    b.title = get_optional(j, "title", std::string());
    if (j.contains("external_id") && !j.at("external_id").is_null()) {
        b.external_id = j.at("external_id").get<std::string>();
    }
}

void from_json(const nlohmann::json& j, Column& col) {
    col.id = get_int64_optional(j, "id", 0LL);
    col.title = get_optional(j, "title", std::string());
    col.uid = get_optional(j, "uid", std::string());
    col.board_id = get_int64_optional(j, "board_id", 0LL);
    col.type = get_optional(j, "type", std::string());
    col.sort_order = get_int_optional(j, "sort_order", 0);
    col.col_count = get_int_optional(j, "col_count", 0);
    col.rules = get_int_optional(j, "rules", 0);
    col.pause_sla = get_optional(j, "pause_sla", false);
    if (j.contains("column_id") && !j.at("column_id").is_null()) {
        col.column_id = get_int64_optional(j, "column_id", 0LL);
    }
}

void from_json(const nlohmann::json& j, Lane& l) {
    l.id = get_int64_optional(j, "id", 0LL);
    l.title = get_optional(j, "title", std::string());
    l.board_id = get_int64_optional(j, "board_id", 0LL);
    l.condition = get_int_optional(j, "condition", 0);
    l.sort_order = get_int_optional(j, "sort_order", 0);
    if (j.contains("default_card_type_id") && !j.at("default_card_type_id").is_null()) {
        l.default_card_type_id = get_int64_optional(j, "default_card_type_id", 0LL);
    }
    if (j.contains("external_id") && !j.at("external_id").is_null()) {
        l.external_id = j.at("external_id").get<std::string>();
    }
}

void from_json(const nlohmann::json& j, CardType& ct) {
    ct.id = get_int64_optional(j, "id", 0LL);
    ct.name = get_optional(j, "name", std::string());
    ct.letter = get_optional(j, "letter", std::string());
    ct.color = get_int_optional(j, "color", 0);
    ct.archived = get_optional(j, "archived", false);
    ct.company_id = get_int64_optional(j, "company_id", 0LL);
    if (j.contains("properties") && !j.at("properties").is_null()) {
        ct.properties = j.at("properties");
    }
}

void from_json(const nlohmann::json& j, Tag& t) {
    t.id = get_int64_optional(j, "id", 0LL);
    t.tag_id = get_int64_optional(j, "tag_id", 0LL);
    t.name = get_optional(j, "name", std::string());
    t.color = get_int_optional(j, "color", 0);
    if (j.contains("card_id") && !j.at("card_id").is_null()) {
        t.card_id = get_int64_optional(j, "card_id", 0LL);
    }
}

void from_json(const nlohmann::json& j, CardPermissions& cp) {
    cp.comment = get_optional(j, "comment", false);
    cp.create = get_optional(j, "create", false);
    cp.delete_perm = get_optional(j, "delete", false);
    cp.move = get_optional(j, "move", false);
    cp.properties = get_optional(j, "properties", false);
    cp.read = get_optional(j, "read", false);
    cp.read_own = get_optional(j, "read_own", false);
    cp.update = get_optional(j, "update", false);
}

void from_json(const nlohmann::json& j, PathData::PathItem& pi) {
    pi.id = get_int64_optional(j, "id", 0LL);
    pi.title = get_optional(j, "title", std::string());
}

void from_json(const nlohmann::json& j, PathData& pd) {
    if (j.contains("board") && !j.at("board").is_null()) j.at("board").get_to(pd.board);
    if (j.contains("column") && !j.at("column").is_null()) j.at("column").get_to(pd.column);
    if (j.contains("lane") && !j.at("lane").is_null()) j.at("lane").get_to(pd.lane);
    if (j.contains("space") && !j.at("space").is_null()) j.at("space").get_to(pd.space);
}

void from_json(const nlohmann::json& j, ParentCard& pc) {
    pc.id = get_int64_optional(j, "id", 0LL);
    pc.card_id = get_int64_optional(j, "card_id", 0LL);
    pc.title = get_optional(j, "title", std::string());
    pc.number = get_optional(j, "number", std::string());
    pc.archived = get_optional(j, "archived", false);
    pc.asap = get_optional(j, "asap", false);
    pc.blocked = get_optional(j, "blocked", false);
    pc.blocking_card = get_optional(j, "blocking_card", false);
    pc.board_id = get_int64_optional(j, "board_id", 0LL);
    pc.column_id = get_int64_optional(j, "column_id", 0LL);
    pc.lane_id = get_int64_optional(j, "lane_id", 0LL);
    pc.state = get_int_optional(j, "state", 0);
    pc.size = get_int_optional(j, "size", 0);
    pc.children_count = get_int_optional(j, "children_count", 0);
    pc.children_done = get_int_optional(j, "children_done", 0);
    
    if (j.contains("created") && !j.at("created").is_null()) j.at("created").get_to(pc.created);
    if (j.contains("updated") && !j.at("updated").is_null()) j.at("updated").get_to(pc.updated);
    if (j.contains("column_changed_at") && !j.at("column_changed_at").is_null()) j.at("column_changed_at").get_to(pc.column_changed_at);
    if (j.contains("lane_changed_at") && !j.at("lane_changed_at").is_null()) j.at("lane_changed_at").get_to(pc.lane_changed_at);
    if (j.contains("last_moved_at") && !j.at("last_moved_at").is_null()) j.at("last_moved_at").get_to(pc.last_moved_at);
    if (j.contains("last_moved_to_done_at") && !j.at("last_moved_to_done_at").is_null()) j.at("last_moved_to_done_at").get_to(pc.last_moved_to_done_at);
    
    if (j.contains("owner_id") && !j.at("owner_id").is_null()) {
        pc.owner_id = get_int64_optional(j, "owner_id", 0LL);
    }
    if (j.contains("children_ids") && !j.at("children_ids").is_null()) {
        pc.children_ids = j.at("children_ids");
    }
    if (j.contains("children_number_properties_sum") && !j.at("children_number_properties_sum").is_null()) {
        pc.children_number_properties_sum = j.at("children_number_properties_sum");
    }
}

void from_json(const nlohmann::json& j, Card& c) {
    try {
        // Базовые поля
        c.id = get_int64_optional(j, "id", 0LL);
        c.number = get_optional(j, "number", std::string());
        c.title = get_optional(j, "title", std::string());
        c.uid = get_optional(j, "uid", std::string());
        c.version = get_int_optional(j, "version", 0);
        
        // Флаги состояния
        c.archived = get_optional(j, "archived", false);
        c.asap = get_optional(j, "asap", false);
        c.blocked = get_optional(j, "blocked", false);
        c.blocking_card = get_optional(j, "blocking_card", false);
        c.description_filled = get_optional(j, "description_filled", false);
        c.due_date_time_present = get_optional(j, "due_date_time_present", false);
        c.expires_later = get_optional(j, "expires_later", false);
        c.has_access_to_space = get_optional(j, "has_access_to_space", false);
        c.has_blocked_children = get_optional(j, "has_blocked_children", false);
        c.locked = get_optional(j, "locked", false);
        c.public_card = get_optional(j, "public", false);
        c.sd_new_comment = get_optional(j, "sd_new_comment", false);
        
        // ID связей
        c.board_id = get_int64_optional(j, "board_id", 0LL);
        c.column_id = get_int64_optional(j, "column_id", 0LL);
        c.lane_id = get_int64_optional(j, "lane_id", 0LL);
        c.owner_id = get_int64_optional(j, "owner_id", 0LL);
        c.type_id = get_int64_optional(j, "type_id", 0LL);
        c.space_id = get_int64_optional(j, "space_id", 0LL);
        
        // Опциональные ID
        if (j.contains("sprint_id") && !j.at("sprint_id").is_null()) {
            c.sprint_id = get_int64_optional(j, "sprint_id", 0LL);
        }
        if (j.contains("service_id") && !j.at("service_id").is_null()) {
            c.service_id = get_int64_optional(j, "service_id", 0LL);
        }
        if (j.contains("depends_on_card_id") && !j.at("depends_on_card_id").is_null()) {
            c.depends_on_card_id = get_int64_optional(j, "depends_on_card_id", 0LL);
        }
        if (j.contains("import_id") && !j.at("import_id").is_null()) {
            c.import_id = get_int64_optional(j, "import_id", 0LL);
        }
        
        // Вложенные объекты
        if (j.contains("board") && !j.at("board").is_null()) j.at("board").get_to(c.board);
        if (j.contains("column") && !j.at("column").is_null()) j.at("column").get_to(c.column);
        if (j.contains("lane") && !j.at("lane").is_null()) j.at("lane").get_to(c.lane);
        if (j.contains("owner") && !j.at("owner").is_null()) j.at("owner").get_to(c.owner);
        if (j.contains("type") && !j.at("type").is_null() && j.at("type").is_object()) {
            j.at("type").get_to(c.card_type);
            c.type = c.card_type.name; // для обратной совместимости
        }
        if (j.contains("card_permissions") && !j.at("card_permissions").is_null()) {
            j.at("card_permissions").get_to(c.permissions);
        }
        if (j.contains("path_data") && !j.at("path_data").is_null()) {
            //j.at("path_data").get_to(c.path_data);
        }
        
        // Даты
        if (j.contains("created") && !j.at("created").is_null()) j.at("created").get_to(c.created);
        if (j.contains("updated") && !j.at("updated").is_null()) j.at("updated").get_to(c.updated);
        if (j.contains("column_changed_at") && !j.at("column_changed_at").is_null()) j.at("column_changed_at").get_to(c.column_changed_at);
        if (j.contains("lane_changed_at") && !j.at("lane_changed_at").is_null()) j.at("lane_changed_at").get_to(c.lane_changed_at);
        if (j.contains("last_moved_at") && !j.at("last_moved_at").is_null()) j.at("last_moved_at").get_to(c.last_moved_at);
        if (j.contains("last_moved_to_done_at") && !j.at("last_moved_to_done_at").is_null()) j.at("last_moved_to_done_at").get_to(c.last_moved_to_done_at);
        if (j.contains("first_moved_to_in_progress_at") && !j.at("first_moved_to_in_progress_at").is_null()) j.at("first_moved_to_in_progress_at").get_to(c.first_moved_to_in_progress_at);
        if (j.contains("completed_at") && !j.at("completed_at").is_null()) j.at("completed_at").get_to(c.completed_at);
        if (j.contains("counters_recalculated_at") && !j.at("counters_recalculated_at").is_null()) j.at("counters_recalculated_at").get_to(c.counters_recalculated_at);
        if (j.contains("comment_last_added_at") && !j.at("comment_last_added_at").is_null()) j.at("comment_last_added_at").get_to(c.comment_last_added_at);
        
        // Массивы
        if (j.contains("members") && j.at("members").is_array()) {
            c.members = j.at("members").get<std::vector<User>>();
        }
        if (j.contains("parents") && j.at("parents").is_array()) {
            c.parents = j.at("parents").get<std::vector<ParentCard>>();
        }
        if (j.contains("tags") && j.at("tags").is_array()) {
            c.tags = j.at("tags").get<std::vector<Tag>>();
        }
        if (j.contains("parents_ids") && j.at("parents_ids").is_array()) {
            c.parent_ids = j.at("parents_ids").get<std::vector<std::int64_t>>();
        }
        if (j.contains("children_ids") && j.at("children_ids").is_array()) {
            c.children_ids = j.at("children_ids").get<std::vector<std::int64_t>>();
        }
        if (j.contains("external_links") && j.at("external_links").is_array()) {
            c.external_links = j.at("external_links").get<std::vector<nlohmann::json>>();
        }
        if (j.contains("files") && j.at("files").is_array()) {
            c.files = j.at("files").get<std::vector<nlohmann::json>>();
        }
        if (j.contains("children") && j.at("children").is_array()) {
            c.children = j.at("children").get<std::vector<nlohmann::json>>();
        }
        
        // Строковые поля
        c.type = get_optional(j, "type", std::string());
        c.description = get_optional(j, "description", std::string());
        c.state = get_optional(j, "state", std::string());
        c.source = get_optional(j, "source", std::string());
        c.email = get_optional(j, "email", std::string());
        c.size_text = get_optional(j, "size_text", std::string());
        c.size_unit = get_optional(j, "size_unit", std::string());
        c.share_id = get_optional(j, "share_id", std::string());
        c.fts_version = get_optional(j, "fts_version", std::string());
        
        // Числовые поля
        c.size = get_int_optional(j, "size", 0);
        c.condition = get_int_optional(j, "condition", 0);
        c.cardRole = get_int_optional(j, "cardRole", 0);
        c.comments_total = get_int_optional(j, "comments_total", 0);
        c.children_count = get_int_optional(j, "children_count", 0);
        c.children_done = get_int_optional(j, "children_done", 0);
        c.goals_done = get_int_optional(j, "goals_done", 0);
        c.goals_total = get_int_optional(j, "goals_total", 0);
        c.estimate_workload = get_int_optional(j, "estimate_workload", 0);
        c.time_spent_sum = get_int_optional(j, "time_spent_sum", 0);
        c.time_blocked_sum = get_int_optional(j, "time_blocked_sum", 0);
        c.sort_order = get_double_optional(j, "sort_order", 0.0);
        
        // Опциональные числовые поля
        if (j.contains("fifo_order") && !j.at("fifo_order").is_null()) {
            c.fifo_order = get_double_optional(j, "fifo_order", 0.0);
        }
        
        // JSON объекты
        if (j.contains("properties") && !j.at("properties").is_null()) {
            c.properties = j.at("properties");
        }
        if (j.contains("share_settings") && !j.at("share_settings").is_null()) {
            c.share_settings = j.at("share_settings");
        }
        if (j.contains("children_number_properties_sum") && !j.at("children_number_properties_sum").is_null()) {
            c.children_number_properties_sum = j.at("children_number_properties_sum");
        }
        if (j.contains("parent_checklist_ids") && !j.at("parent_checklist_ids").is_null()) {
            c.parent_checklist_ids = j.at("parent_checklist_ids");
        }
        
        // Опциональные JSON объекты
        if (j.contains("external_id") && !j.at("external_id").is_null()) {
            c.external_id = j.at("external_id");
        }
        if (j.contains("external_user_emails") && !j.at("external_user_emails").is_null()) {
            c.external_user_emails = j.at("external_user_emails");
        }
        if (j.contains("calculated_planned_end") && !j.at("calculated_planned_end").is_null()) {
            c.calculated_planned_end = j.at("calculated_planned_end");
        }
        if (j.contains("calculated_planned_start") && !j.at("calculated_planned_start").is_null()) {
            c.calculated_planned_start = j.at("calculated_planned_start");
        }
        if (j.contains("planned_end") && !j.at("planned_end").is_null()) {
            c.planned_end = j.at("planned_end");
        }
        if (j.contains("planned_start") && !j.at("planned_start").is_null()) {
            c.planned_start = j.at("planned_start");
        }
        if (j.contains("completed_on_time") && !j.at("completed_on_time").is_null()) {
            c.completed_on_time = j.at("completed_on_time");
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error parsing Card JSON: " << e.what() << std::endl;
        // Выводим дополнительную информацию для отладки
        if (j.contains("id")) {
            std::cerr << "Card ID: " << j.at("id") << std::endl;
        }
        throw;
    }
}


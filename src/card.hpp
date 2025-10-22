// card.hpp
#ifndef CARD_HPP
#define CARD_HPP

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <optional>
#include <variant>

#include "config.hpp"
#include "date.hpp"


// Тип для значений свойств, соответствующий спецификации Kaiten
using PropertyValue = std::variant<
    std::monostate,  // для null
    int,
    double,
    std::string,
    std::vector<std::string>,
    nlohmann::json
>;

// Вспомогательная функция для сериализации PropertyValue в JSON
nlohmann::json property_value_to_json(const PropertyValue& value);


// Альтернативный упрощенный подход
struct Simple_card {
    std::int64_t id = 0;
    std::string number;
    std::string title;
    std::int64_t type_id = 0;
    std::int64_t size = 0;
    bool archived = false;
    std::int64_t board_id = 0;
    std::int64_t column_id = 0;
    std::int64_t lane_id = 0;
    Card_date created;
    Card_date updated;
    std::vector<std::string> tags;
    std::map<std::string, PropertyValue> properties;

    std::int64_t parent_id = 0;
    std::int64_t owner_id = 0;
    std::string owner_email;
    std::int64_t responsible_id = 0;

    std::vector<std::int64_t> members_id;
    std::string description;

    static constexpr std::string_view sprint_number_property = "id_12";
    static constexpr std::string_view role_id_property = "id_19";
    static constexpr std::string_view team_id_property = "id_143";


    Simple_card() = default;
    
    Simple_card(const Simple_card&) = default;
    
    // Конструктор с минимальной инициализацией из Config
    explicit Simple_card(const Config& config, const std::string& card_title = "")
        : title(card_title)
        , type_id(config.taskTypeId)
        , size(config.taskSize)
        , board_id(config.boardId)
        , column_id(config.columnId)
        , lane_id(config.laneId)
    {
        // Инициализация тегов из конфигурации
        if (!config.tags.empty()) {
            tags = config.tags;
        }
        
        // Установка роли из конфигурации
        if (!config.role.empty()) {
            set_role_id(config.role);
        }
    }

    // Оператор присвоения
    Simple_card& operator=(const Simple_card& other) {
        if (this != &other) { // защита от самоприсваивания
            // Копируем простые поля
            id = other.id;
            number = other.number;
            title = other.title;
            type_id = other.type_id;
            size = other.size;
            archived = other.archived;
            board_id = other.board_id;
            column_id = other.column_id;
            lane_id = other.lane_id;
            parent_id = other.parent_id;
            owner_id = other.owner_id;
            owner_email = other.owner_email;
            responsible_id = other.responsible_id;
            description = other.description;
            
            // Копируем контейнеры
            tags = other.tags;
            members_id = other.members_id;
            properties = other.properties; // std::map имеет правильный оператор=
            
            // Копируем даты
            created = other.created;
            updated = other.updated;
        }
        return *this;
    }

    // Оператор присвоения из Config
    Simple_card& operator=(const Config& config) {
        // Обновляем только конфигурируемые поля, не трогая идентификаторы и состояние
        board_id = config.boardId;
        column_id = config.columnId;
        lane_id = config.laneId;
        type_id = config.taskTypeId;
        
        // Размер обновляем только если в конфиге задан ненулевой
        if (config.taskSize > 0) {
            size = config.taskSize;
        }
        
        // Теги заменяем полностью на теги из конфигурации
        if (!config.tags.empty()) {
            tags = config.tags;
        }
        
        // Устанавливаем роль из конфигурации
        if (!config.role.empty()) {
            set_role_id(config.role);
        }
        
        return *this;
    }

    // Комбинированный оператор присвоения (Config + заголовок)
    Simple_card& assign_from_config(const Config& config, const std::string& card_title = "") {
        // Применяем оператор присвоения из Config
        *this = config;
        
        // Устанавливаем заголовок если передан
        if (!card_title.empty()) {
            title = card_title;
        }
        
        return *this;
    }

    // Методы для инициализации из Config
    void init_from_config(const Config& config, const std::string& card_title = "") {
        *this = config; // используем оператор присвоения
        if (!card_title.empty()) {
            title = card_title;
        }
    }

    // Инициализация только обязательных полей из Config
    void init_required_from_config(const Config& config) {
        board_id = config.boardId;
        column_id = config.columnId;
        lane_id = config.laneId;
        type_id = config.taskTypeId;
    }

    // Установка контента (заголовок, размер, описание)
    void set_content(const std::string& card_title, int card_size = 0, 
                    const std::string& card_description = "") {
        title = card_title;
        if (card_size > 0) {
            size = card_size;
        }
        if (!card_description.empty()) {
            description = card_description;
        }
    }

    // Добавление тегов с дедупликацией
    void add_tags(const std::vector<std::string>& new_tags) {
        tags.insert(tags.end(), new_tags.begin(), new_tags.end());
        deduplicate_tags();
    }

    void add_tag(const std::string& tag) {
        tags.push_back(tag);
        deduplicate_tags();
    }

    // Удаление дубликатов тегов
    void deduplicate_tags() {
        std::sort(tags.begin(), tags.end());
        tags.erase(std::unique(tags.begin(), tags.end()), tags.end());
    }

    // Создание копии с применением конфигурации
    Simple_card create_with_config(const Config& config, const std::string& new_title = "") const {
        Simple_card result = *this; // использует конструктор копирования
        
        // Применяем конфигурацию
        result.assign_from_config(config, new_title);
        
        return result;
    }

    // Утилитарные методы для проверки состояния
    bool is_configured() const {
        return board_id > 0 && column_id > 0 && lane_id > 0 && type_id > 0;
    }
    
    bool has_required_fields() const {
        return !title.empty() && is_configured();
    }
    
    void clear_content() {
        title.clear();
        description.clear();
        tags.clear();
        properties.clear();
        size = 0;
    }
    
    void clear_configuration() {
        board_id = 0;
        column_id = 0;
        lane_id = 0;
        type_id = 0;
        size = 0;
        tags.clear();
        properties.clear();
    }


    // GetSprintNumber extracts sprint number from property id_12
    std::optional<std::string> get_sprint_number() const
    {
        return get_property_as_string(sprint_number_property);
    }
    
    void set_sprint_number(const std::string& value) {
        // Пробуем преобразовать в число, если это возможно
        try {
            if (!value.empty()) {
                int num_value = std::stoi(value);
                properties[std::string(sprint_number_property)] = num_value;
            } else {
                // Если пустая строка, устанавливаем null
                properties[std::string(sprint_number_property)] = std::monostate{};
            }
        } catch (const std::exception&) {
            // Если не число, сохраняем как строку
            properties[std::string(sprint_number_property)] = value;
        }
    }

    void clear_sprint_number() {
        properties[std::string(sprint_number_property)] = std::monostate{};
    }

    // GetRoleID extracts role ID from property id_19
    std::optional<std::string> get_role_id() const
    {
        return get_property_as_string(role_id_property);
    }
    void set_role_id(const std::string& role) {
        std::string code("1");
        std::map<std::string, std::string> roles {
            { "C++", "1" },
            { "Backend", "2" },
            { "Java", "2" },
            { "Frontend", "3" },
            { "React", "3" },
            { "Test", "4" },
            { "Analyst", "8" },
            { "UIUX", "9" },
            { "DevOps", "11" },
            { "Writer", "12" },
            { "Approbation", "20" }
        };

        auto it = roles.find(role);
        if (it != roles.end()) {
            code = it->second;
        }
        
        set_property_string(std::string(role_id_property), code);
        // // Сохраняем как число
        // try {
        //     int num_value = std::stoi(code);
        //     properties[std::string(role_id_property)] = num_value;
        // } catch (const std::exception&) {
        //     properties[std::string(role_id_property)] = code;
        // }
    }

    std::optional<std::string> get_team_id() const
    {
        return get_property_as_string(team_id_property);
    }
    
    void set_team_id(const std::string& value) {
        try {
            if (!value.empty()) {
                int num_value = std::stoi(value);
                properties[std::string(team_id_property)] = num_value;
            } else {
                properties[std::string(team_id_property)] = std::monostate{};
            }
        } catch (const std::exception&) {
            properties[std::string(team_id_property)] = value;
        }
    }

    // Универсальные методы для работы со свойствами
    void set_property(const std::string& key, const PropertyValue& value) {
        properties[key] = value;
    }
    
    void set_property_string(const std::string& key, const std::string& value) {
        properties[key] = value;
    }
    
    void set_property_number(const std::string& key, int value) {
        properties[key] = value;
    }
    
    void set_property_double(const std::string& key, double value) {
        properties[key] = value;
    }
    
    void set_property_null(const std::string& key) {
        properties[key] = std::monostate{};
    }
    
    void set_property_array(const std::string& key, const std::vector<std::string>& value) {
        properties[key] = value;
    }
    
    void set_property_object(const std::string& key, const nlohmann::json& value) {
        properties[key] = value;
    }

private:
    std::optional<std::string> get_property_as_string(std::string_view prop) const
    {
        auto it = properties.find(std::string(prop));
        if (it != properties.end()) {
            return property_value_to_string(it->second);
        }
        return std::nullopt;
    }
    
    static std::string property_value_to_string(const PropertyValue& value) {
        return std::visit([](auto&& arg) -> std::string {
            using T = std::decay_t<decltype(arg)>;
            
            if constexpr (std::is_same_v<T, std::monostate>) {
                return "";
            } else if constexpr (std::is_same_v<T, int>) {
                return std::to_string(arg);
            } else if constexpr (std::is_same_v<T, double>) {
                return std::to_string(arg);
            } else if constexpr (std::is_same_v<T, std::string>) {
                return arg;
            } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                // Для массива возвращаем первый элемент или пустую строку
                return arg.empty() ? "" : arg[0];
            } else if constexpr (std::is_same_v<T, nlohmann::json>) {
                return arg.is_string() ? arg.template get<std::string>() : arg.dump();
            } else {
                return "";
            }
        }, value);
    }
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
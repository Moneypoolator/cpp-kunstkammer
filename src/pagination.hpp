#ifndef PAGINATION_HPP
#define PAGINATION_HPP

#include <string>
#include <map>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace kaiten {

// Параметры пагинации
struct Pagination_params {
    int page = 1;
    int per_page = 100;
    int offset = 0;
    int limit = 100;
    std::string sort_by;
    std::string sort_order; // "asc" or "desc"
    
    Pagination_params() = default;
    explicit Pagination_params(int page_size) : per_page(page_size), limit(page_size) {}
};

// Параметры фильтрации карточек
struct Card_filter_params {
    std::optional<std::int64_t> board_id;
    std::optional<std::int64_t> lane_id;
    std::optional<std::int64_t> column_id;
    std::optional<std::int64_t> owner_id;
    std::optional<std::int64_t> member_id;
    std::optional<std::int64_t> type_id;
    std::optional<std::string> type_name;
    std::optional<std::string> state;
    std::optional<std::string> number;
    std::optional<bool> archived;
    std::optional<bool> blocked;
    std::optional<bool> asap;
    std::optional<std::string> search; // полнотекстовый поиск
    std::optional<std::string> created_after;
    std::optional<std::string> created_before;
    std::optional<std::string> updated_after;
    std::optional<std::string> updated_before;
    
    // Дополнительные параметры
    std::map<std::string, std::string> custom_filters;
};

// Параметры фильтрации пользователей
struct User_filter_params {
    std::optional<bool> activated;
    std::optional<bool> virtual_user;
    std::optional<std::string> search; // поиск по имени/email
};

// Результат пагинации с метаданными
template<typename T>
struct Paginated_result {
    std::vector<T> items;
    int total_count = 0;
    int page = 1;
    int per_page = 0;
    int total_pages = 0;
    bool has_more = false;
    std::optional<std::string> next_page_url;
    std::optional<std::string> prev_page_url;
};

// Утилиты для построения query string
class Query_builder {
public:
    static std::string build(const Pagination_params& pagination, 
                            const Card_filter_params& filters = {}) {
        std::stringstream ss;
        ss << "?per_page=" << pagination.per_page;
        
        if (pagination.page > 1) {
            ss << "&page=" << pagination.page;
        }
        
        if (!pagination.sort_by.empty()) {
            ss << "&sort=" << pagination.sort_by;
            if (!pagination.sort_order.empty()) {
                ss << "&order=" << pagination.sort_order;
            }
        }
        
        // Добавляем фильтры
        add_filter(ss, "board_id", filters.board_id);
        add_filter(ss, "lane_id", filters.lane_id);
        add_filter(ss, "column_id", filters.column_id);
        add_filter(ss, "owner_id", filters.owner_id);
        add_filter(ss, "member_id", filters.member_id);
        add_filter(ss, "type_id", filters.type_id);
        add_filter(ss, "type", filters.type_name);
        add_filter(ss, "state", filters.state);
        add_filter(ss, "number", filters.number);
        add_filter(ss, "archived", filters.archived);
        add_filter(ss, "blocked", filters.blocked);
        add_filter(ss, "asap", filters.asap);
        add_filter(ss, "search", filters.search);
        add_filter(ss, "created_after", filters.created_after);
        add_filter(ss, "created_before", filters.created_before);
        add_filter(ss, "updated_after", filters.updated_after);
        add_filter(ss, "updated_before", filters.updated_before);
        
        // Кастомные фильтры
        for (const auto& [key, value] : filters.custom_filters) {
            ss << "&" << key << "=" << value;
        }
        
        return ss.str();
    }
    
    static std::string build(const Pagination_params& pagination,
                            const User_filter_params& filters) {
        std::stringstream ss;
        ss << "?per_page=" << pagination.per_page;
        
        if (pagination.page > 1) {
            ss << "&page=" << pagination.page;
        }
        
        add_filter(ss, "activated", filters.activated);
        add_filter(ss, "virtual", filters.virtual_user);
        add_filter(ss, "search", filters.search);
        
        return ss.str();
    }

private:
    template<typename T>
    static void add_filter(std::stringstream& ss, const std::string& key, 
                          const std::optional<T>& value) {
        if (value.has_value()) {
            ss << "&" << key << "=";
            if constexpr (std::is_same_v<T, bool>) {
                ss << (*value ? "true" : "false");
            } else {
                ss << *value;
            }
        }
    }
};

} // namespace kaiten

#endif // PAGINATION_HPP
#include "board_operations.hpp"

// #include <iostream>
#include <string>
// #include <utility>
#include <vector>
#include <nlohmann/json.hpp>

// #include "cache.hpp"
#include "card.hpp"
// #include "card_utils.hpp"
#include "http_client.hpp"
#include "pagination.hpp"
#include "error_handler.hpp"
// #include "api_utils.hpp"

namespace kaiten {
namespace board_operations {


// Boards pagination with correct implementation
Paginated_result<Board> get_boards_paginated(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
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

    auto [status, response] = client.get(host, port, target, token);

    if (status != 200) {
        auto error = kaiten::error_handler::handle_http_error(status, response, "fetch boards");
        kaiten::error_handler::log_error(error, "get_boards_paginated");
        return result;
    }

    try {
        auto json = nlohmann::json::parse(response);

        if (json.is_array()) {
            result.items = json.get<std::vector<Board>>();
        } else {
            auto error = kaiten::error_handler::Error_info{
                kaiten::error_handler::Error_category::API,
                status,
                "Unexpected response format for boards",
                "Response is not an array",
                "Check the API response format and try again",
                json.dump(2)
            };
            kaiten::error_handler::log_error(error, "get_boards_paginated");
            return result;
        }

        result.limit = safe_pagination.limit;
        result.offset = safe_pagination.offset;
        result.has_more = result.items.size() == static_cast<size_t>(safe_pagination.limit);

    } catch (const std::exception& e) {
        auto error = kaiten::error_handler::handle_parsing_error(e, "boards paginated response", response);
        kaiten::error_handler::log_error(error, "get_boards_paginated");
    }

    return result;
}

} // namespace board_operations
} // namespace kaiten
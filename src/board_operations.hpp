#ifndef BOARD_OPERATIONS_HPP
#define BOARD_OPERATIONS_HPP

#include <string>
#include <vector>
#include <utility>

#include "card.hpp"
#include "http_client.hpp"
#include "pagination.hpp"

namespace kaiten {
namespace board_operations {

// Board operations
Paginated_result<Board> get_boards_paginated(
    Http_client& client,
    const std::string& host,
    const std::string& port, 
    const std::string& api_path,
    const std::string& token,
    const Pagination_params& pagination = {});

} // namespace board_operations
} // namespace kaiten

#endif // BOARD_OPERATIONS_HPP
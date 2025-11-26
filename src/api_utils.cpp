#include "api_utils.hpp"

namespace kaiten {
namespace api_utils {

    void log_api_error(const char* action, int status, const std::string& response) {
        auto error = kaiten::error_handler::parse_api_error(status, response, action);
        kaiten::error_handler::log_error(error);
    }

} // namespace api_utils
} // namespace kaiten
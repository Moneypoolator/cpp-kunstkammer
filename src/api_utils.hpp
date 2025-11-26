#ifndef KAITEN_API_UTILS_HPP
#define KAITEN_API_UTILS_HPP

#include <string>
#include "error_handler.hpp"

namespace kaiten {
namespace api_utils {

    // Common function to log API errors
    void log_api_error(const char* action, int status, const std::string& response);

} // namespace api_utils
} // namespace kaiten

#endif // KAITEN_API_UTILS_HPP
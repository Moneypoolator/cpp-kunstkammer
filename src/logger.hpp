#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <memory>
#include <string>

namespace kaiten {

/**
 * @brief Logging utility for the Kunstkammer project
 * 
 * Provides a centralized logging system using spdlog with support for
 * both console and file output, configurable log levels, and easy-to-use
 * logging macros.
 */
class Logger {
public:
    /**
     * @brief Initialize the logger system
     * 
     * @param log_level Minimum log level to output (trace, debug, info, warn, error, critical)
     * @param log_to_console Whether to output logs to console
     * @param log_to_file Whether to output logs to file
     * @param log_file_path Path to log file (if logging to file)
     * @param max_file_size Maximum size of log file before rotation (in bytes)
     * @param max_files Maximum number of rotated files to keep
     */
    static void init(const std::string& log_level = "info",
                     bool log_to_console = true,
                     bool log_to_file = false,
                     const std::string& log_file_path = "kunstkammer.log",
                     size_t max_file_size = 5 * 1024 * 1024, // 5 MB
                     size_t max_files = 3);

    /**
     * @brief Get the global logger instance
     * 
     * @return std::shared_ptr<spdlog::logger> Logger instance
     */
    static std::shared_ptr<spdlog::logger> get();

    /**
     * @brief Set the minimum log level
     * 
     * @param level Log level string (trace, debug, info, warn, error, critical)
     */
    static void set_level(const std::string& level);

    /**
     * @brief Flush all pending log messages
     */
    static void flush();

    /**
     * @brief Check if logger is initialized
     * 
     * @return true if initialized, false otherwise
     */
    static bool is_initialized() { return s_initialized; }

    /**
     * @brief Shutdown the logger system
     */
    static void shutdown();

private:
    static std::shared_ptr<spdlog::logger> s_logger;
    static bool s_initialized;

    static spdlog::level::level_enum string_to_level(const std::string& level);
};

// Convenience macros for logging
#define LOG_TRACE(...)      if (spdlog::get("kunstkammer") != nullptr) spdlog::get("kunstkammer")->trace(__VA_ARGS__)
#define LOG_DEBUG(...)      if (spdlog::get("kunstkammer") != nullptr) spdlog::get("kunstkammer")->debug(__VA_ARGS__)
#define LOG_INFO(...)       if (spdlog::get("kunstkammer") != nullptr) spdlog::get("kunstkammer")->info(__VA_ARGS__)
#define LOG_WARN(...)       if (spdlog::get("kunstkammer") != nullptr) spdlog::get("kunstkammer")->warn(__VA_ARGS__)
#define LOG_ERROR(...)      if (spdlog::get("kunstkammer") != nullptr) spdlog::get("kunstkammer")->error(__VA_ARGS__)
#define LOG_CRITICAL(...)   if (spdlog::get("kunstkammer") != nullptr) spdlog::get("kunstkammer")->critical(__VA_ARGS__)

// Macros with source location (file and line)
#define LOG_TRACE_AT(...)   if (spdlog::get("kunstkammer") != nullptr) spdlog::get("kunstkammer")->trace(__VA_ARGS__)
#define LOG_DEBUG_AT(...)   if (spdlog::get("kunstkammer") != nullptr) spdlog::get("kunstkammer")->debug(__VA_ARGS__)
#define LOG_INFO_AT(...)    if (spdlog::get("kunstkammer") != nullptr) spdlog::get("kunstkammer")->info(__VA_ARGS__)
#define LOG_WARN_AT(...)    if (spdlog::get("kunstkammer") != nullptr) spdlog::get("kunstkammer")->warn(__VA_ARGS__)
#define LOG_ERROR_AT(...)   if (spdlog::get("kunstkammer") != nullptr) spdlog::get("kunstkammer")->error(__VA_ARGS__)
#define LOG_CRITICAL_AT(...) if (spdlog::get("kunstkammer") != nullptr) spdlog::get("kunstkammer")->critical(__VA_ARGS__)

// Conditional logging macros
#define LOG_IF(condition, level, ...) \
    if ((condition) && spdlog::get("kunstkammer") != nullptr) spdlog::get("kunstkammer")->level(__VA_ARGS__)

} // namespace kaiten
#include "logger.hpp"
#include <iostream>

namespace kaiten {

// Static member initialization
std::shared_ptr<spdlog::logger> Logger::s_logger = nullptr;
bool Logger::s_initialized = false;

void Logger::init(const std::string& log_level,
                  bool log_to_console,
                  bool log_to_file,
                  const std::string& log_file_path,
                  size_t max_file_size,
                  size_t max_files) {
    if (s_initialized) {
        // Already initialized, just update level if needed
        set_level(log_level);
        return;
    }

    try {
        std::vector<spdlog::sink_ptr> sinks;

        // Console sink
        if (log_to_console) {
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] %v");
            sinks.push_back(console_sink);
        }

        // File sink (with rotation)
        if (log_to_file) {
            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                log_file_path, max_file_size, max_files);
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%s:%#] [thread %t] %v");
            sinks.push_back(file_sink);
        }

        // Create logger with all sinks
        s_logger = std::make_shared<spdlog::logger>("kunstkammer", sinks.begin(), sinks.end());
        
        // Set log level
        set_level(log_level);
        
        // Register logger globally
        spdlog::register_logger(s_logger);
        
        // Set flush policy
        s_logger->flush_on(spdlog::level::warn);
        
        s_initialized = true;
        
        LOG_INFO("Logger initialized with level: {}", log_level);
        if (log_to_file) {
            LOG_INFO("Logging to file: {}", log_file_path);
        }
    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
        // Fallback to basic console logger
        s_logger = spdlog::stdout_color_mt("kunstkammer");
        s_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
        set_level(log_level);
        s_initialized = true;
        LOG_WARN("Using fallback logger due to initialization error");
    }
}

std::shared_ptr<spdlog::logger> Logger::get() {
    if (!s_initialized) {
        // Initialize with default settings if not already initialized
        init();
    }
    return s_logger;
}

void Logger::set_level(const std::string& level) {
    if (!s_initialized) {
        return;
    }
    
    spdlog::level::level_enum level_enum = string_to_level(level);
    s_logger->set_level(level_enum);
    
    // Also set global level for any other loggers
    spdlog::set_level(level_enum);
}

void Logger::flush() {
    if (s_initialized) {
        s_logger->flush();
    }
}

void Logger::shutdown() {
    if (s_initialized) {
        LOG_INFO("Shutting down logger");
        flush();
        spdlog::drop("kunstkammer");
        s_logger = nullptr;
        s_initialized = false;
    }
}

spdlog::level::level_enum Logger::string_to_level(const std::string& level)
{
    if (level == "trace") {
        return spdlog::level::trace;
    }
    if (level == "debug") {
        return spdlog::level::debug;
    }
    if (level == "info") {
        return spdlog::level::info;
    }
    if (level == "warn") {
        return spdlog::level::warn;
    }
    if (level == "error") {
        return spdlog::level::err;
    }
    if (level == "critical") {
        return spdlog::level::critical;
    }

    // Default to info level
    LOG_WARN("Unknown log level '{}', defaulting to 'info'", level);
    return spdlog::level::info;
}

} // namespace kaiten
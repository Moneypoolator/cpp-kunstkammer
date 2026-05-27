# Logging with spdlog in Kunstkammer

## Overview

The Kunstkammer project now uses [spdlog](https://github.com/gabime/spdlog) as its logging library. This provides:
- Fast, asynchronous logging
- Multiple log levels (trace, debug, info, warn, error, critical)
- Configurable output (console, file, or both)
- Thread-safe operations
- Structured logging with source location

## Quick Start

### Basic Usage

Include the logger header and use the provided macros:

```cpp
#include "logger.hpp"

// Initialize logger (typically in main.cpp)
kaiten::Logger::init("info", true, false); // level=info, console=true, file=false

// Log messages at different levels
LOG_INFO("Application started");
LOG_DEBUG("Processing card {}", card_id);
LOG_ERROR("Failed to connect to API: {}", error_message);
```

### Log Levels

The available log levels (from most verbose to least):
- `trace`: Detailed tracing information
- `debug`: Debugging information
- `info`: General information about application progress
- `warn`: Warning conditions
- `error`: Error conditions
- `critical`: Critical conditions requiring immediate attention

### Configuration

Initialize the logger with custom settings:

```cpp
// Initialize with file logging
kaiten::Logger::init(
    "debug",                    // log level
    true,                       // log to console
    true,                       // log to file
    "kunstkammer.log",          // log file path
    5 * 1024 * 1024,            // max file size (5 MB)
    3                           // max rotated files to keep
);
```

## Integration with Existing Code

### Error Handler

The `error_handler.cpp` has been updated to use spdlog. Error categories are mapped to appropriate log levels:
- Network/API errors: `LOG_ERROR`
- Authentication errors: `LOG_WARN`
- Unknown errors: `LOG_CRITICAL`

### HTTP Client

The `http_client.cpp` now uses appropriate log levels:
- Request/response details: `LOG_DEBUG`
- Headers and body: `LOG_TRACE`
- Errors: `LOG_ERROR`

## Migration Guide

### Replacing std::cout/std::cerr

| Old Code | New Code |
|----------|----------|
| `std::cout << "Message" << std::endl;` | `LOG_INFO("Message");` |
| `std::cerr << "Error: " << e.what() << std::endl;` | `LOG_ERROR("Error: {}", e.what());` |
| `std::cout << "Value: " << value << std::endl;` | `LOG_DEBUG("Value: {}", value);` |

### Conditional Logging

```cpp
// Old
if (debug_mode) {
    std::cout << "Debug info: " << data << std::endl;
}

// New
LOG_IF(debug_mode, debug, "Debug info: {}", data);
```

## Advanced Features

### Source Location

The logger automatically includes source file and line number in the output format.

### Performance Considerations

- By default, logs are written synchronously
- For high-performance scenarios, consider enabling async logging (future enhancement)
- `LOG_TRACE` and `LOG_DEBUG` statements are compiled out in release builds when level is set higher

### Customizing Output Format

The log format can be customized in `logger.cpp`:
- Console format: `[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] %v`
- File format: `[%Y-%m-%d %H:%M:%S.%e] [%l] [%s:%#] [thread %t] %v`

## Build Configuration

The logger is automatically included in the build:
- Source file: `src/logger.cpp`
- Header file: `src/logger.hpp`
- CMake: Linked with `spdlog::spdlog`

## Testing

To test the logging system:

1. Set log level to "trace" for maximum verbosity
2. Run the application with different operations
3. Check console output and log file (if enabled)

Example test command:
```bash
./kunstkammer --mode get-card --card-number 123 --log-level debug
```

## Troubleshooting

### Common Issues

1. **No log output**: Ensure logger is initialized before use
2. **Missing spdlog**: Verify conan dependency `spdlog/1.17.0` is installed
3. **Compilation errors**: Check CMake links `spdlog::spdlog` to all targets

### Debugging

Enable trace level logging for maximum detail:
```cpp
kaiten::Logger::init("trace", true, false);
```

## Future Enhancements

Potential improvements:
1. Async logging for better performance
2. Log rotation by time (daily, hourly)
3. Network logging (send logs to remote server)
4. Structured logging (JSON format)
5. Log filtering by module/component
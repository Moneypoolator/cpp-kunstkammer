cmake_minimum_required(VERSION 3.15)
project(kunstkammer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Политика для FindBoost
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# Поиск зависимостей
find_package(Boost 1.71.0 REQUIRED COMPONENTS program_options system)
find_package(OpenSSL REQUIRED)

# Настройка компилятора
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Общие флаги для всех конфигураций
    add_compile_options(-Wall -Wextra -pedantic)
    
    # Специфичные флаги для Debug
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG")
    
    # Специфичные флаги для Release
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
    
    # Специфичные флаги для RelWithDebInfo
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O2 -g -DNDEBUG")
    
    # Специфичные флаги для MinSizeRel
    set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -Os -DNDEBUG")
    
elseif(MSVC)
    # Настройки для Visual Studio
    add_compile_options(/W4)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd /Zi /Ob0 /Od /RTC1")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD /O2 /Ob2 /DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MD /Zi /O2 /Ob1 /DNDEBUG")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MD /O1 /Ob1 /DNDEBUG")
endif()

# Настройка типов конфигураций по умолчанию
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build" FORCE)
    # Возможные значения: Debug Release RelWithDebInfo MinSizeRel
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

# Вывод информации о конфигурации
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Boost version: ${Boost_VERSION}")
message(STATUS "Boost includes: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost libraries: ${Boost_LIBRARIES}")


# Исходные файлы
set(SOURCES
    src/main.cpp
    src/http_client.cpp
    src/modes.cpp
    src/card.cpp
    src/card_utils.cpp
    src/kaiten.cpp
    src/rate_limiter.cpp
)

add_executable(kunstkammer ${SOURCES})

# Подключение заголовков
target_include_directories(kunstkammer PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${Boost_INCLUDE_DIRS}
)

# Линковка
target_link_libraries(kunstkammer PRIVATE 
    Boost::program_options 
    Boost::system 
    OpenSSL::SSL 
    OpenSSL::Crypto
)


# Дополнительные настройки для Release сборки
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Release build: optimizing for performance")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # Дополнительные оптимизации для GCC/Clang
        target_compile_options(kunstkammer PRIVATE -fomit-frame-pointer -funroll-loops)
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(kunstkammer PRIVATE -fno-semantic-interposition)
        endif()
    endif()
    
    # Установка свойств для уменьшения размера бинарника
    set_target_properties(kunstkammer PROPERTIES
        LINK_FLAGS "-s"  # Удаление символов (для GCC)
    )
endif()

# Дополнительные настройки для Debug сборки
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Debug build: including debug symbols and disabling optimizations")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(kunstkammer PRIVATE -fno-omit-frame-pointer)
    endif()
endif()

# Установка целевых файлов
install(TARGETS kunstkammer DESTINATION bin)

# Пользовательские цели для удобства
add_custom_target(build-debug
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --config Debug
    COMMENT "Building Debug version"
)

add_custom_target(build-release
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --config Release
    COMMENT "Building Release version"
)

add_custom_target(build-all
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --config Debug
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --config Release
    COMMENT "Building both Debug and Release versions"
)

# Информация о доступных целях сборки
message(STATUS "Available build targets:")
message(STATUS "  make kunstkammer      - Build with current configuration")
message(STATUS "  make build-debug      - Build Debug version")
message(STATUS "  make build-release    - Build Release version")
message(STATUS "  make build-all        - Build both Debug and Release")


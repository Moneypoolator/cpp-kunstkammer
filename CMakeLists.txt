cmake_minimum_required(VERSION 3.10)
project(kunstkammer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Поиск зависимостей
find_package(Boost 1.71.0 REQUIRED COMPONENTS program_options system)
find_package(OpenSSL REQUIRED)
#find_package(nlohmann_json 3.9.0 REQUIRED)

# Настройка компилятора
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# Исходные файлы
set(SOURCES
    src/main.cpp
    src/http_client.cpp
    src/modes.cpp
    src/card.cpp
    #src/kaiten.cpp  # если вы вынесете реализации из hpp
)

add_executable(kunstkammer ${SOURCES})

# Подключение заголовков
target_include_directories(kunstkammer PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Линковка
target_link_libraries(kunstkammer PRIVATE 
    Boost::program_options 
    Boost::system 
    OpenSSL::SSL 
    OpenSSL::Crypto
#    nlohmann_json::nlohmann_json
)

# Для Boost.Beast (header-only)
target_include_directories(kunstkammer PRIVATE 
    ${Boost_INCLUDE_DIRS}
)

# Установка целевых файлов
install(TARGETS kunstkammer DESTINATION bin)
cmake_minimum_required(VERSION 3.10)
project(kunstkammer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost 1.71.0 REQUIRED COMPONENTS program_options system json beast)
find_package(OpenSSL REQUIRED)

add_executable(kunstkammer src/main.cpp src/http_client.cpp src/modes.cpp)

target_include_directories(kunstkammer PRIVATE third_party)

target_link_libraries(kunstkammer PRIVATE Boost::program_options Boost::system Boost::json Boost::beast OpenSSL::SSL OpenSSL::Crypto) 
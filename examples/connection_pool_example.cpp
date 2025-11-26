#include "../src/connection_pool.hpp"
#include <iostream>
// #include <thread>
// #include <vector>
// #include <chrono>

int main() {
    std::cout << "Connection Pool Example" << std::endl;
    
    // Initialize Boost ASIO context and SSL context
    net::io_context ioc;
    ssl::context ctx(ssl::context::tlsv12_client);
    ctx.set_verify_mode(ssl::verify_none);
    
    // Create connection pool
    auto pool = std::make_shared<kaiten::Connection_pool>(ioc, ctx, 5);
    
    // Show initial pool stats
    auto stats = pool->get_stats();
    std::cout << "Initial pool stats - Active: " << stats.active_connections 
              << ", Idle: " << stats.idle_connections 
              << ", Total: " << stats.total_connections << std::endl;
    
    // Try to get connections (these will fail because we're not actually connecting to a server)
    std::cout << "\nGetting connections..." << std::endl;
    
    auto result1 = pool->get_connection("localhost", "8080");
    auto result2 = pool->get_connection("localhost", "8080");
    auto result3 = pool->get_connection("localhost", "8080");
    
    // Show pool stats after getting connections
    stats = pool->get_stats();
    std::cout << "Pool stats after getting connections - Active: " << stats.active_connections 
              << ", Idle: " << stats.idle_connections 
              << ", Total: " << stats.total_connections << std::endl;
    
    // Return connections to pool (this will happen automatically when they go out of scope)
    result1.second.reset();
    result2.second.reset();
    result3.second.reset();
    
    // Show pool stats after returning connections
    stats = pool->get_stats();
    std::cout << "Pool stats after returning connections - Active: " << stats.active_connections 
              << ", Idle: " << stats.idle_connections 
              << ", Total: " << stats.total_connections << std::endl;
    
    std::cout << "Example completed!" << std::endl;
    
    return 0;
}
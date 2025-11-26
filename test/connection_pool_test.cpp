#include <gtest/gtest.h>
#include "../src/connection_pool.hpp"
// #include "../src/http_client.hpp"
#include <boost/asio.hpp>

namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

class Connection_pool_test : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize Boost ASIO context and SSL context
        _ioc = std::make_unique<net::io_context>();
        _ctx = std::make_unique<ssl::context>(ssl::context::tlsv12_client);
        _ctx->set_verify_mode(ssl::verify_none);
    }

    void TearDown() override {
        // Clean up
        _ioc.reset();
        _ctx.reset();
    }

    std::unique_ptr<net::io_context> _ioc;
    std::unique_ptr<ssl::context> _ctx;
};

TEST_F(Connection_pool_test, Initialization) {
    // Create connection pool
    auto pool = std::make_shared<kaiten::Connection_pool>(*_ioc, *_ctx, 5);
    
    // Check initial stats
    auto stats = pool->get_stats();
    EXPECT_EQ(stats.active_connections, 0);
    EXPECT_EQ(stats.idle_connections, 0);
    EXPECT_EQ(stats.total_connections, 0);
}

TEST_F(Connection_pool_test, GetConnection) {
    // Create connection pool
    auto pool = std::make_shared<kaiten::Connection_pool>(*_ioc, *_ctx, 5);
    
    // Try to get a connection (this will fail because we're not actually connecting to a server)
    auto result = pool->get_connection("localhost", "80");
    
    // We expect this to fail since we're not actually connecting to a server
    // but we can still check that the pool is working correctly
    EXPECT_EQ(result.first, -1); // Connection failed
    
    // Check stats
    auto stats = pool->get_stats();
    EXPECT_EQ(stats.active_connections, 0);
    EXPECT_EQ(stats.idle_connections, 0);
    EXPECT_EQ(stats.total_connections, 0);
}

TEST_F(Connection_pool_test, PoolStatistics) {
    // Create connection pool
    auto pool = std::make_shared<kaiten::Connection_pool>(*_ioc, *_ctx, 3);
    
    // Check initial stats
    auto stats = pool->get_stats();
    EXPECT_EQ(stats.active_connections, 0);
    EXPECT_EQ(stats.idle_connections, 0);
    EXPECT_EQ(stats.total_connections, 0);
    
    // Try to get connections
    auto result1 = pool->get_connection("localhost", "80");
    auto result2 = pool->get_connection("localhost", "80");
    
    // Check stats after attempting to get connections
    stats = pool->get_stats();
    EXPECT_EQ(stats.active_connections, 0); // Still 0 because connections failed
    EXPECT_EQ(stats.idle_connections, 0);
    EXPECT_EQ(stats.total_connections, 0);
}
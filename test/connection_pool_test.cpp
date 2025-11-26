#include <gtest/gtest.h>
#include "../src/connection_pool.hpp"
#include "../src/http_client.hpp"
#include <boost/asio.hpp>

namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

class ConnectionPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize Boost ASIO context and SSL context
        ioc_ = std::make_unique<net::io_context>();
        ctx_ = std::make_unique<ssl::context>(ssl::context::tlsv12_client);
        ctx_->set_verify_mode(ssl::verify_none);
    }

    void TearDown() override {
        // Clean up
        ioc_.reset();
        ctx_.reset();
    }

    std::unique_ptr<net::io_context> ioc_;
    std::unique_ptr<ssl::context> ctx_;
};

TEST_F(ConnectionPoolTest, Initialization) {
    // Create connection pool
    auto pool = std::make_shared<kaiten::Connection_pool>(*ioc_, *ctx_, 5);
    
    // Check initial stats
    auto stats = pool->get_stats();
    EXPECT_EQ(stats.active_connections, 0);
    EXPECT_EQ(stats.idle_connections, 0);
    EXPECT_EQ(stats.total_connections, 0);
}

TEST_F(ConnectionPoolTest, GetConnection) {
    // Create connection pool
    auto pool = std::make_shared<kaiten::Connection_pool>(*ioc_, *ctx_, 5);
    
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

TEST_F(ConnectionPoolTest, PoolStatistics) {
    // Create connection pool
    auto pool = std::make_shared<kaiten::Connection_pool>(*ioc_, *ctx_, 3);
    
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
#include <gtest/gtest.h>
#include "../src/offline_manager.hpp"
#include <filesystem>

class Offline_manager_test : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary directory for testing
        _test_dir = "./test_offline_data_" + std::to_string(std::time(nullptr));
        _manager = std::make_unique<kaiten::offline::Offline_manager>(_test_dir);
    }

    void TearDown() override {
        // Clean up the temporary directory
        if (!_test_dir.empty() && std::filesystem::exists(_test_dir)) {
            std::filesystem::remove_all(_test_dir);
        }
    }

    std::string _test_dir;
    std::unique_ptr<kaiten::offline::Offline_manager> _manager;
};

TEST_F(Offline_manager_test, Initialization) {
    EXPECT_TRUE(_manager->initialize());
    EXPECT_TRUE(_manager->is_initialized());
}

TEST_F(Offline_manager_test, GenerateUniqueId) {
    _manager->initialize();
    std::string id1 = _manager->generate_unique_id();
    std::string id2 = _manager->generate_unique_id();
    
    EXPECT_FALSE(id1.empty());
    EXPECT_FALSE(id2.empty());
    EXPECT_NE(id1, id2);
}

TEST_F(Offline_manager_test, QueueOperation) {
    _manager->initialize();
    
    kaiten::offline::Offline_operation op;
    op.id = _manager->generate_unique_id();
    op.type = kaiten::offline::OperationType::CREATE;
    op.resource_type = "card";
    op.resource_id = "";
    op.data = "{}";
    op.completed = false;
    op.timestamp = std::chrono::system_clock::now();
    
    EXPECT_TRUE(_manager->queue_operation(op));
}

TEST_F(Offline_manager_test, GetPendingOperations) {
    _manager->initialize();
    
    // Initially no pending operations
    auto pending = _manager->get_pending_operations();
    EXPECT_TRUE(pending.empty());
    
    // Add an operation
    kaiten::offline::Offline_operation op;
    op.id = _manager->generate_unique_id();
    op.type = kaiten::offline::OperationType::CREATE;
    op.resource_type = "card";
    op.resource_id = "";
    op.data = "{}";
    op.completed = false;
    op.timestamp = std::chrono::system_clock::now();
    
    EXPECT_TRUE(_manager->queue_operation(op));
    
    // Now we should have one pending operation
    pending = _manager->get_pending_operations();
    EXPECT_EQ(pending.size(), 1);
    EXPECT_EQ(pending[0].id, op.id);
}

TEST_F(Offline_manager_test, MarkOperationCompleted) {
    _manager->initialize();
    
    // Add an operation
    kaiten::offline::Offline_operation op;
    op.id = _manager->generate_unique_id();
    op.type = kaiten::offline::OperationType::CREATE;
    op.resource_type = "card";
    op.resource_id = "";
    op.data = "{}";
    op.completed = false;
    op.timestamp = std::chrono::system_clock::now();
    
    EXPECT_TRUE(_manager->queue_operation(op));
    
    // Mark it as completed
    EXPECT_TRUE(_manager->mark_operation_completed(op.id));
    
    // Now there should be no pending operations
    auto pending = _manager->get_pending_operations();
    EXPECT_TRUE(pending.empty());
}

TEST_F(Offline_manager_test, GetStats) {
    _manager->initialize();
    
    // Initial stats
    auto stats = _manager->get_stats();
    EXPECT_EQ(stats.total_operations, 0);
    EXPECT_EQ(stats.pending_operations, 0);
    EXPECT_EQ(stats.completed_operations, 0);
    
    // Add an operation
    kaiten::offline::Offline_operation op;
    op.id = _manager->generate_unique_id();
    op.type = kaiten::offline::OperationType::CREATE;
    op.resource_type = "card";
    op.resource_id = "";
    op.data = "{}";
    op.completed = false;
    op.timestamp = std::chrono::system_clock::now();
    
    EXPECT_TRUE(_manager->queue_operation(op));
    
    // Check stats after adding operation
    stats = _manager->get_stats();
    EXPECT_EQ(stats.total_operations, 1);
    EXPECT_EQ(stats.pending_operations, 1);
    EXPECT_EQ(stats.completed_operations, 0);
    
    // Mark as completed and check stats again
    EXPECT_TRUE(_manager->mark_operation_completed(op.id));
    stats = _manager->get_stats();
    EXPECT_EQ(stats.total_operations, 1);
    EXPECT_EQ(stats.pending_operations, 0);
    EXPECT_EQ(stats.completed_operations, 1);
}
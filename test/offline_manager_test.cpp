#include <gtest/gtest.h>
#include "../src/offline_manager.hpp"
#include <filesystem>

class OfflineManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary directory for testing
        test_dir_ = "./test_offline_data_" + std::to_string(std::time(nullptr));
        manager_ = std::make_unique<kaiten::offline::OfflineManager>(test_dir_);
    }

    void TearDown() override {
        // Clean up the temporary directory
        if (!test_dir_.empty() && std::filesystem::exists(test_dir_)) {
            std::filesystem::remove_all(test_dir_);
        }
    }

    std::string test_dir_;
    std::unique_ptr<kaiten::offline::OfflineManager> manager_;
};

TEST_F(OfflineManagerTest, Initialization) {
    EXPECT_TRUE(manager_->initialize());
    EXPECT_TRUE(manager_->is_initialized());
}

TEST_F(OfflineManagerTest, GenerateUniqueId) {
    manager_->initialize();
    std::string id1 = manager_->generate_unique_id();
    std::string id2 = manager_->generate_unique_id();
    
    EXPECT_FALSE(id1.empty());
    EXPECT_FALSE(id2.empty());
    EXPECT_NE(id1, id2);
}

TEST_F(OfflineManagerTest, QueueOperation) {
    manager_->initialize();
    
    kaiten::offline::OfflineOperation op;
    op.id = manager_->generate_unique_id();
    op.type = kaiten::offline::OperationType::CREATE;
    op.resource_type = "card";
    op.resource_id = "";
    op.data = "{}";
    op.completed = false;
    op.timestamp = std::chrono::system_clock::now();
    
    EXPECT_TRUE(manager_->queue_operation(op));
}

TEST_F(OfflineManagerTest, GetPendingOperations) {
    manager_->initialize();
    
    // Initially no pending operations
    auto pending = manager_->get_pending_operations();
    EXPECT_TRUE(pending.empty());
    
    // Add an operation
    kaiten::offline::OfflineOperation op;
    op.id = manager_->generate_unique_id();
    op.type = kaiten::offline::OperationType::CREATE;
    op.resource_type = "card";
    op.resource_id = "";
    op.data = "{}";
    op.completed = false;
    op.timestamp = std::chrono::system_clock::now();
    
    EXPECT_TRUE(manager_->queue_operation(op));
    
    // Now we should have one pending operation
    pending = manager_->get_pending_operations();
    EXPECT_EQ(pending.size(), 1);
    EXPECT_EQ(pending[0].id, op.id);
}

TEST_F(OfflineManagerTest, MarkOperationCompleted) {
    manager_->initialize();
    
    // Add an operation
    kaiten::offline::OfflineOperation op;
    op.id = manager_->generate_unique_id();
    op.type = kaiten::offline::OperationType::CREATE;
    op.resource_type = "card";
    op.resource_id = "";
    op.data = "{}";
    op.completed = false;
    op.timestamp = std::chrono::system_clock::now();
    
    EXPECT_TRUE(manager_->queue_operation(op));
    
    // Mark it as completed
    EXPECT_TRUE(manager_->mark_operation_completed(op.id));
    
    // Now there should be no pending operations
    auto pending = manager_->get_pending_operations();
    EXPECT_TRUE(pending.empty());
}

TEST_F(OfflineManagerTest, GetStats) {
    manager_->initialize();
    
    // Initial stats
    auto stats = manager_->get_stats();
    EXPECT_EQ(stats.total_operations, 0);
    EXPECT_EQ(stats.pending_operations, 0);
    EXPECT_EQ(stats.completed_operations, 0);
    
    // Add an operation
    kaiten::offline::OfflineOperation op;
    op.id = manager_->generate_unique_id();
    op.type = kaiten::offline::OperationType::CREATE;
    op.resource_type = "card";
    op.resource_id = "";
    op.data = "{}";
    op.completed = false;
    op.timestamp = std::chrono::system_clock::now();
    
    EXPECT_TRUE(manager_->queue_operation(op));
    
    // Check stats after adding operation
    stats = manager_->get_stats();
    EXPECT_EQ(stats.total_operations, 1);
    EXPECT_EQ(stats.pending_operations, 1);
    EXPECT_EQ(stats.completed_operations, 0);
    
    // Mark as completed and check stats again
    EXPECT_TRUE(manager_->mark_operation_completed(op.id));
    stats = manager_->get_stats();
    EXPECT_EQ(stats.total_operations, 1);
    EXPECT_EQ(stats.pending_operations, 0);
    EXPECT_EQ(stats.completed_operations, 1);
}
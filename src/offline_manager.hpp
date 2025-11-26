#ifndef KAITEN_OFFLINE_MANAGER_HPP
#define KAITEN_OFFLINE_MANAGER_HPP

#include <string>
#include <vector>
#include <map>
// #include <optional>
#include <filesystem>
#include <mutex>
#include <chrono>
#include "card.hpp"
// #include "cache.hpp"

namespace kaiten {
namespace offline {

    // Offline operation types
    enum class OperationType {
        CREATE,
        UPDATE,
        DELETE,
        GET
    };

    // Offline operation structure
    struct Offline_operation {
        std::string id;                    // Unique operation ID
        OperationType type;               // Type of operation
        std::string resource_type;         // Type of resource (card, user, etc.)
        std::string resource_id;         // ID of the resource
        std::string data;                   // Serialized data for the operation
        std::chrono::system_clock::time_point timestamp; // When the operation was created
        std::map<std::string, std::string> metadata;      // Additional metadata
        bool completed;                    // Whether the operation has been completed
    };

    // Offline manager class
    class Offline_manager {
    public:
        // Constructor
        explicit Offline_manager(const std::string& storage_path = ".kaiten_offline");
        
        // Destructor
        ~Offline_manager();
        
        // Initialize the offline manager
        bool initialize();
        
        // Check if offline manager is initialized
        bool is_initialized() const;
        
        // Enable/disable offline mode
        void set_offline_mode(bool enabled);
        
        // Check if offline mode is enabled
        bool is_offline_mode() const;
        
        // Add an operation to the offline queue
        bool queue_operation(const Offline_operation& operation);
        
        // Get all pending operations
        std::vector<Offline_operation> get_pending_operations() const;
        
        // Mark an operation as completed
        bool mark_operation_completed(const std::string& operation_id);
        
        // Remove a completed operation
        bool remove_operation(const std::string& operation_id);
        
        // Clear all completed operations
        bool clear_completed_operations();
        
        // Process pending operations (when coming back online)
        size_t process_pending_operations();
        
        // Get offline manager statistics
        struct Stats {
            size_t pending_operations;
            size_t completed_operations;
            size_t total_operations;
            std::chrono::system_clock::time_point last_sync;
        };
        Stats get_stats() const;
        
        // Save state to persistent storage
        bool save_state();
        
        // Load state from persistent storage
        bool load_state();
        
        // Get storage path
        std::string get_storage_path() const;
        
        // Generate unique ID for operations
        static std::string generate_unique_id();
        
    private:
        std::string _storage_path;
        std::vector<Offline_operation> _operations;
        bool _initialized;
        bool _offline_mode;
        mutable std::mutex _manager_mutex;
        std::chrono::system_clock::time_point _last_sync;
        
        // Get operations file path
        std::filesystem::path get_operations_file_path() const;
        
        // Save operations to file
        bool save_operations() const;
        
        // Load operations from file
        bool load_operations();
        
        // Process a single operation
        bool process_operation(const Offline_operation& operation);
        
        // Process create operation
        bool process_create_operation(const Offline_operation& operation);
        
        // Process update operation
        bool process_update_operation(const Offline_operation& operation);
        
        // Process delete operation
        bool process_delete_operation(const Offline_operation& operation);
        
        // Process get operation
        bool process_get_operation(const Offline_operation& operation);
    };

    // Global offline manager instance
    Offline_manager& get_global_manager();

    // Helper functions for common operations
    
    // Queue a card creation operation
    bool queue_card_creation(const Simple_card& card_data, const std::string& parent_id = "");
    
    // Queue a card update operation
    bool queue_card_update(const std::string& card_id, const Simple_card& card_data);
    
    // Queue a card deletion operation
    bool queue_card_deletion(const std::string& card_id);
    
    // Queue a card retrieval operation
    bool queue_card_retrieval(const std::string& card_id);
    
    // Process all pending operations
    size_t process_all_pending_operations();

} // namespace offline
} // namespace kaiten

#endif // KAITEN_OFFLINE_MANAGER_HPP
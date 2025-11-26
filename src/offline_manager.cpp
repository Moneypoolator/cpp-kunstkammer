#include "offline_manager.hpp"
#include <fstream>
#include <sstream>
// #include <iomanip>
#include <nlohmann/json.hpp>
#include <iostream>
#include <algorithm>
#include <random>
#include <chrono>

namespace kaiten {
namespace offline {

    // Constructor
    Offline_manager::Offline_manager(const std::string& storage_path) :
        _storage_path(storage_path),
        _initialized(false),
        _offline_mode(false),
        _last_sync(std::chrono::system_clock::time_point::min())
    {
    }

    // Destructor
    Offline_manager::~Offline_manager()
    {
        if (_initialized) {
            save_state();
        }
    }

    // Initialize the offline manager
    bool Offline_manager::initialize()
    {
        std::lock_guard<std::mutex> lock(_manager_mutex);
        
        try {
            // Create storage directory if it doesn't exist
            std::filesystem::create_directories(_storage_path);
            
            // Load existing operations
            load_operations();
            
            _initialized = true;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to initialize offline manager: " << e.what() << std::endl;
            return false;
        }
    }

    // Check if offline manager is initialized
    bool Offline_manager::is_initialized() const
    {
        return _initialized;
    }

    // Enable/disable offline mode
    void Offline_manager::set_offline_mode(bool enabled)
    {
        std::lock_guard<std::mutex> lock(_manager_mutex);
        _offline_mode = enabled;
    }

    // Check if offline mode is enabled
    bool Offline_manager::is_offline_mode() const
    {
        return _offline_mode;
    }

    // Add an operation to the offline queue
    bool Offline_manager::queue_operation(const Offline_operation& operation)
    {
        std::lock_guard<std::mutex> lock(_manager_mutex);
        
        if (!_initialized) {
            return false;
        }
        
        _operations.push_back(operation);
        return save_operations();
    }

    // Get all pending operations
    std::vector<Offline_operation> Offline_manager::get_pending_operations() const
    {
        std::lock_guard<std::mutex> lock(_manager_mutex);
        
        std::vector<Offline_operation> pending;
        for (const auto& op : _operations) {
            if (!op.completed) {
                pending.push_back(op);
            }
        }
        
        return pending;
    }

    // Mark an operation as completed
    bool Offline_manager::mark_operation_completed(const std::string& operation_id)
    {
        std::lock_guard<std::mutex> lock(_manager_mutex);
        
        for (auto& op : _operations) {
            if (op.id == operation_id && !op.completed) {
                op.completed = true;
                return save_operations();
            }
        }
        
        return false;
    }

    // Remove a completed operation
    bool Offline_manager::remove_operation(const std::string& operation_id)
    {
        std::lock_guard<std::mutex> lock(_manager_mutex);
        
        auto it = std::remove_if(_operations.begin(), _operations.end(),
            [&operation_id](const Offline_operation& op) {
                return op.id == operation_id;
            });
        
        if (it != _operations.end()) {
            _operations.erase(it, _operations.end());
            return save_operations();
        }
        
        return false;
    }

    // Clear all completed operations
    bool Offline_manager::clear_completed_operations()
    {
        std::lock_guard<std::mutex> lock(_manager_mutex);
        
        auto it = std::remove_if(_operations.begin(), _operations.end(),
            [](const Offline_operation& op) {
                return op.completed;
            });
        
        if (it != _operations.end()) {
            _operations.erase(it, _operations.end());
            return save_operations();
        }
        
        return true;
    }

    // Process pending operations (when coming back online)
    size_t Offline_manager::process_pending_operations()
    {
        std::lock_guard<std::mutex> lock(_manager_mutex);
        
        if (!_initialized || _offline_mode) {
            return 0;
        }
        
        size_t processed = 0;
        std::vector<Offline_operation> pending = get_pending_operations();
        
        for (auto& op : pending) {
            if (process_operation(op)) {
                mark_operation_completed(op.id);
                processed++;
            }
        }
        
        if (processed > 0) {
            _last_sync = std::chrono::system_clock::now();
            save_state();
        }
        
        return processed;
    }

    // Get offline manager statistics
    Offline_manager::Stats Offline_manager::get_stats() const
    {
        std::lock_guard<std::mutex> lock(_manager_mutex);
        
        Stats stats{};
        stats.total_operations = _operations.size();
        stats.last_sync = _last_sync;
        
        for (const auto& op : _operations) {
            if (op.completed) {
                stats.completed_operations++;
            } else {
                stats.pending_operations++;
            }
        }
        
        return stats;
    }

    // Save state to persistent storage
    bool Offline_manager::save_state()
    {
        return save_operations();
    }

    // Load state from persistent storage
    bool Offline_manager::load_state()
    {
        return load_operations();
    }

    // Get storage path
    std::string Offline_manager::get_storage_path() const
    {
        return _storage_path;
    }

    // Generate unique ID for operations
    std::string Offline_manager::generate_unique_id()
    {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1000, 9999);
        
        std::stringstream ss;
        ss << "op_" << nanoseconds << "_" << dis(gen);
        return ss.str();
    }

    // Get operations file path
    std::filesystem::path Offline_manager::get_operations_file_path() const
    {
        return std::filesystem::path(_storage_path) / "operations.json";
    }

    // Save operations to file
    bool Offline_manager::save_operations() const
    {
        try {
            nlohmann::json j;
            j["operations"] = nlohmann::json::array();
            
            for (const auto& op : _operations) {
                nlohmann::json op_json;
                op_json["id"] = op.id;
                op_json["type"] = static_cast<int>(op.type);
                op_json["resource_type"] = op.resource_type;
                op_json["resource_id"] = op.resource_id;
                op_json["data"] = op.data;
                op_json["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                    op.timestamp.time_since_epoch()).count();
                op_json["completed"] = op.completed;
                
                nlohmann::json metadata_json = nlohmann::json::object();
                for (const auto& [key, value] : op.metadata) {
                    metadata_json[key] = value;
                }
                op_json["metadata"] = metadata_json;
                
                j["operations"].push_back(op_json);
            }
            
            j["last_sync"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                _last_sync.time_since_epoch()).count();
            
            std::ofstream file(get_operations_file_path());
            if (file.is_open()) {
                file << j.dump(2);
                return true;
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to save operations: " << e.what() << std::endl;
        }
        
        return false;
    }

    // Load operations from file
    bool Offline_manager::load_operations()
    {
        try {
            std::ifstream file(get_operations_file_path());
            if (!file.is_open()) {
                // File doesn't exist, which is fine for first run
                return true;
            }
            
            nlohmann::json j;
            file >> j;
            
            _operations.clear();
            
            if (j.contains("operations") && j["operations"].is_array()) {
                for (const auto& op_json : j["operations"]) {
                    Offline_operation op;
                    op.id = op_json.value("id", "");
                    op.type = static_cast<OperationType>(op_json.value("type", 0));
                    op.resource_type = op_json.value("resource_type", "");
                    op.resource_id = op_json.value("resource_id", "");
                    op.data = op_json.value("data", "");
                    op.completed = op_json.value("completed", false);
                    
                    auto timestamp_ms = op_json.value("timestamp", int64_t(0));
                    op.timestamp = std::chrono::system_clock::time_point(
                        std::chrono::milliseconds(timestamp_ms));
                    
                    if (op_json.contains("metadata") && op_json["metadata"].is_object()) {
                        for (const auto& [key, value] : op_json["metadata"].items()) {
                            op.metadata[key] = value.get<std::string>();
                        }
                    }
                    
                    _operations.push_back(op);
                }
            }
            
            if (j.contains("last_sync")) {
                auto last_sync_ms = j["last_sync"].get<int64_t>();
                _last_sync = std::chrono::system_clock::time_point(
                    std::chrono::milliseconds(last_sync_ms));
            }
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to load operations: " << e.what() << std::endl;
            return false;
        }
    }

    // Process a single operation
    bool Offline_manager::process_operation(const Offline_operation& operation)
    {
        // In a real implementation, this would actually perform the API calls
        // For now, we'll just simulate successful processing
        switch (operation.type) {
            case OperationType::CREATE:
                return process_create_operation(operation);
            case OperationType::UPDATE:
                return process_update_operation(operation);
            case OperationType::DELETE:
                return process_delete_operation(operation);
            case OperationType::GET:
                return process_get_operation(operation);
        }
        return false;
    }

    // Process create operation
    bool Offline_manager::process_create_operation(const Offline_operation& operation)
    {
        // This is a placeholder implementation
        // In a real implementation, this would make actual API calls
        std::cout << "Processing CREATE operation for " << operation.resource_type 
                  << " with ID " << operation.resource_id << std::endl;
        return true;
    }

    // Process update operation
    bool Offline_manager::process_update_operation(const Offline_operation& operation)
    {
        // This is a placeholder implementation
        // In a real implementation, this would make actual API calls
        std::cout << "Processing UPDATE operation for " << operation.resource_type 
                  << " with ID " << operation.resource_id << std::endl;
        return true;
    }

    // Process delete operation
    bool Offline_manager::process_delete_operation(const Offline_operation& operation)
    {
        // This is a placeholder implementation
        // In a real implementation, this would make actual API calls
        std::cout << "Processing DELETE operation for " << operation.resource_type 
                  << " with ID " << operation.resource_id << std::endl;
        return true;
    }

    // Process get operation
    bool Offline_manager::process_get_operation(const Offline_operation& operation)
    {
        // This is a placeholder implementation
        // In a real implementation, this would make actual API calls
        std::cout << "Processing GET operation for " << operation.resource_type 
                  << " with ID " << operation.resource_id << std::endl;
        return true;
    }

    // Global offline manager instance
    Offline_manager& get_global_manager()
    {
        static Offline_manager instance;
        return instance;
    }

    // Helper functions for common operations
    
    // Queue a card creation operation
    bool queue_card_creation(const Simple_card& card_data, const std::string& parent_id)
    {
        Offline_operation op;
        op.id = get_global_manager().generate_unique_id();
        op.type = OperationType::CREATE;
        op.resource_type = "card";
        op.resource_id = ""; // Will be assigned by the server
        op.data = ""; // In a real implementation, this would be serialized card data
        op.timestamp = std::chrono::system_clock::now();
        op.completed = false;
        
        if (!parent_id.empty()) {
            op.metadata["parent_id"] = parent_id;
        }
        
        return get_global_manager().queue_operation(op);
    }
    
    // Queue a card update operation
    bool queue_card_update(const std::string& card_id, const Simple_card& card_data)
    {
        Offline_operation op;
        op.id = get_global_manager().generate_unique_id();
        op.type = OperationType::UPDATE;
        op.resource_type = "card";
        op.resource_id = card_id;
        op.data = ""; // In a real implementation, this would be serialized card data
        op.timestamp = std::chrono::system_clock::now();
        op.completed = false;
        
        return get_global_manager().queue_operation(op);
    }
    
    // Queue a card deletion operation
    bool queue_card_deletion(const std::string& card_id)
    {
        Offline_operation op;
        op.id = get_global_manager().generate_unique_id();
        op.type = OperationType::DELETE;
        op.resource_type = "card";
        op.resource_id = card_id;
        op.data = "";
        op.timestamp = std::chrono::system_clock::now();
        op.completed = false;
        
        return get_global_manager().queue_operation(op);
    }
    
    // Queue a card retrieval operation
    bool queue_card_retrieval(const std::string& card_id)
    {
        Offline_operation op;
        op.id = get_global_manager().generate_unique_id();
        op.type = OperationType::GET;
        op.resource_type = "card";
        op.resource_id = card_id;
        op.data = "";
        op.timestamp = std::chrono::system_clock::now();
        op.completed = false;
        
        return get_global_manager().queue_operation(op);
    }
    
    // Process all pending operations
    size_t process_all_pending_operations()
    {
        return get_global_manager().process_pending_operations();
    }

} // namespace offline
} // namespace kaiten
#ifndef CACHE_HPP
#define CACHE_HPP

#include "card.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <optional>
#include <iostream>

namespace kaiten {

template<typename Key, typename Value>
class Cache {
public:
    struct CacheEntry {
        Value value;
        std::chrono::steady_clock::time_point timestamp;
        std::chrono::seconds ttl;
        
        bool is_expired() const {
            return std::chrono::steady_clock::now() - timestamp > ttl;
        }
    };

    // Конструктор с TTL по умолчанию
    explicit Cache(std::chrono::seconds default_ttl = std::chrono::minutes(5))
        : default_ttl_(default_ttl), _enabled(true) {}

    // Получить значение из кэша
    std::optional<Value> get(const Key& key) {
        if (!_enabled) return std::nullopt;

        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            if (!it->second.is_expired()) {
                hits_++;
                return it->second.value;
            } else {
                // Удаляем просроченную запись
                cache_.erase(it);
                misses_++;
            }
        } else {
            misses_++;
        }
        
        return std::nullopt;
    }

    // Сохранить значение в кэш
    void put(const Key& key, const Value& value, 
             std::chrono::seconds ttl = std::chrono::seconds(-1)) {
        if (!_enabled) return;

        if (ttl.count() < 0) {
            ttl = default_ttl_;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        
        CacheEntry entry{value, std::chrono::steady_clock::now(), ttl};
        cache_[key] = entry;
        
        // Очистка старых записей при достижении лимита
        if (cache_.size() > max_size_) {
            cleanup_expired();
        }
    }

    // Удалить значение из кэша
    void invalidate(const Key& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.erase(key);
    }

    // Очистить весь кэш
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.clear();
        hits_ = 0;
        misses_ = 0;
    }

    // Очистка просроченных записей
    void cleanup_expired() {
        auto now = std::chrono::steady_clock::now();
        for (auto it = cache_.begin(); it != cache_.end(); ) {
            if (it->second.is_expired()) {
                it = cache_.erase(it);
            } else {
                ++it;
            }
        }
    }

    // Получить статистику кэша
    void print_stats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << "Cache Stats:" << std::endl;
        std::cout << "  Size: " << cache_.size() << "/" << max_size_ << std::endl;
        std::cout << "  Hits: " << hits_ << std::endl;
        std::cout << "  Misses: " << misses_ << std::endl;
        if (hits_ + misses_ > 0) {
            double hit_ratio = static_cast<double>(hits_) / (hits_ + misses_) * 100.0;
            std::cout << "  Hit Ratio: " << hit_ratio << "%" << std::endl;
        }
        std::cout << "  Enabled: " << (_enabled ? "Yes" : "No") << std::endl;
    }

    // Включить/выключить кэш
    void set_enabled(bool enabled) {
        std::lock_guard<std::mutex> lock(mutex_);
        _enabled = enabled;
    }

    // Установить максимальный размер кэша
    void set_max_size(size_t max_size) {
        std::lock_guard<std::mutex> lock(mutex_);
        max_size_ = max_size;
        if (cache_.size() > max_size_) {
            cleanup_expired();
        }
    }

    // Установить TTL по умолчанию
    void set_default_ttl(std::chrono::seconds ttl) {
        default_ttl_ = ttl;
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<Key, CacheEntry> cache_;
    std::chrono::seconds default_ttl_;
    size_t max_size_ = 10000;
    bool _enabled = true; // Changed from std::atomic<bool> to bool, as it's always accessed under a mutex.
    
    // Статистика
    size_t hits_ = 0;
    size_t misses_ = 0;
};

// Специализированные кэши для различных типов данных
class ApiCache {
public:
    // Кэш для карточек (TTL 2 минуты)
    static Cache<std::int64_t, Card>& card_cache() {
        static Cache<std::int64_t, Card> instance(std::chrono::minutes(2));
        return instance;
    }
    
    // Кэш для карточек по номеру (TTL 2 минуты)
    static Cache<std::string, Card>& card_number_cache() {
        static Cache<std::string, Card> instance(std::chrono::minutes(2));
        return instance;
    }
    
    // Кэш для пользователей (TTL 10 минут)
    static Cache<std::int64_t, User>& user_cache() {
        static Cache<std::int64_t, User> instance(std::chrono::minutes(10));
        return instance;
    }
    
    // Кэш для досок (TTL 30 минут)
    static Cache<std::int64_t, Board>& board_cache() {
        static Cache<std::int64_t, Board> instance(std::chrono::minutes(30));
        return instance;
    }
    
    // Кэш для списков (TTL 1 минута)
    static Cache<std::string, nlohmann::json>& list_cache() {
        static Cache<std::string, nlohmann::json> instance(std::chrono::minutes(1));
        return instance;
    }

    // Очистить все кэши
    static void clear_all() {
        card_cache().clear();
        card_number_cache().clear();
        user_cache().clear();
        board_cache().clear();
        list_cache().clear();
    }

    // Показать статистику всех кэшей
    static void print_all_stats() {
        std::cout << "=== API Cache Statistics ===" << std::endl;
        card_cache().print_stats();
        card_number_cache().print_stats();
        user_cache().print_stats();
        board_cache().print_stats();
        list_cache().print_stats();
    }
};

} // namespace kaiten

#endif // CACHE_HPP
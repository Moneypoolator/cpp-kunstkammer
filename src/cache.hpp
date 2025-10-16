#ifndef CACHE_HPP
#define CACHE_HPP

#include "card.hpp"
#include <chrono>
#include <iostream>
//#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace kaiten {

template <typename Key, typename Value>
class Cache {
public:
    struct Cache_entry {
        Value value;
        std::chrono::steady_clock::time_point timestamp;
        std::chrono::seconds ttl {};

        bool is_expired() const
        {
            return std::chrono::steady_clock::now() - timestamp > ttl;
        }
    };

    // Конструктор с TTL по умолчанию
    explicit Cache(std::chrono::seconds default_ttl = std::chrono::minutes(5)):
        _default_ttl(default_ttl), _enabled(true) { }

    // Получить значение из кэша
    std::optional<Value> get(const Key& key)
    {
        if (!_enabled) {
            return std::nullopt;
        }

        std::lock_guard<std::mutex> lock(_mutex);

        auto it = _cache.find(key);
        if (it != _cache.end()) {
            if (!it->second.is_expired()) {
                _hits++;
                return it->second.value;
            } // Удаляем просроченную запись
            _cache.erase(it);
            _misses++;

        } else {
            _misses++;
        }

        return std::nullopt;
    }

    // Сохранить значение в кэш
    void put(const Key& key, const Value& value,
        std::chrono::seconds ttl = std::chrono::seconds(-1))
    {
        if (!_enabled) {
            return;
        }

        if (ttl.count() < 0) {
            ttl = _default_ttl;
        }

        std::lock_guard<std::mutex> lock(_mutex);

        Cache_entry entry { value, std::chrono::steady_clock::now(), ttl };
        _cache[key] = entry;

        // Очистка старых записей при достижении лимита
        if (_cache.size() > _max_size) {
            cleanup_expired();
        }
    }

    // Удалить значение из кэша
    void invalidate(const Key& key)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _cache.erase(key);
    }

    // Очистить весь кэш
    void clear()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _cache.clear();
        _hits = 0;
        _misses = 0;
    }

    // Очистка просроченных записей
    void cleanup_expired()
    {
        // auto now = std::chrono::steady_clock::now();
        for (auto it = _cache.begin(); it != _cache.end();) {
            if (it->second.is_expired()) {
                it = _cache.erase(it);
            } else {
                ++it;
            }
        }
    }

    // Получить статистику кэша
    void print_stats() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        std::cout << "Cache Stats:" << std::endl;
        std::cout << "  Size: " << _cache.size() << "/" << _max_size << std::endl;
        std::cout << "  Hits: " << _hits << std::endl;
        std::cout << "  Misses: " << _misses << std::endl;
        if (_hits + _misses > 0) {
            double hit_ratio = static_cast<double>(_hits) / (_hits + _misses) * 100.0;
            std::cout << "  Hit Ratio: " << hit_ratio << "%" << std::endl;
        }
        std::cout << "  Enabled: " << (_enabled ? "Yes" : "No") << std::endl;
    }

    // Включить/выключить кэш
    void set_enabled(bool enabled)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _enabled = enabled;
    }

    // Установить максимальный размер кэша
    void set_max_size(size_t max_size)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _max_size = max_size;
        if (_cache.size() > _max_size) {
            cleanup_expired();
        }
    }

    // Установить TTL по умолчанию
    void set_default_ttl(std::chrono::seconds ttl)
    {
        _default_ttl = ttl;
    }

private:
    mutable std::mutex _mutex;
    std::unordered_map<Key, Cache_entry> _cache;
    std::chrono::seconds _default_ttl;
    size_t _max_size = 10000;
    bool _enabled = true; // Changed from std::atomic<bool> to bool, as it's always accessed under a mutex.

    // Статистика
    size_t _hits = 0;
    size_t _misses = 0;
};

// Специализированные кэши для различных типов данных
class Api_cache {
public:
    // Кэш для карточек (TTL 2 минуты)
    static Cache<std::int64_t, Card>& card_cache()
    {
        static Cache<std::int64_t, Card> instance(std::chrono::minutes(2));
        return instance;
    }

    // Кэш для карточек по номеру (TTL 2 минуты)
    static Cache<std::string, Card>& card_number_cache()
    {
        static Cache<std::string, Card> instance(std::chrono::minutes(2));
        return instance;
    }

    // Кэш для пользователей (TTL 10 минут)
    static Cache<std::int64_t, User>& user_cache()
    {
        static Cache<std::int64_t, User> instance(std::chrono::minutes(10));
        return instance;
    }

    // Кэш для досок (TTL 30 минут)
    static Cache<std::int64_t, Board>& board_cache()
    {
        static Cache<std::int64_t, Board> instance(std::chrono::minutes(30));
        return instance;
    }

    // Кэш для списков (TTL 1 минута)
    static Cache<std::string, nlohmann::json>& list_cache()
    {
        static Cache<std::string, nlohmann::json> instance(std::chrono::minutes(1));
        return instance;
    }

    // Очистить все кэши
    static void clear_all()
    {
        card_cache().clear();
        card_number_cache().clear();
        user_cache().clear();
        board_cache().clear();
        list_cache().clear();
    }

    // Показать статистику всех кэшей
    static void print_all_stats()
    {
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
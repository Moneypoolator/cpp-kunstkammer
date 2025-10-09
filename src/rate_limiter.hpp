#ifndef RATE_LIMITER_HPP
#define RATE_LIMITER_HPP

#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <iostream>

namespace kaiten {

class Rate_limiter {
public:
    // Конструктор с настройками лимитов
    explicit Rate_limiter(
        int max_requests_per_minute = 60,
        int max_requests_per_hour = 1000,
        const std::chrono::milliseconds& min_request_interval = std::chrono::milliseconds(100)
    ) : _max_per_minute(max_requests_per_minute),
        _max_per_hour(max_requests_per_hour),
        _min_interval(min_request_interval),
        _enabled(true)
    {
        reset_counters();
    }

    // Ожидание перед выполнением запроса
    void wait_if_needed() {
        if (!_enabled) { 
            return;
        }

        std::lock_guard<std::mutex> lock(_mutex);
        
        auto now = std::chrono::steady_clock::now();
        
        // Проверяем часовой лимит
        if (_hourly_requests >= _max_per_hour) {
            auto time_since_hour_reset = now - _last_hour_reset;
            if (time_since_hour_reset < std::chrono::hours(1)) {
                auto wait_time = std::chrono::hours(1) - time_since_hour_reset;
                std::cout << "Hourly limit reached. Waiting " 
                          << std::chrono::duration_cast<std::chrono::minutes>(wait_time).count() 
                          << " minutes..." << std::endl;
                std::this_thread::sleep_for(wait_time);
                reset_counters();
            }
        }
        
        // Проверяем минутный лимит
        if (_minute_requests >= _max_per_minute) {
            auto time_since_minute_reset = now - _last_minute_reset;
            if (time_since_minute_reset < std::chrono::minutes(1)) {
                auto wait_time = std::chrono::minutes(1) - time_since_minute_reset;
                std::cout << "Minute limit reached. Waiting " 
                          << std::chrono::duration_cast<std::chrono::seconds>(wait_time).count() 
                          << " seconds..." << std::endl;
                std::this_thread::sleep_for(wait_time);
                reset_minute_counter();
            }
        }
        
        // Соблюдаем минимальный интервал между запросами
        if (_last_request_time.time_since_epoch().count() > 0) {
            auto time_since_last_request = now - _last_request_time;
            if (time_since_last_request < _min_interval) {
                auto wait_time = _min_interval - time_since_last_request;
                std::this_thread::sleep_for(wait_time);
            }
        }
        
        // Обновляем счетчики
        _last_request_time = std::chrono::steady_clock::now();
        _minute_requests++;
        _hourly_requests++;
        
        // Проверяем необходимость сброса счетчиков
        auto current_time = std::chrono::steady_clock::now();
        if (current_time - _last_minute_reset >= std::chrono::minutes(1)) {
            reset_minute_counter();
        }
        if (current_time - _last_hour_reset >= std::chrono::hours(1)) {
            reset_hourly_counter();
        }
    }

    // Получить статистику
    void print_stats() const {
        std::lock_guard<std::mutex> lock(_mutex);
        std::cout << "Rate Limiter Stats:" << std::endl;
        std::cout << "  Minute: " << _minute_requests << "/" << _max_per_minute << std::endl;
        std::cout << "  Hour: " << _hourly_requests << "/" << _max_per_hour << std::endl;
        std::cout << "  Enabled: " << (_enabled ? "Yes" : "No") << std::endl;
    }

    // Включить/выключить лимитер
    void set_enabled(bool enabled) {
        std::lock_guard<std::mutex> lock(_mutex);
        _enabled = enabled;
    }

    // Установить новые лимиты
    void set_limits(int per_minute, int per_hour) {
        std::lock_guard<std::mutex> lock(_mutex);
        _max_per_minute = per_minute;
        _max_per_hour = per_hour;
    }

private:
    void reset_counters() {
        _last_minute_reset = std::chrono::steady_clock::now();
        _last_hour_reset = std::chrono::steady_clock::now();
        _minute_requests = 0;
        _hourly_requests = 0;
    }

    void reset_minute_counter() {
        _last_minute_reset = std::chrono::steady_clock::now();
        _minute_requests = 0;
    }

    void reset_hourly_counter() {
        _last_hour_reset = std::chrono::steady_clock::now();
        _hourly_requests = 0;
    }

    mutable std::mutex _mutex;
    std::chrono::steady_clock::time_point _last_request_time;
    std::chrono::steady_clock::time_point _last_minute_reset;
    std::chrono::steady_clock::time_point _last_hour_reset;
    
    int _max_per_minute;
    int _max_per_hour;
    std::chrono::milliseconds _min_interval;
    
    int _minute_requests = 0;
    int _hourly_requests = 0;
    std::atomic<bool> _enabled;
};

// Глобальный лимитер (можно сделать thread-local для многопоточности)
extern Rate_limiter global_rate_limiter;

} // namespace kaiten

#endif // RATE_LIMITER_HPP
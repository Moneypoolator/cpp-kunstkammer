#ifndef RATE_LIMITER_HPP
#define RATE_LIMITER_HPP

#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <map>
#include <string>
#include <iostream>

namespace kaiten {

class RateLimiter {
public:
    // Конструктор с настройками лимитов
    explicit RateLimiter(
        int max_requests_per_minute = 60,
        int max_requests_per_hour = 1000,
        const std::chrono::milliseconds& min_request_interval = std::chrono::milliseconds(100)
    ) : max_per_minute_(max_requests_per_minute),
        max_per_hour_(max_requests_per_hour),
        min_interval_(min_request_interval),
        enabled_(true)
    {
        reset_counters();
    }

    // Ожидание перед выполнением запроса
    void wait_if_needed() {
        if (!enabled_) return;

        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = std::chrono::steady_clock::now();
        
        // Проверяем часовой лимит
        if (hourly_requests_ >= max_per_hour_) {
            auto time_since_hour_reset = now - last_hour_reset_;
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
        if (minute_requests_ >= max_per_minute_) {
            auto time_since_minute_reset = now - last_minute_reset_;
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
        if (last_request_time_.time_since_epoch().count() > 0) {
            auto time_since_last_request = now - last_request_time_;
            if (time_since_last_request < min_interval_) {
                auto wait_time = min_interval_ - time_since_last_request;
                std::this_thread::sleep_for(wait_time);
            }
        }
        
        // Обновляем счетчики
        last_request_time_ = std::chrono::steady_clock::now();
        minute_requests_++;
        hourly_requests_++;
        
        // Проверяем необходимость сброса счетчиков
        auto current_time = std::chrono::steady_clock::now();
        if (current_time - last_minute_reset_ >= std::chrono::minutes(1)) {
            reset_minute_counter();
        }
        if (current_time - last_hour_reset_ >= std::chrono::hours(1)) {
            reset_hourly_counter();
        }
    }

    // Получить статистику
    void print_stats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << "Rate Limiter Stats:" << std::endl;
        std::cout << "  Minute: " << minute_requests_ << "/" << max_per_minute_ << std::endl;
        std::cout << "  Hour: " << hourly_requests_ << "/" << max_per_hour_ << std::endl;
        std::cout << "  Enabled: " << (enabled_ ? "Yes" : "No") << std::endl;
    }

    // Включить/выключить лимитер
    void set_enabled(bool enabled) {
        std::lock_guard<std::mutex> lock(mutex_);
        enabled_ = enabled;
    }

    // Установить новые лимиты
    void set_limits(int per_minute, int per_hour) {
        std::lock_guard<std::mutex> lock(mutex_);
        max_per_minute_ = per_minute;
        max_per_hour_ = per_hour;
    }

private:
    void reset_counters() {
        last_minute_reset_ = std::chrono::steady_clock::now();
        last_hour_reset_ = std::chrono::steady_clock::now();
        minute_requests_ = 0;
        hourly_requests_ = 0;
    }

    void reset_minute_counter() {
        last_minute_reset_ = std::chrono::steady_clock::now();
        minute_requests_ = 0;
    }

    void reset_hourly_counter() {
        last_hour_reset_ = std::chrono::steady_clock::now();
        hourly_requests_ = 0;
    }

    mutable std::mutex mutex_;
    std::chrono::steady_clock::time_point last_request_time_;
    std::chrono::steady_clock::time_point last_minute_reset_;
    std::chrono::steady_clock::time_point last_hour_reset_;
    
    int max_per_minute_;
    int max_per_hour_;
    std::chrono::milliseconds min_interval_;
    
    int minute_requests_ = 0;
    int hourly_requests_ = 0;
    std::atomic<bool> enabled_;
};

// Глобальный лимитер (можно сделать thread-local для многопоточности)
extern RateLimiter global_rate_limiter;

} // namespace kaiten

#endif // RATE_LIMITER_HPP
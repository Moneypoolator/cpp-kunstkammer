#ifndef DATE_HPP
#define DATE_HPP

#include <chrono>
#include <ctime>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

// A small helper class to represent card-related dates (created/updated/etc.)
// Stores time in UTC and supports RFC3339/ISO-8601 strings like:
//   2024-09-01T12:34:56Z
//   2024-09-01T12:34:56+03:00
class Card_date {
public:
    Card_date():
        timePoint_(std::chrono::system_clock::time_point {}) { }

    explicit Card_date(std::chrono::system_clock::time_point tp):
        timePoint_(tp) { }

    static Card_date nowUtc()
    {
        return Card_date(std::chrono::system_clock::now());
    }

    static Card_date parse(const std::string& iso8601)
    {
        // Parse formats like YYYY-MM-DDTHH:MM:SSZ or with offset +HH:MM / -HH:MM
        // Strategy: parse date/time, detect 'Z' or "+/-HH:MM", compute offset, convert to UTC.
        if (iso8601.empty()) {
            return Card_date();
        }

        // Find timezone part
        size_t tz_pos = iso8601.find_first_of("Z+-", 19); // after seconds position
        if (tz_pos == std::string::npos) {
            throw std::invalid_argument("Invalid ISO-8601 date: missing timezone");
        }

        std::string datetime = iso8601.substr(0, tz_pos);
        int offset_seconds = 0;

        if (iso8601[tz_pos] == 'Z') {
            // UTC
        } else {
            // Expect +HH:MM or -HH:MM
            if (tz_pos + 6 > iso8601.size()) {
                throw std::invalid_argument("Invalid ISO-8601 timezone offset");
            }
            char sign = iso8601[tz_pos];
            std::string hh = iso8601.substr(tz_pos + 1, 2);
            if (iso8601[tz_pos + 3] != ':') {
                throw std::invalid_argument("Invalid ISO-8601 timezone separator");
            }
            std::string mm = iso8601.substr(tz_pos + 4, 2);
            int hours = std::stoi(hh);
            int minutes = std::stoi(mm);
            offset_seconds = hours * 3600 + minutes * 60;
            if (sign == '-') {
                offset_seconds = -offset_seconds;
            }
        }

        // Parse datetime part (no timezone), expected YYYY-MM-DDTHH:MM:SS
        std::tm tm = {};
        if (datetime.size() < 19 || datetime[10] != 'T') {
            throw std::invalid_argument("Invalid ISO-8601 datetime part");
        }
        tm.tm_year = std::stoi(datetime.substr(0, 4)) - 1900;
        tm.tm_mon = std::stoi(datetime.substr(5, 2)) - 1;
        tm.tm_mday = std::stoi(datetime.substr(8, 2));
        tm.tm_hour = std::stoi(datetime.substr(11, 2));
        tm.tm_min = std::stoi(datetime.substr(14, 2));
        tm.tm_sec = std::stoi(datetime.substr(17, 2));

        // Convert tm as if it is in UTC, not local. We create time_t by mktime (local) and adjust by timezone.
        // To avoid platform-specific timegm, we manually compute using timegm-like behavior.
        // Approximation: use timegm if available via _GNU_SOURCE; otherwise adjust using time zone difference.
#if defined(_GNU_SOURCE) || defined(__USE_MISC)
        time_t t = timegm(&tm);
#else
        // Fallback: temporarily use mktime assuming tm is local time, then adjust by local timezone offset to get UTC.
        // This can be slightly off for historical DST edges, but acceptable for most API timestamps.
        std::tm tm_copy = tm;
        time_t local_t = std::mktime(&tm_copy);
        // Compute local offset to UTC at that time
        std::tm* g = std::gmtime(&local_t);
        std::tm* l = std::localtime(&local_t);
        time_t g_t = std::mktime(g);
        time_t l_t = std::mktime(l);
        time_t tz_diff = l_t - g_t; // local - utc
        time_t t = local_t - tz_diff;
#endif

        // Apply timezone offset to get UTC
        t -= offset_seconds;
        return Card_date(std::chrono::system_clock::from_time_t(t));
    }

    std::string toIso8601() const
    {
        auto t = std::chrono::system_clock::to_time_t(timePoint_);
        std::tm tm = *std::gmtime(&t);
        char buf[32];
        if (std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm) == 0) {
            return std::string();
        }
        return std::string(buf);
    }

    std::chrono::system_clock::time_point timePoint() const { return timePoint_; }

    bool isValid() const { return timePoint_.time_since_epoch().count() != 0; }

private:
    std::chrono::system_clock::time_point timePoint_;
};

inline void to_json(nlohmann::json& j, const Card_date& d)
{
    j = d.toIso8601();
}

inline void from_json(const nlohmann::json& j, Card_date& d)
{
    if (j.is_null()) {
        d = Card_date();
        return;
    }
    if (!j.is_string()) {
        throw std::invalid_argument("CardDate expects string in JSON");
    }
    d = Card_date::parse(j.get<std::string>());
}

#endif // DATE_HPP

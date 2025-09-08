#ifndef CARD_HPP
#define CARD_HPP

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "date.hpp"

// Represents a Kaiten card
struct Card {
    std::int64_t id = 0;
    std::string number; // card number (string to preserve prefixes)
    std::string title;
    std::string type;
    int size = 0;

    std::int64_t board_id = 0;
    std::int64_t column_id = 0;
    std::int64_t lane_id = 0;

    CardDate created_at;
    CardDate updated_at;

    std::vector<std::string> tags;
};

// JSON serialization
inline void to_json(nlohmann::json& j, const Card& c)
{
    j = nlohmann::json {
        { "id", c.id },
        { "number", c.number },
        { "title", c.title },
        { "type", c.type },
        { "size", c.size },
        { "board_id", c.board_id },
        { "column_id", c.column_id },
        { "lane_id", c.lane_id },
        { "created_at", c.created_at },
        { "updated_at", c.updated_at },
        { "tags", c.tags }
    };
}

inline std::int64_t parse_int64_from_json(const nlohmann::json& v)
{
    if (v.is_number_integer()) {
        return v.get<std::int64_t>();
    }
    if (v.is_string()) {
        return std::stoll(v.get<std::string>());
    }
    return 0;
}

// JSON deserialization (tolerant to numbers or strings for IDs)
inline void from_json(const nlohmann::json& j, Card& c)
{
    if (j.contains("id")) {
        c.id = parse_int64_from_json(j.at("id"));
    }

    if (j.contains("number")) {
        if (j.at("number").is_string()) {
            c.number = j.at("number").get<std::string>();
        } else if (j.at("number").is_number()) {
            c.number = std::to_string(j.at("number").get<std::int64_t>());
        }
    }

    if (j.contains("title")) {
        j.at("title").get_to(c.title);
    }
    if (j.contains("type")) {
        const auto& t = j.at("type");
        if (t.is_string()) {
            c.type = t.get<std::string>();
        } else if (t.is_object()) {
            if (t.contains("name") && t["name"].is_string()) {
                c.type = t["name"].get<std::string>();
            } else if (t.contains("id")) {
                if (t["id"].is_string()) {
                    c.type = t["id"].get<std::string>();
                } else if (t["id"].is_number_integer()) {
                    c.type = std::to_string(t["id"].get<std::int64_t>());
                }
            }
        }
    }
    if (j.contains("size")) {
        c.size = j.at("size").is_number() ? j.at("size").get<int>() : 0;
    }

    if (j.contains("board_id")) {
        c.board_id = parse_int64_from_json(j.at("board_id"));
    }
    if (j.contains("column_id")) {
        c.column_id = parse_int64_from_json(j.at("column_id"));
    }
    if (j.contains("lane_id")) {
        c.lane_id = parse_int64_from_json(j.at("lane_id"));
    }

    if (j.contains("created_at")) {
        c.created_at = j.at("created_at").is_null() ? CardDate() : CardDate::parse(j.at("created_at").get<std::string>());
    } else if (j.contains("created")) {
        c.created_at = j.at("created").is_null() ? CardDate() : CardDate::parse(j.at("created").get<std::string>());
    }
    if (j.contains("updated_at")) {
        c.updated_at = j.at("updated_at").is_null() ? CardDate() : CardDate::parse(j.at("updated_at").get<std::string>());
    } else if (j.contains("updated")) {
        c.updated_at = j.at("updated").is_null() ? CardDate() : CardDate::parse(j.at("updated").get<std::string>());
    }

    if (j.contains("tags") && j.at("tags").is_array()) {
        c.tags.clear();
        for (const auto& tag : j.at("tags")) {
            if (tag.is_string()) {
                c.tags.push_back(tag.get<std::string>());
            } else if (tag.is_object()) {
                if (tag.contains("name") && tag["name"].is_string()) {
                    c.tags.push_back(tag["name"].get<std::string>());
                } else if (tag.contains("id")) {
                    if (tag["id"].is_string()) {
                        c.tags.push_back(tag["id"].get<std::string>());
                    } else if (tag["id"].is_number_integer()) {
                        c.tags.push_back(std::to_string(tag["id"].get<std::int64_t>()));
                    }
                }
            }
        }
    }
}

#endif // CARD_HPP

#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

struct Config {
    std::string token;
    std::string baseUrl;
    std::string logLevel;
    std::string boardId;
    std::string columnId;
    std::string laneId;
    std::vector<std::string> tags;
};

inline void from_json(const nlohmann::json& j, Config& c) {
    j.at("Token").get_to(c.token);
    j.at("BaseURL").get_to(c.baseUrl);
    j.at("LogLevel").get_to(c.logLevel);
    j.at("BoardID").get_to(c.boardId);
    j.at("ColumnID").get_to(c.columnId);
    j.at("LaneID").get_to(c.laneId);
    j.at("Tags").get_to(c.tags);
}

#endif // CONFIG_HPP 
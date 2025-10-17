#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

struct Config {
    std::string token;
    std::string baseUrl;
    std::string logLevel;
    std::int64_t boardId;
    std::int64_t columnId;
    std::int64_t laneId;
    std::int64_t spaceId;
    std::vector<std::string> tags;
    std::int64_t taskTypeId;
    std::int64_t taskSize;
    std::string role;
};

inline void from_json(const nlohmann::json& j, Config& c) {
    j.at("Token").get_to(c.token);
    j.at("BaseURL").get_to(c.baseUrl);
    j.at("LogLevel").get_to(c.logLevel);
    j.at("BoardID").get_to(c.boardId);
    j.at("ColumnID").get_to(c.columnId);
    j.at("LaneID").get_to(c.laneId);
    j.at("SpaceID").get_to(c.spaceId);
    j.at("Tags").get_to(c.tags);

    j.at("TaskTypeId").get_to(c.taskTypeId);
    j.at("TaskSize").get_to(c.taskSize);
    j.at("Role").get_to(c.role);
}

#endif // CONFIG_HPP 
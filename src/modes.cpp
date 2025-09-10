#include "modes.hpp"

#include <fstream>
#include <iostream>
// <sstream> intentionally kept for potential future parsing needs in modes

#include "card.hpp"
#include "kaiten.hpp"

namespace {

template <typename Client>
bool paginate_cards(
    Client& client,
    const std::string& host,
    const std::string& api_path,
    const std::string& token,
    int page_size,
    const std::function<void(const nlohmann::json&)>& handle_items,
    int& total_items_out)
{
    std::string cards_path = api_path + "/cards";
    total_items_out = 0;

    bool used_page_style = true;
    int page_number = 1;
    std::string last_first_id;
    while (true) {
        std::string target = cards_path + "?page=" + std::to_string(page_number) + "&per_page=" + std::to_string(page_size);
        auto [status, body] = client.get(host, "443", target, token);
        if (status != 200) {
            used_page_style = false;
            break;
        }
        try {
            auto json = nlohmann::json::parse(body);
            nlohmann::json items_json = json.is_array() ? json : (json.contains("cards") && json["cards"].is_array() ? json["cards"] : nlohmann::json::array());
            if (items_json.empty()) {
                break;
            }

            std::string first_id_curr;
            try {
                first_id_curr = items_json[0].value("id", std::string());
            } catch (...) {
                first_id_curr = "";
            }
            if (page_number > 1 && !first_id_curr.empty() && first_id_curr == last_first_id) {
                used_page_style = false;
                break;
            }
            last_first_id = first_id_curr;

            handle_items(items_json);
            total_items_out += static_cast<int>(items_json.size());
            std::cout << "Fetched page " << page_number << ", items: " << items_json.size() << std::endl;
            if (static_cast<int>(items_json.size()) < page_size) {
                break;
            }
        } catch (const std::exception&) {
            used_page_style = false;
            break;
        }
        page_number++;
    }

    if (!used_page_style) {
        int offset = 0;
        while (true) {
            std::string target = cards_path + "?offset=" + std::to_string(offset) + "&limit=" + std::to_string(page_size);
            auto [status, body] = client.get(host, "443", target, token);
            if (status != 200) {
                break;
            }
            try {
                auto json = nlohmann::json::parse(body);
                nlohmann::json items_json = json.is_array() ? json : (json.contains("cards") && json["cards"].is_array() ? json["cards"] : nlohmann::json::array());
                if (items_json.empty()) {
                    break;
                }
                handle_items(items_json);
                total_items_out += static_cast<int>(items_json.size());
                std::cout << "Fetched offset " << offset << ", items: " << items_json.size() << std::endl;
                if (static_cast<int>(items_json.size()) < page_size) {
                    break;
                }
                offset += static_cast<int>(items_json.size());
            } catch (const std::exception&) {
                break;
            }
        }
    }

    return total_items_out > 0;
}
} // namespace

int handle_get_card(Http_client& client, const std::string& host,
    const std::string& api_path, const std::string& token,
    const std::string& card_number)
{

    auto [status, card] = kaiten::get_card(client, host, api_path, token, card_number);

    if (status == 200) {
        return 0;
    } 
    std::cerr << "Failed to get card. Status: " << status << std::endl;
    return 1;
}

int handle_cards_list(Http_client& client, const std::string& host, const std::string& api_path, const std::string& token)
{
    const int page_size = 100;
    int total_cards = 0;
    auto print_items = [](const nlohmann::json& items_json) {
        for (const auto& it : items_json) {
            try {
                Card c = it.get<Card>();
                std::cout << "#" << c.number << " [" << c.id << "] " << c.title << " (" << c.type << ", size=" << c.size << ")" << std::endl;
            } catch (...) {
                // skip malformed item
            }
        }
    };

    paginate_cards(client, host, api_path, token, page_size, print_items, total_cards);
    std::cout << "Total cards fetched: " << total_cards << std::endl;
    return 0;
}

int handle_tasks(Http_client& client, const std::string& host, const std::string& api_path, const std::string& token, const Config& config, const std::string& tasks_file_path)
{
    std::ifstream tasks_file(tasks_file_path);
    if (!tasks_file) {
        std::cerr << "Could not open tasks file: " << tasks_file_path << std::endl;
        return 1;
    }
    nlohmann::json tasks_json;
    try {
        tasks_file >> tasks_json;
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse tasks JSON: " << e.what() << std::endl;
        return 1;
    }
    if (!tasks_json.contains("schedule") || !tasks_json["schedule"].contains("tasks")) {
        std::cerr << "Invalid tasks JSON structure." << std::endl;
        return 1;
    }
    auto& tasks = tasks_json["schedule"]["tasks"];
    for (const auto& task : tasks) {
        Simple_card desired;
        desired.title = task.value("title", "");
        desired.column_id = std::stoll(config.columnId);
        desired.lane_id = std::stoll(config.laneId);
        desired.type = task.value("type", "");
        desired.size = task.value("size", 0);
        // if (task.contains("tags") && task["tags"].is_array()) {
        //     desired.tags = task["tags"].get<std::vector<std::string>>();
        // }

        std::cout << "Creating card: title='" << desired.title << "' column=" << desired.column_id << " lane=" << desired.lane_id << std::endl;
        auto [status, created] = kaiten::create_card(client, host, api_path, token, desired);
        std::cout << "Create HTTP status: " << status << std::endl;
        if (status == 200 || status == 201) {
            std::cout << "Created card #" << created.number << " [" << created.id << "] '" << created.title << "'" << std::endl;
        }
    }
    return 0;
}

int handle_create_card(Http_client& client, const std::string& host, const std::string& api_path, const std::string& token, const Config& config, const std::string& title, const std::string& type, int size, const std::vector<std::string>& tags)
{
    Simple_card desired;
    desired.title = title;
    desired.column_id = std::stoll(config.columnId);
    desired.lane_id = std::stoll(config.laneId);
    desired.type = type;
    desired.size = size;
    desired.tags = tags;

    std::cout << "Creating single card: '" << desired.title << "'" << std::endl;
    auto [status, created] = kaiten::create_card(client, host, api_path, token, desired);
    std::cout << "Create HTTP status: " << status << std::endl;
    if (status == 200 || status == 201) {
        std::cout << "Created card #" << created.number << " [" << created.id << "] '" << created.title << "'" << std::endl;
        return 0;
    }
    return 1;
}

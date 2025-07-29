#include <iostream>
#include <fstream>
#include <boost/program_options.hpp>
#include "nlohmann/json.hpp"
#include "config.hpp"
#include "http_client.hpp"

#include <boost/asio/ssl.hpp>

namespace po = boost::program_options;
namespace ssl = boost::asio::ssl;

int main(int argc, char** argv) {
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("tasks", po::value<std::string>(), "Path to the tasks JSON file for task creation mode")
        ("report", po::value<std::string>(), "Path to the report JSON file for report import mode")
        ("config", po::value<std::string>()->default_value("config.json"), "Path to the configuration file (optional, default: config.json)");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    if (!vm.count("tasks") && !vm.count("report")) {
        std::cerr << "Wrong command line arguments" << std::endl;
        std::cout << desc << "\n";
        return 1;
    }

    std::string configFile = vm["config"].as<std::string>();
    
    Config config;
    if (!configFile.empty()) {
        std::ifstream f(configFile);
        if (f) {
            try {
                nlohmann::json data = nlohmann::json::parse(f);
                config = data.get<Config>();
                
                std::cout << "Config loaded from " << configFile << std::endl;
                std::cout << "Token: " << config.token << std::endl;
                std::cout << "BaseURL: " << config.baseUrl << std::endl;

            } catch (nlohmann::json::parse_error& e) {
                std::cerr << "Failed to parse config file: " << e.what() << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Could not open config file: " << configFile << std::endl;
            return 1;
        }
    } else {
        std::cerr << "Empty config file name" << std::endl;
        return 1;
    }

    if (vm.count("tasks")) {
        std::cout << "Tasks file: " << vm["tasks"].as<std::string>() << std::endl;
    }

    if (vm.count("report")) {
        std::cout << "Report file: " << vm["report"].as<std::string>() << std::endl;
    }

    ssl::context ctx(ssl::context::tlsv12_client);
    // ctx.set_default_verify_paths();
    // ctx.set_verify_mode(ssl::verify_peer);
    ctx.set_verify_mode(ssl::verify_none); // Disable SSL verification for testing

    nlohmann::json card_data = {
        {"title", "Test from C++"},
        {"column_id", std::stoi(config.columnId)},
        {"lane_id", std::stoi(config.laneId)}
    };

    // Parse host and path from config.baseUrl
    std::string base_url = config.baseUrl;
    std::string host, api_path;
    if (base_url.rfind("https://", 0) == 0) base_url = base_url.substr(8);
    size_t slash_pos = base_url.find('/');
    if (slash_pos != std::string::npos) {
        host = base_url.substr(0, slash_pos);
        api_path = base_url.substr(slash_pos);
    } else {
        host = base_url;
        api_path = "";
    }
    std::string cards_path = api_path + "/cards";

    std::cout << "Host: " << host << std::endl;
    std::cout << "Port: 443" << std::endl;
    std::cout << "Path: " << cards_path << std::endl;

    net::io_context ioc;
    HttpClient client(ioc, ctx);

    if (vm.count("tasks")) {
        std::string tasksFile = vm["tasks"].as<std::string>();
        // Load tasks from tasks.json
        std::ifstream tasks_file(tasksFile);
        if (!tasks_file) {
            std::cerr << "Could not open tasks file: " << tasksFile << std::endl;
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
            nlohmann::json card_data = {
                {"title", task.value("title", "")},
                {"column_id", std::stoi(config.columnId)},
                {"lane_id", std::stoi(config.laneId)},
                {"type", task.value("type", "")},
                {"size", task.value("size", 0)}
            };
            std::cout << "Creating card: " << card_data.dump() << std::endl;
            auto [status, response] = client.post(
                host,
                "443",
                cards_path,
                card_data.dump(),
                config.token
            );
            std::cout << "POST HTTP status: " << status << std::endl;
            std::cout << "POST response: " << response << std::endl;
            if (status == 200 || status == 201) {
                try {
                    auto resp_json = nlohmann::json::parse(response);
                    if (resp_json.contains("id")) {
                        std::cout << "Created card ID: " << resp_json["id"] << std::endl;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Failed to parse response JSON: " << e.what() << std::endl;
                }
            } else {
                std::cerr << "Error from Kaiten API: " << response << std::endl;
            }
        }
    }

    bool get_card_mode = false;
    if (get_card_mode) {
        // Example: GET all cards after creating one
        auto [get_status, get_response] = client.get(
            host,
            "443",
            cards_path, // This will be /api/latest/cards
            config.token
        );
        std::cout << "GET HTTP status: " << get_status << std::endl;
        std::cout << "GET response: " << get_response << std::endl;
        if (get_status == 200) {
            try {
                auto get_json = nlohmann::json::parse(get_response);
                std::cout << "GET response JSON parsed successfully." << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Failed to parse GET response JSON: " << e.what() << std::endl;
            }
        } else {
            std::cerr << "GET error or non-200 status." << std::endl;
        }
    }


    return 0;
} 
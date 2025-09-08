#include <iostream>
#include <fstream>
#include <boost/program_options.hpp>
#include "nlohmann/json.hpp"
#include "config.hpp"
#include "http_client.hpp"
#include "modes.hpp"

#include <boost/asio/ssl.hpp>

namespace po = boost::program_options;
namespace ssl = boost::asio::ssl;

bool option_exists(const po::variables_map& vm, const std::string& option) {
    return vm.count(option) > 0;
}

int main(int argc, char** argv) {
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("tasks", po::value<std::string>(), "Path to the tasks JSON file for task creation mode")
        ("report", po::value<std::string>(), "Path to the report JSON file for report import mode")
        ("get-card", po::value<std::string>(), "Card number to retrieve (get-card mode)")
        ("cards-list", "List cards (cards-list mode)")
        ("create-card", po::value<std::string>(), "Create a card with given title (uses ColumnID/LaneID from config)")
        ("type", po::value<std::string>()->default_value(""), "Card type for create-card")
        ("size", po::value<int>()->default_value(0), "Card size for create-card")
        ("tags", po::value<std::string>()->default_value(""), "Comma-separated tags for create-card")
        ("config", po::value<std::string>()->default_value("config.json"), "Path to the configuration file (optional, default: config.json)");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (option_exists(vm, "help")) {
        std::cout << desc << "\n";
        return 1;
    }

    if (!option_exists(vm, "tasks") 
        && !option_exists(vm, "report") 
        && !option_exists(vm, "get-card") 
        && !option_exists(vm, "cards-list") 
        && !option_exists(vm, "create-card")) 
    {
        std::cerr << "Wrong command line arguments" << std::endl;
        std::cout << desc << "\n";
        return 1;
    }

    std::string config_file = vm["config"].as<std::string>();
    
    Config config;
    if (!config_file.empty()) {
        std::ifstream f(config_file);
        if (f) {
            try {
                nlohmann::json data = nlohmann::json::parse(f);
                config = data.get<Config>();
                
                std::cout << "Config loaded from " << config_file << std::endl;
                std::cout << "Token: " << config.token << std::endl;
                std::cout << "BaseURL: " << config.baseUrl << std::endl;

            } catch (nlohmann::json::parse_error& e) {
                std::cerr << "Failed to parse config file: " << e.what() << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Could not open config file: " << config_file << std::endl;
            return 1;
        }
    } else {
        std::cerr << "Empty config file name" << std::endl;
        return 1;
    }

    if (option_exists(vm, "tasks")) {
        std::cout << "Tasks file: " << vm["tasks"].as<std::string>() << std::endl;
    }

    if (option_exists(vm, "report")) {
        std::cout << "Report file: " << vm["report"].as<std::string>() << std::endl;
    }

    ssl::context ctx(ssl::context::tlsv12_client);
    // ctx.set_default_verify_paths();
    // ctx.set_verify_mode(ssl::verify_peer);
    ctx.set_verify_mode(ssl::verify_none); // Disable SSL verification for testing

    //

    // Parse host and path from config.baseUrl
    std::string base_url = config.baseUrl;
    std::string host;
    std::string api_path;

    if (base_url.rfind("https://", 0) == 0) {
        base_url = base_url.substr(8); 
    }

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
    Http_client client(ioc, ctx);

    if (option_exists(vm, "tasks")) {
        std::string tasks_file_name = vm["tasks"].as<std::string>();
        handle_tasks(client, host, api_path, config.token, config, tasks_file_name);
    }

    if (option_exists(vm, "create-card")) {
        std::string title = vm["create-card"].as<std::string>();
        std::string type = vm["type"].as<std::string>();
        int size = vm["size"].as<int>();
        std::string tags_csv = vm["tags"].as<std::string>();
        std::vector<std::string> tags;
        if (!tags_csv.empty()) {
            std::stringstream ss(tags_csv);
            std::string tag;
            while (std::getline(ss, tag, ',')) {
                if (!tag.empty()) {
                    tags.push_back(tag);
                }
            }
        }
        handle_create_card(client, host, api_path, config.token, config, title, type, size, tags);
    }

    if (option_exists(vm, "get-card")) {
        std::string card_number = vm["get-card"].as<std::string>();
        handle_get_card(client, host, api_path, config.token, card_number);
    }

    if (option_exists(vm, "cards-list")) {
        handle_cards_list(client, host, api_path, config.token);
    }


    return 0;
} 
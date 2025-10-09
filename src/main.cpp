#include <iostream>
#include <fstream>
#include <boost/program_options.hpp>
#include "nlohmann/json.hpp"

#include "config.hpp"
#include "http_client.hpp"
#include "modes.hpp"
#include "cache.hpp"
#include "rate_limiter.hpp"

#include <boost/asio/ssl.hpp>

namespace po = boost::program_options;
namespace ssl = boost::asio::ssl;

bool option_exists(const po::variables_map& vm, const std::string& option) {
    return vm.count(option) > 0;
}

// Функция для парсинга фильтров из строки
std::map<std::string, std::string> parse_filters(const std::string& filters_str) {
    std::map<std::string, std::string> filters;
    if (filters_str.empty()) { 
        return filters;
    }
    
    std::stringstream ss(filters_str);
    std::string filter;
    
    while (std::getline(ss, filter, ',')) {
        size_t pos = filter.find('=');
        if (pos != std::string::npos) {
            std::string key = filter.substr(0, pos);
            std::string value = filter.substr(pos + 1);
            // URL encode значение?
            filters[key] = value;
        } else {
            std::cerr << "Warning: Invalid filter format: " << filter << " (expected key=value)" << std::endl;
        }
    }
    
    return filters;
}

int main(int argc, char** argv) {
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("tasks", po::value<std::string>(), "Path to the tasks JSON file for task creation mode")
        ("report", po::value<std::string>(), "Path to the report JSON file for report import mode")
        ("get-card", po::value<std::string>(), "Card number to retrieve (get-card mode)")
        ("cards-list", "List cards (cards-list mode)")
        ("cards-filter", po::value<std::string>(), "Filter cards (format: key1=value1,key2=value2)")
        ("users-list", "List all users")
        ("get-user", po::value<std::string>(), "Get specific user by ID")
        ("boards-list", "List all boards")
        ("create-card", po::value<std::string>(), "Create a card with given title (uses ColumnID/LaneID from config)")
        ("type", po::value<std::string>()->default_value(""), "Card type for create-card")
        ("size", po::value<int>()->default_value(0), "Card size for create-card")
        ("tags", po::value<std::string>()->default_value(""), "Comma-separated tags for create-card")
        ("config", po::value<std::string>()->default_value("config.json"), "Path to the configuration file")
        ("no-cache", "Disable caching")
        ("no-rate-limit", "Disable rate limiting")
        ("cache-stats", "Show cache statistics")
        ("rate-limit-stats", "Show rate limit statistics")
        ("clear-cache", "Clear all caches")
        ("rate-limit-per-minute", po::value<int>()->default_value(60), "Requests per minute limit")
        ("rate-limit-per-hour", po::value<int>()->default_value(1000), "Requests per hour limit")
        ("request-interval", po::value<int>()->default_value(100), "Minimum interval between requests (ms)")
        ("page-size", po::value<int>()->default_value(100), "Page size for pagination")
        ("sort-by", po::value<std::string>(), "Sort field (e.g., 'updated', 'created')")
        ("sort-order", po::value<std::string>()->default_value("desc"), "Sort order (asc/desc)");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (option_exists(vm, "help")) {
        std::cout << desc << "\n";
        return 1;
    }

    // Проверка наличия хотя бы одной опции
    bool has_mode = option_exists(vm, "tasks") || option_exists(vm, "report") || 
                   option_exists(vm, "get-card") || option_exists(vm, "cards-list") ||
                   option_exists(vm, "cards-filter") || option_exists(vm, "users-list") ||
                   option_exists(vm, "get-user") || option_exists(vm, "boards-list") ||
                   option_exists(vm, "create-card");
    
    if (!has_mode) {
        std::cerr << "Error: No operation mode specified" << std::endl;
        std::cout << desc << "\n";
        return 1;
    }

    // Загрузка конфигурации
    std::string config_file = vm["config"].as<std::string>();
    Config config;
    
    if (!config_file.empty()) {
        std::ifstream f(config_file);
        if (f) {
            try {
                nlohmann::json data = nlohmann::json::parse(f);
                config = data.get<Config>();
                
                std::cout << "Config loaded from " << config_file << std::endl;
                std::cout << "BaseURL: " << config.baseUrl << std::endl;
                std::cout << "BoardID: " << config.boardId << std::endl;
                std::cout << "ColumnID: " << config.columnId << std::endl;
                std::cout << "LaneID: " << config.laneId << std::endl;

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

    // Настройка rate limiting
    if (option_exists(vm, "no-rate-limit")) {
        kaiten::global_rate_limiter.set_enabled(false);
        std::cout << "Rate limiting disabled" << std::endl;
    } else {
        int per_minute = vm["rate-limit-per-minute"].as<int>();
        int per_hour = vm["rate-limit-per-hour"].as<int>();
        int interval_ms = vm["request-interval"].as<int>();
        
        kaiten::global_rate_limiter.set_limits(per_minute, per_hour);
        std::cout << "Rate limiting: " << per_minute << "/min, " 
                  << per_hour << "/hour, interval: " << interval_ms << "ms" << std::endl;
    }

    // Настройка кэширования
    if (option_exists(vm, "no-cache")) {
        kaiten::Api_cache::card_cache().set_enabled(false);
        kaiten::Api_cache::user_cache().set_enabled(false);
        kaiten::Api_cache::list_cache().set_enabled(false);
        kaiten::Api_cache::board_cache().set_enabled(false);
        std::cout << "Caching disabled" << std::endl;
    }

    // Показать статистику кэша
    if (option_exists(vm, "cache-stats")) {
        kaiten::Api_cache::print_all_stats();
    }

    // Показать статистику rate limiting
    if (option_exists(vm, "rate-limit-stats")) {
        kaiten::global_rate_limiter.print_stats();
    }

    // Очистить кэш
    if (option_exists(vm, "clear-cache")) {
        kaiten::Api_cache::clear_all();
        std::cout << "All caches cleared" << std::endl;
    }

    // Парсинг host и path из config.baseUrl
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

    std::cout << "API Endpoint: " << host << api_path << std::endl;

    // Инициализация HTTP клиента
    ssl::context ctx(ssl::context::tlsv12_client);
    ctx.set_verify_mode(ssl::verify_none);

    net::io_context ioc;
    Http_client client(ioc, ctx);

    // Обработка режимов работы
    if (option_exists(vm, "tasks")) {
        std::string tasks_file_name = vm["tasks"].as<std::string>();
        return handle_tasks(client, host, api_path, config.token, config, tasks_file_name);
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
        return handle_create_card(client, host, api_path, config.token, config, title, type, size, tags);
    }

    if (option_exists(vm, "get-card")) {
        std::string card_number = vm["get-card"].as<std::string>();
        return handle_get_card(client, host, api_path, config.token, card_number);
    }

    if (option_exists(vm, "cards-list")) {
        return handle_cards_list(client, host, api_path, config.token);
    }

    if (option_exists(vm, "cards-filter")) {
        std::string filters_str = vm["cards-filter"].as<std::string>();
        auto filters = parse_filters(filters_str);
        return handle_cards_filter(client, host, api_path, config.token, filters);
    }

    if (option_exists(vm, "users-list")) {
        return handle_users_list(client, host, api_path, config.token);
    }

    if (option_exists(vm, "get-user")) {
        std::string user_id = vm["get-user"].as<std::string>();
        return handle_get_user(client, host, api_path, config.token, user_id);
    }

    if (option_exists(vm, "boards-list")) {
        return handle_boards_list(client, host, api_path, config.token);
    }

    // В конце показать финальную статистику
    if (!option_exists(vm, "no-cache") && !option_exists(vm, "cache-stats")) {
        kaiten::Api_cache::print_all_stats();
    }
    
    if (!option_exists(vm, "no-rate-limit") && !option_exists(vm, "rate-limit-stats")) {
        kaiten::global_rate_limiter.print_stats();
    }

    return 0;
}
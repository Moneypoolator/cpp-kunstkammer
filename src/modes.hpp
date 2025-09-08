#ifndef MODES_HPP
#define MODES_HPP

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#include "http_client.hpp"
#include "config.hpp"

// Delegated mode handlers to reduce main() complexity

// --get-card
int handle_get_card(Http_client& client, const std::string& host, const std::string& api_path, const std::string& token, const std::string& card_number);

// --cards-list (with pagination and printing)
int handle_cards_list(Http_client& client, const std::string& host, const std::string& api_path, const std::string& token);

// --tasks (create cards from tasks JSON)
int handle_tasks(Http_client& client, const std::string& host, const std::string& api_path, const std::string& token, const Config& config, const std::string& tasks_file_path);

// --create-card (single create)
int handle_create_card(Http_client& client, const std::string& host, const std::string& api_path, const std::string& token, const Config& config, const std::string& title, const std::string& type, int size, const std::vector<std::string>& tags);

#endif // MODES_HPP



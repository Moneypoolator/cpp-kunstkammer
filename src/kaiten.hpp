#ifndef KAITEN_HPP
#define KAITEN_HPP

#include <string>
#include <utility>
#include <iostream>
#include <nlohmann/json.hpp>

#include "http_client.hpp"
#include "card.hpp"
#include "card_utils.hpp"

namespace kaiten {

// Creates a new card using the Kaiten API.
// Required fields: title, column_id, lane_id.
// Optional fields used if present: type, size, tags.
// Returns: { http_status, Card (parsed if success, otherwise default) }
inline std::pair<int, Card> create_card(
    Http_client& client,
    const std::string& host,
    const std::string& api_path,
    const std::string& token,
    const Simple_card& desired
) {
    std::string target = api_path + "/cards";

    nlohmann::json body = {
        {"title", desired.title},
        {"column_id", desired.column_id},
        {"lane_id", desired.lane_id}
    };
    if (!desired.type.empty()) {
        body["type"] = desired.type;
    }
    if (desired.size != 0) {
        body["size"] = desired.size;
    }
    if (!desired.tags.empty()) {
        body["tags"] = desired.tags;
    }

    auto [status, response] = client.post(host, "443", target, body.dump(), token);
    if (status == 200 || status == 201) {
        try {
            auto j = nlohmann::json::parse(response);
            Card created = j.get<Card>();
            return {status, created};
        } catch (...) {
            std::cerr << "Create card: success status but failed to parse response." << std::endl;
            return {status, Card{}};
        }
    }

    // Handle known error statuses with informative messages
    switch (status) {
        case 400:
            std::cerr << "Create card failed: 400 Bad Request. Check required fields (title, column_id, lane_id) and payload format." << std::endl;
            break;
        case 401:
            std::cerr << "Create card failed: 401 Unauthorized. Check Bearer token." << std::endl;
            break;
        case 402:
            std::cerr << "Create card failed: 402 Payment Required (plan/limits)." << std::endl;
            break;
        case 403:
            std::cerr << "Create card failed: 403 Forbidden. Insufficient permissions for board/column/lane." << std::endl;
            break;
        case 404:
            std::cerr << "Create card failed: 404 Not Found. Verify API path or IDs (column/lane)." << std::endl;
            break;
        default:
            std::cerr << "Create card failed: Unsupported status " << status << "." << std::endl;
            break;
    }

    // Try to print error details from JSON if available
    try {
        auto err = nlohmann::json::parse(response);
        if (err.is_object()) {
            if (err.contains("message")) {
                std::cerr << "Error message: " << err["message"].dump() << std::endl;
            }
            if (err.contains("errors")) {
                std::cerr << "Errors: " << err["errors"].dump() << std::endl;
            }
        }
    } catch (...) {
        if (!response.empty()) {
            std::cerr << "Error body: " << response << std::endl;
        }
    }

    return {status, Card{}};
}

} // namespace kaiten

// Updates an existing card. Target can be by numeric id or card number (string).
// Only non-empty/meaningful fields are sent.
// Returns: { http_status, Card (parsed if success) }
inline std::pair<int, Card> update_card(
    Http_client& client,
    const std::string& host,
    const std::string& api_path,
    const std::string& token,
    const std::string& id_or_number,
    const Simple_card& changes
) {
    std::string target = api_path + "/cards/" + id_or_number;

    nlohmann::json body = nlohmann::json::object();
    if (!changes.title.empty()) {
        body["title"] = changes.title;
    }
    if (!changes.type.empty()) {
        body["type"] = changes.type;
    }
    if (changes.size != 0) {
        body["size"] = changes.size;
    }
    if (changes.column_id != 0) {
        body["column_id"] = changes.column_id;
    }
    if (changes.lane_id != 0) {
        body["lane_id"] = changes.lane_id;
    }
    if (!changes.tags.empty()) {
        body["tags"] = changes.tags;
    }

    auto [status, response] = client.patch(host, "443", target, body.dump(), token);
    if (status == 200) {
        try {
            auto j = nlohmann::json::parse(response);
            Card updated = j.get<Card>();
            return {status, updated};
        } catch (...) {
            std::cerr << "Update card: success status but failed to parse response." << std::endl;
            return {status, Card{}};
        }
    }

    switch (status) {
        case 400:
            std::cerr << "Update card failed: 400 Bad Request. Check payload fields and constraints." << std::endl;
            break;
        case 401:
            std::cerr << "Update card failed: 401 Unauthorized. Check Bearer token." << std::endl;
            break;
        case 402:
            std::cerr << "Update card failed: 402 Payment Required (plan/limits)." << std::endl;
            break;
        case 403:
            std::cerr << "Update card failed: 403 Forbidden. Insufficient permissions." << std::endl;
            break;
        case 404:
            std::cerr << "Update card failed: 404 Not Found. Verify card id/number and path." << std::endl;
            break;
        default:
            std::cerr << "Update card failed: Unsupported status " << status << "." << std::endl;
            break;
    }

    try {
        auto err = nlohmann::json::parse(response);
        if (err.is_object()) {
            if (err.contains("message")) {
                std::cerr << "Error message: " << err["message"].dump() << std::endl;
            }
            if (err.contains("errors")) {
                std::cerr << "Errors: " << err["errors"].dump() << std::endl;
            }
        }
    } catch (...) {
        if (!response.empty()) {
            std::cerr << "Error body: " << response << std::endl;
        }
    }

    return {status, Card{}};
}

// kaiten.hpp - добавить после update_card
inline std::pair<int, Card> get_card(
    Http_client& client,
    const std::string& host,
    const std::string& api_path,
    const std::string& token,
    const std::string& id_or_number
) {
    std::string target = api_path + "/cards/" + id_or_number;
    
    auto [status, response] = client.get(host, "443", target, token);
    if (status == 200) {
        try {
            auto j = nlohmann::json::parse(response);
            Card card = j.get<Card>();
            card_utils::print_card_details(card, true);
            return {status, card};
        } catch (const std::exception& e) {
            std::cerr << "Failed to parse card JSON: " << e.what() << std::endl;
            std::cerr << "Raw response: " << response << std::endl;
            return {status, Card{}};
        }
    }

    // Обработка ошибок
    switch (status) {
        case 401:
            std::cerr << "Get card failed: 401 Unauthorized. Check Bearer token." << std::endl;
            break;
        case 403:
            std::cerr << "Get card failed: 403 Forbidden. Insufficient permissions." << std::endl;
            break;
        case 404:
            std::cerr << "Get card failed: 404 Not Found. Card not found." << std::endl;
            break;
        default:
            std::cerr << "Get card failed: Status " << status << "." << std::endl;
            break;
    }

    return {status, Card{}};
}

#endif // KAITEN_HPP



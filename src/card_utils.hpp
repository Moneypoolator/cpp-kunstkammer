// card_utils.hpp
#ifndef CARD_UTILS_HPP
#define CARD_UTILS_HPP

#include "card.hpp"
#include <iostream>
#include <iomanip>

namespace card_utils {

void print_card_details(const Card& card, bool verbose = false) {
    std::cout << "=== Card Details ===\n"
              << "ID: " << card.id << "\n"
              << "Number: " << card.number << "\n"
              << "Title: " << card.title << "\n"
              << "Type: " << card.type << " (ID: " << card.type_id << ")\n"
              << "Size: " << card.size << " " << card.size_unit << "\n"
              << "State: " << card.state << "\n"
              << "Archived: " << (card.archived ? "Yes" : "No") << "\n"
              << "Blocked: " << (card.blocked ? "Yes" : "No") << "\n"
              << "ASAP: " << (card.asap ? "Yes" : "No") << "\n"
              << "\n=== Board ===\n"
              << "Board ID: " << card.board_id << "\n"
              << "Board Title: " << card.board.title << "\n"
              << "\n=== Column ===\n"
              << "Column ID: " << card.column_id << "\n"
              << "Column Title: " << card.column.title << "\n"
              << "\n=== Lane ===\n"
              << "Lane ID: " << card.lane_id << "\n"
              << "Lane Title: " << card.lane.title << "\n"
              << "\n=== Owner ===\n"
              << "Owner ID: " << card.owner_id << "\n"
              << "Owner Name: " << card.owner.full_name << "\n"
              << "Owner Email: " << card.owner.email << "\n"
              << "\n=== Dates ===\n"
              << "Created: " << card.created.toIso8601() << "\n"
              << "Updated: " << card.updated.toIso8601() << "\n"
              << "Last Moved: " << card.last_moved_at.toIso8601() << "\n";
    
    if (!card.members.empty()) {
        std::cout << "\n=== Members (" << card.members.size() << ") ===\n";
        for (const auto& member : card.members) {
            std::cout << " - " << member.full_name << " (" << member.email << ")\n";
        }
    }
    
    if (!card.tags.empty()) {
        std::cout << "\n=== Tags (" << card.tags.size() << ") ===\n";
        for (const auto& tag : card.tags) {
            std::cout << " - " << tag.name << " (Color: " << tag.color << ")\n";
        }
    }
    
    if (!card.parents.empty()) {
        std::cout << "\n=== Parents (" << card.parents.size() << ") ===\n";
        for (const auto& parent : card.parents) {
            std::cout << " - #" << parent.number << ": " << parent.title 
                      << " (State: " << parent.state << ")\n";
        }
    }
    
    if (!card.description.empty()) {
        std::cout << "\n=== Description ===\n"
                  << card.description << "\n";
    }
    
    if (verbose) {
        std::cout << "\n=== Additional Info ===\n"
                  << "Comments: " << card.comments_total << "\n"
                  << "Children: " << card.children_count << "/" << card.children_done << " done\n"
                  << "Goals: " << card.goals_done << "/" << card.goals_total << " done\n"
                  << "Time Spent: " << card.time_spent_sum << " minutes\n"
                  << "UID: " << card.uid << "\n"
                  << "Version: " << card.version << "\n";
    }
}

} // namespace card_utils

#endif // CARD_UTILS_HPP
#ifndef KAITEN_HPP
#define KAITEN_HPP

#include "card_operations.hpp"
#include "user_operations.hpp"
#include "board_operations.hpp"

namespace kaiten {

// Re-export the functions from the separate modules for backward compatibility
using card_operations::create_card;
using card_operations::update_card;
using card_operations::get_card;
using card_operations::add_child_card;
using card_operations::add_tag_to_card;
using card_operations::get_cards_paginated;
using card_operations::get_all_cards;

using user_operations::get_user;
using user_operations::get_current_user;
using user_operations::get_users_by_email;
using user_operations::get_users_paginated;
using user_operations::get_all_users;

using board_operations::get_boards_paginated;

} // namespace kaiten

#endif // KAITEN_HPP
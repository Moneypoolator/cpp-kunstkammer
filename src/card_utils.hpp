// card_utils.hpp
#ifndef CARD_UTILS_HPP
#define CARD_UTILS_HPP

//#include "card.hpp"
#include "card.hpp"
#include <nlohmann/json.hpp>
#include <iostream>

// Вспомогательные функции для безопасного парсинга
template<typename T>
T get_optional(const nlohmann::json& j, const std::string& key, const T& default_value) {
    if (j.contains(key) && !j.at(key).is_null()) {
        try {
            return j.at(key).get<T>();
        } catch (const nlohmann::json::type_error& e) {
            std::cerr << "Warning: Type error for key '" << key << "': " << e.what() << std::endl;
            return default_value;
        } catch (...) {
            return default_value;
        }
    }
    return default_value;
}

// Специализации
template<>
std::string get_optional<std::string>(const nlohmann::json& j, const std::string& key, const std::string& default_value);

template<>
bool get_optional<bool>(const nlohmann::json& j, const std::string& key, const bool& default_value);

// Функции для числовых значений
std::int64_t get_int64_optional(const nlohmann::json& j, const std::string& key, std::int64_t default_value);
int get_int_optional(const nlohmann::json& j, const std::string& key, int default_value);
double get_double_optional(const nlohmann::json& j, const std::string& key, double default_value);


//namespace card_utils {
    
struct Card;

void print_card_details(const Card& card, bool verbose = false); 


//} // namespace card_utils

#endif // CARD_UTILS_HPP
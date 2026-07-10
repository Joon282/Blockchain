#pragma once

#include <nlohmann/json.hpp>
#include <unordered_map>
#include <vector>
#include <string>

class Validator {
    using json = nlohmann::json;
public:
    static bool validate_transaction(const json& transaction, const std::unordered_map<std::string, int>& nonces, const std::vector<json>& mem_pool);
private:
    static bool valid_hex(const std::string& str, size_t len);
    static bool valid_message(const std::string& str);
    static bool verify_signature(const json& transaction);
};
#pragma once

#include <string>
#include <nlohmann/json.hpp>

class Hasher {
    using json = nlohmann::json;
public:
    static std::string hash(const json& data);
};

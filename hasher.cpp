#include "hasher.hpp"
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>

using json = nlohmann::json;

std::string Hasher::hash(const json& data) {
    json hash_data;
    hash_data["index"] = data["index"];
    hash_data["previous_hash"] = data["previous_hash"];
    hash_data["transactions"]  = data["transactions"];
    
    std::string canonical = hash_data.dump(2); 
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(canonical.c_str()), canonical.size(), hash);
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

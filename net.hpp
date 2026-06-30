#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>

struct PeerAddress {
    std::string host;
    uint16_t port;
};
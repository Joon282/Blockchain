#pragma once
#include "net.hpp"
#include <condition_variable>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <boost/asio.hpp>

class Node {
    using json = nlohmann::json;
public:
    Node(const json& block, const std::vector<PeerAddress>& peers, uint16_t port);
    void run_consensus();
    void run_server();
}
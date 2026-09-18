#pragma once

#include <nlohmann/json.hpp>
#include "node.hpp"
#include "net.hpp"
#include <vector>
#include <mutex>
#include <thread>
#include <chrono>
#include <atomic>

class Consensus {
    using json = nlohmann::json;
public:
    Consensus(Node& node);
    void run_consensus();

private: 
    Node& node;
    json create_block();
    json decide_block(const std::vector<json>& proposed_blocks);
};

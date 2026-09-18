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
    Node(const json& config, const std::vector<PeerAddress>& peers, uint16_t port);
    void run_consensus();
    void run_server();
    bool add_transaction(const json& transaction);
    void commit_block(const json& block);

    uint16_t port;
    std::vector<PeerAddress> peers;
    std::vector<json> blockchain;
    std::vector<json> mem_pool;
    std::unordered_map<std::string, int> nonces;
    std::mutex block_mtx;
    std::condition_variable cv;
    std::mutex mem_pool_mtx;
    boost::asio::io_context io_context;
    std::unordered_map<std::string, std::unique_ptr<boost::asio::ip::tcp::socket>> connections;
    std::unordered_set<std::string> crashed_peers;
    std::mutex sockets_mtx;
    std::atomic<bool> peers_connected{false};
    int pending_values{-1};
    std::unordered_map<std::string, std::unique_ptr<std::mutex>> send_mutexes;
    std::mutex cout_mtx;
};

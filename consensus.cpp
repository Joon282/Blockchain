#include "consensus.hpp"
#include "node.hpp"
#include "hasher.hpp"
#include <algorithm>
#include <iostream>
#include <chrono>
#include <thread>
/*
    Node::Node(const json& config, const std::vector<PeerAddress>& peers, uint16_t port){
        this->port = port;
        this->peers = peers;
        this->blockchain.push_back(config);
    }
*/
    using json = nlohmann::json;
    Consensus::Consensus(Node& node) : node(node) {
    }

    void Consensus::run_consensus() {
        while (!node.peers_connected) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        while (true) {
            int pvi;
            {
                std::unique_lock<std::mutex> lock(node.mem_pool_mtx);
                node.cv.wait(lock, [this] {
                    return !node.mem_pool.empty() || node.pending_values != -1;
                });
                pvi = node.pending_values;
                node.pending_values = -1;
            }
            {
                int expected_next;
                {
                    std::lock_guard<std::mutex> block_lock(node.block_mtx);
                    expected_next = node.blockchain.back()["index"].get<int>() + 1;
                }
                bool pool_non_empty;
                {
                    std::lock_guard<std::mutex> mem_lock(node.mem_pool_mtx);
                    pool_non_empty = !node.mem_pool.empty();
                }
                if (!pool_non_empty && pvi != -1 && pvi < expected_next) {
                    continue;
                }
            }

            json block = create_block();
            std::vector<json> proposed_blocks;
            proposed_blocks.push_back(block);
            for (const auto& peer : node.peers) {
                std::string key = peer_key(peer);
                boost::asio::ip::tcp::socket* sock_ptr = nullptr;
                std::mutex* send_mtx_ptr = nullptr;
                {
                    std::lock_guard<std::mutex> lock(node.sockets_mtx);
                    if (node.crashed_peers.count(key)) {
                        continue;
                    }
                    if (!node.connections.count(key) || !node.send_mutexes.count(key)) {
                        continue;
                    }
                    sock_ptr = node.connections.at(key).get();
                    send_mtx_ptr = node.send_mutexes.at(key).get();
                }
                try {
                    std::lock_guard<std::mutex> send_lock(*send_mtx_ptr);
                    auto& socket = *sock_ptr;
                    struct timeval tv;
                    tv.tv_sec = 2;
                    tv.tv_usec = 0;
                    setsockopt(socket.native_handle(), SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
                    json message = json {{"type", "values"}, {"payload", json::array({block})}};
                    send_msg(socket, message);
                    json response = receive_msg(socket);
                    if (response["type"] == "values" && !response["payload"].empty()) {
                        proposed_blocks.push_back(response["payload"][0]);
                    }
                    struct timeval remove_tv;
                    remove_tv.tv_sec = 0;
                    remove_tv.tv_usec = 0;
                    setsockopt(socket.native_handle(), SOL_SOCKET, SO_RCVTIMEO, (const char*)&remove_tv, sizeof remove_tv);
                } catch (const std::exception& e) {
                    std::lock_guard<std::mutex> lock(node.sockets_mtx);
                    node.crashed_peers.insert(key);
                }
            }
            json decided_block = decide_block(proposed_blocks);
            node.commit_block(decided_block);
            {
                std::lock_guard<std::mutex> cout_lock(node.cout_mtx);
                std::cout << decided_block.dump(2) << std::endl;
            }
        }
    }

    json Consensus::create_block() {
        json block;
        {
            std::lock_guard<std::mutex> lock(node.block_mtx);
            block["index"] = node.blockchain.back()["index"].get<int>() + 1;
            block["previous_hash"] = node.blockchain.back()["current_hash"];
        }
        {
            std::lock_guard<std::mutex> lock(node.mem_pool_mtx);
            block["transactions"] = node.mem_pool;
        }
        block["current_hash"] = Hasher::hash(block);
        return block;
    }

    json Consensus::decide_block(const std::vector<json>& proposed_blocks) {
        std::string expected_prev_hash;
        int expected_index;
        {
            std::lock_guard<std::mutex> lock(node.block_mtx);
            expected_prev_hash = node.blockchain.back()["current_hash"].get<std::string>();
            expected_index = node.blockchain.back()["index"].get<int>() + 1;
        }
        
        std::vector<json> chain_valid;
        for (const auto& block : proposed_blocks) {
            if (block["previous_hash"].get<std::string>() == expected_prev_hash &&
                block["index"].get<int>() == expected_index) {
                chain_valid.push_back(block);
            }
        }
        if (chain_valid.empty()) {
            chain_valid = proposed_blocks;
        }

        std::vector<json> non_empty;
        for (const auto& block : chain_valid) {
            if (!block["transactions"].empty()) {
                non_empty.push_back(block);
            }
        }
        std::vector<json>& final_set = non_empty.empty() ? chain_valid : non_empty;

        std::sort(final_set.begin(), final_set.end(), [](const json& a, const json& b) {
            return a["current_hash"].get<std::string>() < b["current_hash"].get<std::string>();
        });

        return final_set[0];
    }

    /*
    Decision on Block (Consensus Decision): Once a node has collected all available
    proposals (or marked some peers as crashed), it decides on the block for this round:
    • If at least one proposed block contains at least one transaction, then ignore
    any empty block proposals.
    • Among the remaining blocks, choose the block whose current_hash is lexicographically smallest (treat the hash as a hexadecimal string).
    This ensures that all correct nodes choose the same block (denote it as B_decided).
*/

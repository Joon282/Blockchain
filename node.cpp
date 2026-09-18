#include "node.hpp"
#include "validator.hpp"
#include "consensus.hpp"
#include <iostream>
#include <nlohmann/json.hpp>
#include <thread>

using json = nlohmann::json;

Node::Node(const json& config, const std::vector<PeerAddress>& peers, uint16_t port){
    this->port = port;
    this->peers = peers;
    this->blockchain.push_back(config);
    for (const auto& peer : peers) {
        send_mutexes[peer_key(peer)] = std::make_unique<std::mutex>();
    }
    std::thread([this]() {
        io_context.restart();
        connect_peers(io_context, this->peers, connections, sockets_mtx);
        peers_connected = true;
    }).detach();
}

void Node::run_consensus(){
    Consensus consensus(*this);
    consensus.run_consensus();
}

bool Node::add_transaction(const json& transaction){
    bool accepted;
    {
        std::lock_guard<std::mutex> lock(mem_pool_mtx);
        accepted = Validator::validate_transaction(transaction, nonces, mem_pool);
        if (accepted) {
            mem_pool.push_back(transaction);
            json log_entry;
            log_entry["type"] = "transaction";
            log_entry["payload"] = transaction;
            {
                std::lock_guard<std::mutex> cout_lock(cout_mtx);
                std::cout << log_entry.dump(2) << std::endl;
            }
        }
    }
    if (accepted) {
        json msg;
        msg["type"] = "transaction";
        msg["payload"] = transaction;
        for (const auto& peer : peers) {
            std::string key = peer_key(peer);
            boost::asio::ip::tcp::socket* sock_ptr = nullptr;
            std::mutex* send_mtx_ptr = nullptr;
            {
                std::lock_guard<std::mutex> lock(sockets_mtx);
                if (crashed_peers.count(key) || !connections.count(key) || !send_mutexes.count(key)) {
                    continue;
                }
                sock_ptr = connections.at(key).get();
                send_mtx_ptr = send_mutexes.at(key).get();
            }
            try {
                std::lock_guard<std::mutex> send_lock(*send_mtx_ptr);
                struct timeval tv;
                tv.tv_sec = 2;
                tv.tv_usec = 0;
                setsockopt(sock_ptr->native_handle(), SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
                send_msg(*sock_ptr, msg);
                receive_msg(*sock_ptr);
                tv.tv_sec = 0;
                tv.tv_usec = 0;
                setsockopt(sock_ptr->native_handle(), SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
            } catch (...) {
                std::lock_guard<std::mutex> lock(sockets_mtx);
                crashed_peers.insert(key);
            }
        }
        cv.notify_one();
    }
    return accepted;
}

void Node::commit_block(const json& block){
    std::lock_guard<std::mutex> block_lock(block_mtx);
    std::lock_guard<std::mutex> mem_pool_lock(mem_pool_mtx);
    for (const auto& it: block["transactions"]) {
        std::string sender = it["sender"];
        nonces[sender] = it["nonce"].get<int>() + 1;
    }
    std::vector<json> new_mem_pool;
    for (const auto& it : mem_pool) {
        std::string sender = it["sender"];
        if (nonces[sender] == it["nonce"].get<int>()) {
            new_mem_pool.push_back(it);
        }
    }
    mem_pool = std::move(new_mem_pool);
    blockchain.push_back(block);
}


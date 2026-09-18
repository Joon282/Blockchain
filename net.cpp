#include "net.hpp"
#include "node.hpp"
#include <nlohmann/json.hpp>
#include <boost/asio.hpp>
#include <thread>
#include <chrono>
#include <iostream>
#include "hasher.hpp"

using json = nlohmann::json;

std::string peer_key(const PeerAddress& p){
    return p.host + ":" + std::to_string(p.port);
}
void send_msg(boost::asio::ip::tcp::socket& sock, const nlohmann::json& msg){
    std::string message = msg.dump();
    if (message.size() > 65535) {
        throw std::runtime_error("Message length is too large");
    }
    uint16_t length = static_cast<uint16_t>(message.size());
    std::vector<uint8_t> buffer;
    buffer.reserve(2 + message.size());
    buffer.push_back(static_cast<uint8_t>(length >> 8));
    buffer.push_back(static_cast<uint8_t>(length & 0xFF));
    buffer.insert(buffer.end(), message.begin(), message.end());
    boost::asio::write(sock, boost::asio::buffer(buffer));
}

json receive_msg(boost::asio::ip::tcp::socket& sock){
    boost::system::error_code ec;
    uint8_t buffer[2];
    boost::asio::read(sock, boost::asio::buffer(buffer, 2), ec);
    if (ec) {
        throw std::runtime_error("Failed to receive message header");
    }
    uint16_t length = (static_cast<uint16_t>(buffer[0]) << 8) | static_cast<uint16_t>(buffer[1]);
    std::vector<char> message_buffer(length);
    boost::asio::read(sock, boost::asio::buffer(message_buffer), ec);
    if (ec) {
        throw std::runtime_error("Failed to receive message body");
    }
    return json::parse(message_buffer.begin(), message_buffer.end());
}

void send_response(boost::asio::ip::tcp::socket& sock, bool accepted){
    send_msg(sock, json(accepted));
}
void connect_peers(boost::asio::io_context& io_context, const std::vector<PeerAddress>& peers, std::unordered_map<std::string, std::unique_ptr<boost::asio::ip::tcp::socket>>& sockets, std::mutex& mtx){
    for (const auto& peer : peers) {
        std::string key = peer_key(peer);
        while (true) {
            try {
                auto socket = std::make_unique<boost::asio::ip::tcp::socket>(io_context);
                boost::asio::ip::tcp::resolver resolver(io_context);
                auto endpoints = resolver.resolve(peer.host, std::to_string(peer.port));
                boost::asio::connect(*socket, endpoints);
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    sockets[key] = std::move(socket);
                }
                break;
            } catch (const std::exception& e) {
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    if (sockets.count(key)) break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }
}
void handle_peer(boost::asio::ip::tcp::socket socket, Node& node){
    while (true) {
        try {
            nlohmann::json transaction = receive_msg(socket);
            std::string type = transaction["type"].get<std::string>();
            if (type == "transaction") {
                json transaction_message = transaction["payload"];
                bool accepted = node.add_transaction(transaction_message);
                send_response(socket, accepted);
            } else if (type == "values") {
                json proposal;
                {
                    std::lock_guard<std::mutex> block_lock(node.block_mtx);
                    std::lock_guard<std::mutex> pool_lock(node.mem_pool_mtx);
                    int our_last_index = node.blockchain.back()["index"].get<int>();
                    int expected_next = our_last_index + 1;

                    int peer_index = expected_next;
                    if (!transaction["payload"].empty() && transaction["payload"][0].contains("index")) {
                        peer_index = transaction["payload"][0]["index"].get<int>();
                    }

                    if (peer_index >= 1 && peer_index <= our_last_index) {
                        proposal = node.blockchain[peer_index - 1];
                    } else {
                        proposal["index"] = expected_next;
                        proposal["previous_hash"] = node.blockchain.back()["current_hash"].get<std::string>();
                        proposal["transactions"] = node.mem_pool;
                        proposal["current_hash"] = Hasher::hash(proposal);

                        bool non_empty_incoming = (!transaction["payload"].empty() && !transaction["payload"][0]["transactions"].empty());
                        if (non_empty_incoming || !node.mem_pool.empty()) {
                            node.pending_values = expected_next;
                            node.cv.notify_one();
                        }
                    }
                }
                json response;
                response["type"] = "values";
                response["payload"] = json::array({proposal});
                send_msg(socket, response);
            }
        } catch (const boost::system::system_error& ec) {
            if (ec.code() == boost::asio::error::eof || ec.code() == boost::asio::error::connection_reset) {
                break;
            }
            break;
        } catch (const std::exception& e) {
            break;
        }
    }
}

void Node::run_server() {
    boost::asio::io_context server_ioc;
    boost::asio::ip::tcp::acceptor acceptor(server_ioc);
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
    acceptor.open(endpoint.protocol());
    acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor.bind(endpoint);
    acceptor.listen();
    while (true) {
        try {
            boost::asio::ip::tcp::socket socket(server_ioc);
            acceptor.accept(socket);
            std::thread([s = std::move(socket), &n = *this]() mutable {
                handle_peer(std::move(s), n);
            }).detach();
        } catch (const std::exception& e) {
        }
    }
}

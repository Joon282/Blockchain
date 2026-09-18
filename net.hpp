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
    uint16_t    port;
};

std::string peer_key(const PeerAddress& p);
void send_msg(boost::asio::ip::tcp::socket& sock, const nlohmann::json& msg);
nlohmann::json receive_msg(boost::asio::ip::tcp::socket& sock);
void send_response(boost::asio::ip::tcp::socket& sock, bool);
void connect_peers(boost::asio::io_context& io_context, const std::vector<PeerAddress>& peers, std::unordered_map<std::string, std::unique_ptr<boost::asio::ip::tcp::socket>>& sockets, std::mutex& mtx);
class Node;
void handle_peer(boost::asio::ip::tcp::socket socket, Node& node);

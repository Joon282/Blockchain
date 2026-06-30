#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <fstream>
#include <vector>
#include <filesystem>
#include "node.hpp"
#include "net.hpp"
#include "hasher.hpp"
#include "consensus.hpp"

using json = nlohmann::json;

int main(int argc, char* argv[]) {
    if (argc != 3){
        std::cerr << "Usage: ./Run.sh <Port-Server> <Node-List-File>" << std::endl;
        return 1;
    }
    uint16_t port = std::stoi(argv[1]);
    if (port < 0){
        std::cerr << "Port number must be a positive integer." << std::endl;
        return 1;
    }
    std::vector<PeerAddress> peers;
    std::ifstream file(argv[2]);
    if (!file.is_open()) {
        std::cerr << "Failed to open node list file: " << argv[2] << std::endl;
        return 1;
    }
    std::string line;
    while (std::getline(file, line)){
        if (line.empty()){
            continue;
        }
        if (!line.empty() && line.back() == '\r') {
            line.pop_back(); 
        }
        if (line.find_first_not_of(" \t\n\r") == std::string::npos) {
            continue;
        }
        auto pos = line.find(':');
        if (pos == std::string::npos){
            continue;
        }
        std::string host = line.substr(0, pos);
        uint16_t peerPort = std::stoi(line.substr(pos + 1));
        peers.push_back({host, peerPort});
    }
    file.close();
    
    json genesis;
    genesis["index"] = 1;
    genesis["transactions"] = json::array();
    genesis["previous_hash"] = std::string(64, '0');
    genesis["current_hash"] = Hasher::hash(genesis);

    Node node(genesis, peers, port);
    std::thread serverThread(&Node::run_server, &node);
    std::thread consensusThread(&Node::run_consensus, &node);
    serverThread.join();
    consensusThread.join();
    return 0;
}
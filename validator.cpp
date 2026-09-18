#include "validator.hpp"
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <algorithm>
#include <string>
#include <iostream>
static std::vector<unsigned char> decode_hex(const std::string& hex);
static int verify_ed25519(EVP_PKEY* pkey, const unsigned char* sig, size_t sig_len, const unsigned char* msg, size_t msg_len);
using json = nlohmann::json;

bool Validator::validate_transaction(const json& transaction, const std::unordered_map<std::string, int>& nonces, const std::vector<json>& mem_pool){
    if (!transaction.contains("sender") || !transaction.contains("message") || !transaction.contains("nonce") || !transaction.contains("signature")) {
        return false;
    }
    if (!transaction["sender"].is_string() || !transaction["message"].is_string() || !transaction["nonce"].is_number_integer() || !transaction["signature"].is_string()) {
        return false;
    }
    std::string sender = transaction["sender"].get<std::string>();
    std::string message = transaction["message"].get<std::string>();
    int nonce = transaction["nonce"].get<int>();
    std::string signature = transaction["signature"].get<std::string>();

    if (!valid_hex(sender, 64) || !valid_hex(signature, 128) || !valid_message(message)) {
        return false;
    }
    if (nonce < 0) {
        return false;
    }

    int new_nonce = 0;
    auto it = nonces.find(sender);
    if (it != nonces.end()) {
        new_nonce = it->second;
    }
    if (nonce != new_nonce) {
        return false;
    }

    if (!verify_signature(transaction)) {
        return false;
    }
    for (const auto& pool_transaction : mem_pool) {
        if (pool_transaction["sender"] == sender && pool_transaction["nonce"] == nonce) {
            return false;
        }
    }
    return true;
}
bool Validator::valid_hex(const std::string& str, size_t len){
    if (str.size() != len) {
        return false;
    }
    for (const char& c : str) {
        if (!std::isxdigit(c) || std::isupper(c)) {
            return false;
        }
    }
    return true;
}

bool Validator::valid_message(const std::string& str){
    if (str.size() > 70){
        return false;
    }
    for (const char& c : str) {
        if (!std::isalnum(c) && c != ' ' && c != '-') {
            return false;
        }
    }
    return true;
}

std::vector<unsigned char> decode_hex(const std::string& hex) {
    std::vector<unsigned char> bytes;
    for (size_t i = 0; i < hex.size(); i += 2) {
        unsigned char byte = (std::stoi(hex.substr(i, 2), nullptr, 16) & 0xFF);
        bytes.push_back(byte);
    }
    return bytes;
}

int verify_ed25519(EVP_PKEY* pkey, const unsigned char* sig, size_t sig_len, const unsigned char* msg, size_t msg_len) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (ctx == nullptr) {
        return 0;
    }
    if (EVP_DigestVerifyInit(ctx, NULL, NULL, NULL, pkey) != 1) {
        EVP_MD_CTX_free(ctx);
        return 0;   
    } 
    int valid = EVP_DigestVerify(ctx, sig, sig_len, msg, msg_len) == 1;
    EVP_MD_CTX_free(ctx);
    return valid == 1;
}

bool Validator::verify_signature(const json& transaction) {
    std::string sender    = transaction["sender"].get<std::string>();
    std::string message   = transaction["message"].get<std::string>();
    int nonce     = transaction["nonce"].get<int>();
    std::string signature = transaction["signature"].get<std::string>();

    std::string string_signature =  "{\"message\": \"" + message + "\", \"nonce\": " + std::to_string(nonce) + ", \"sender\": \"" + sender + "\"}";

    std::vector<unsigned char> sender_b    = decode_hex(transaction["sender"].get<std::string>());
    std::vector<unsigned char> signature_b = decode_hex(transaction["signature"].get<std::string>());

    EVP_PKEY* pkey = EVP_PKEY_new_raw_public_key(
        EVP_PKEY_ED25519, nullptr,
        sender_b.data(), sender_b.size());
    if (!pkey) {
        return false;
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) { EVP_PKEY_free(pkey); return false; }

    if (EVP_DigestVerifyInit(ctx, nullptr, nullptr, nullptr, pkey) != 1) {
        EVP_MD_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return false;
    }

    const unsigned char* msg = reinterpret_cast<const unsigned char*>(string_signature.c_str());
    int result = EVP_DigestVerify(ctx, signature_b.data(), signature_b.size(), msg, string_signature.size());

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    return result == 1;
}


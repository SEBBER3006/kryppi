#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <iomanip>
#include <ctime>
#include <cstdlib>
#include "kryp.hpp"

// hex helper functions
std::string kryp::string_to_hex(const std::string& data) {
    std::ostringstream oss;
    for (unsigned char c : data) {
        oss << std::hex << std::setw(2) << std::setfill('0') 
        << std::uppercase << static_cast<int>(c);
    }
    return oss.str();
}

std::string kryp::hex_to_string(const std::string& hex) {
    std::string out;
    for (size_t i = 0; i < hex.length(); i += 2) {
        unsigned char byte = static_cast<unsigned char>(std::stoi(
            hex.substr(i, 2), nullptr, 16));
        out += byte;
    }
    return out;
}

// XOR encryption/decryption

std::string kryp::xor_cryption(const std::string& text, const std::string& key) {
    std::string result = text;
    for (size_t i = 0; i < text.size(); ++i) {
        result[i] = text[i] ^ key[i % key.size()];
    }
    return result;
}






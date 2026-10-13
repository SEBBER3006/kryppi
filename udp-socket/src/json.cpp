#include "json.hpp"
#include "json.h"
#include <iostream>
#include <string>
#include <ctime>
#include <fstream>
#include <cstdlib>
#include <vector>

using json = nlohmann::json;
int returnbytes = 0;

std::string jsonwriteread::jsonwrite(const std::string& targetIp, const std::string& msg) {
    json j;
    j["modtager_ip"] = targetIp;
    j["besked"] = msg;
    j["tid"] = std::time(nullptr);

    if (returnbytes == 3) {
        // std::cout << "Spoofing opdaget, besked ikke sendt til IP: " << targetIp << std::endl;
        return j.dump();
    }

    try {
        std::ofstream File("write.json", std::ios::app);
        if (File.is_open()) {
            File << j.dump(4) << std::endl;
            File.close();
            // std::cout << "Besked gemt i write.json" << std::endl;
        }
    }
    catch (const std::exception& e) {
        // std::cerr << "Fejl: " << e.what() << std::endl;
    }
    return j.dump();
}

std::string jsonwriteread::jsonread(const std::string& senderIp, const std::string& payload) {
    try {
        json modtagetJson = json::parse(payload);
        

        if (returnbytes == 3) {
            // std::cout << "Spoofing opdaget, ignorerer besked fra IP: " << senderIp << std::endl;
            return modtagetJson["besked"];
        }

        json reading;
        reading["sender_ip"] = senderIp;
        reading["modtager_ip"] = modtagetJson["modtager_ip"];
        reading["besked"] = modtagetJson["besked"];
        reading["tid"] = std::time(nullptr);

        std::ofstream File("read.json", std::ios::app);
        if (File.is_open()) {
            File << reading.dump(4) << std::endl;
            File.close();
            // std::cout << "Besked gemt i read.json" << std::endl;
        }

        return reading["besked"];
    }
    catch (const std::exception& e) {
        // std::cerr << "Fejl: " << e.what() << std::endl;
        return "";
    }
}

std::string getMeshPath(const std::string& filename) {
    const char* homeDir = std::getenv("HOME");
    if (homeDir == nullptr) {
        return filename; // Fallback til lokal sti hvis HOME ikke findes
    }
    return std::string(homeDir) + "/mesh/" + filename;
}

std::string readIpFromFile() {
    // Dynamisk sti
    std::string fullPath = getMeshPath("node_ip.txt");
    std::ifstream File(fullPath);
    
    if (!File.is_open()) {
        // std::cerr << "Fejl: Kunne ikke åbne IP-fil på: " << fullPath << std::endl;
        return "";
    }
    
    std::string ip;
    std::getline(File, ip);
    if (!ip.empty() && ip.back() == '\r') ip.pop_back(); // Håndter Windows/Linux line endings
    File.close();
    return ip;
}

std::vector<json> getHistoryFromIp(const std::string& SearchIp, const std::string& filename) {
    std::vector<json> results;
    std::ifstream File(filename);
    if (!File.is_open()) {
        // std::cerr << "Fejl: Kunne ikke åbne filen: " << filename << std::endl;
        return results;
    }

    json j;
    while (File >> j) {
        bool match = false;
        if (j.contains("sender_ip") && j["sender_ip"] == SearchIp) {
            match = true;
        }
        if (j.contains("modtager_ip") && j["modtager_ip"] == SearchIp) {
            match = true;
        }
        if (match) {
            results.push_back(j);
        }

}

File.close();
    return results;
}
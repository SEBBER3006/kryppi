#include "../../kryptering/include/json.hpp"
#include <iostream>
#include <string>
#include <ctime>
#include <fstream>

using json = nlohmann::json;
int returnbytes = 0;

std::string jsonwrite(const std::string& targetIp, const std::string& msg) {
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
            //std::cout << "Besked gemt i write.json" << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Fejl: " << e.what() << std::endl;
    }
    return j.dump();
}

std::string jsonread(const std::string& senderIp, const std::string& payload) {
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
        std::cerr << "Fejl: " << e.what() << std::endl;
        return "";
    }
}
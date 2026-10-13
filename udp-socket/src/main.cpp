#include "udp_socket.hpp"
#include "config.hpp"
#include "kryp.hpp"
#include "json.hpp"
#include "json.h"
 
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <limits>
#include <cstdlib>
#include <ctime>
 

int main(int argc, char* argv[]) {
    srand(static_cast<unsigned>(time(nullptr)));
 
    std::string myIp;
    if (argc >= 2) {
        myIp = argv[1];
    } else {
        myIp = readIpFromFile();
        if (myIp.empty()) {
            std::cerr << "Fejl: Ingen IP angivet og "
                      << NODE_IP_FILE << " ikke fundet.\n"
                      << "Brug: " << argv[0] << " <ip> [broadcast-adresse]\n"
                      << "Eks:  " << argv[0] << " 10.0.0.1 127.255.255.255"
                      << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::string bcastAddr = BROADCAST_ADDR;
    if (argc >= 3) bcastAddr = argv[2];

    MeshNode node(myIp, bcastAddr);

    std::cout << "Node klar | IP: " << myIp
              << " | Broadcast: " << bcastAddr
              << " | Port: " << PORT << std::endl;
    std::cout << "Format: <target-ip> [Enter] <besked> [Enter] <noegle> [Enter]" << std::endl;

    node.on_receive = [](const std::string& sender, const std::string& rawEncrypted) -> bool {
        std::string decrypted = kryp::xor_cryption(rawEncrypted, CRYPTO_KEY);
        std::string msg = jsonwriteread::jsonread(sender, decrypted);
        if (msg.empty()) return false;
        std::cout << "[Modtaget] " << sender << ": " << msg << std::endl;
        return true;
    };

    std::thread receiver(&MeshNode::receiveAndSend, &node);
    receiver.detach();

    while (true) {
        std::string targetIp, besked, encryptionKey;
        std::cout << "\nIndtast modtager IP: ";
        std::cin  >> targetIp;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        std::cout << "Indtast besked: ";
        std::getline(std::cin, besked);
        if (besked.empty()) {
            std::cout << "Fejl: Besked maa ikke vaere tom." << std::endl;
            continue;
        }

        std::cout << "Indtast krypteringsnoegle: ";
        std::getline(std::cin, encryptionKey);
        if (encryptionKey.empty()) {
            std::cout << "Fejl: Noegle maa ikke vaere tom." << std::endl;
            continue;
        }

        node.createAndSend(targetIp, besked, encryptionKey);
        std::cout << "[Sendt] -> " << targetIp << ": " << besked << std::endl;
    }
 
    return EXIT_SUCCESS;
}
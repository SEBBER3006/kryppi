#include "../include/mesh_node.h"
#include "../../kryptering/include/kryp.hpp"
#include <cstring>
#include <cstdlib>

const std::string CRYPTO_KEY = "kryppi_key";

std::string jsonwrite(const std::string& targetIp, const std::string& msg);
std::string jsonread(const std::string& senderIp, const std::string& payload);

MeshNode::MeshNode(const std::string& ip) : myIp(ip) {
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    int broadcastEnable = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

    nodeAddr.sin_family = AF_INET;
    nodeAddr.sin_addr.s_addr = INADDR_ANY;
    nodeAddr.sin_port = htons(PORT);

    bind(sock, (struct sockaddr *)&nodeAddr, sizeof(nodeAddr));
}

bool MeshNode::isDuplicate(uint32_t id) {
    std::lock_guard<std::mutex> lock(mtx);
    if (seenMessages.find(id) != seenMessages.end()) return true;
    seenMessages.insert(id);
    if (seenMessages.size() > 100) seenMessages.erase(seenMessages.begin());
    return false;
}

void MeshNode::broadcast(MeshPacket pkt) {
    struct sockaddr_in bcAddr;
    bcAddr.sin_family = AF_INET;
    bcAddr.sin_addr.s_addr = inet_addr("255.255.255.255");
    bcAddr.sin_port = htons(PORT);
    sendto(sock, &pkt, sizeof(pkt), 0, (struct sockaddr *)&bcAddr, sizeof(bcAddr));
}

void MeshNode::receiveLoop() {
    while (true) {
        MeshPacket incoming;
        struct sockaddr_in from;
        socklen_t fromLen = sizeof(from);

        int n = recvfrom(sock, &incoming, sizeof(incoming), 0, (struct sockaddr *)&from, &fromLen);
        if (n <= 0) continue;

        if (!isDuplicate(incoming.messageId)) {
            if (std::string(incoming.targetIp) == myIp) {
                std::string rawData = hex_to_string(std::string(incoming.payload));
                std::string decryptedJson = xor_cryption(rawData, CRYPTO_KEY);
                std::string text = jsonread(incoming.senderIp, decryptedJson);
                if (on_receive) on_receive(incoming.senderIp, text);
            } else {
                broadcast(incoming);
            }
        }
    }
}

void MeshNode::createAndSend(const std::string& target, const std::string& text) {
    std::string jsonData = jsonwrite(target, text);
    std::string encryptedData = xor_cryption(jsonData, CRYPTO_KEY);
    std::string hexPayload = string_to_hex(encryptedData);

    MeshPacket p;
    p.messageId = rand();
    strncpy(p.targetIp, target.c_str(), 16);
    strncpy(p.senderIp, myIp.c_str(), 16);
    strncpy(p.payload, hexPayload.c_str(), 512);

    isDuplicate(p.messageId);
    broadcast(p);
}

#ifdef STANDALONE
#include <iostream>
#include <thread>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "Du har ikke givet en ip" << std::endl;
        return 1;
    }
    srand(time(NULL));

    MeshNode node(argv[1]);
    std::cout << "Node startet på IP: " << argv[1] << std::endl;

    std::thread t(&MeshNode::receiveLoop, &node);

    while (true) {
        std::string tip, msg;
        std::cout << "\nIndtast modtager IP: ";
        std::cin >> tip;
        std::cout << "Indtast besked: ";
        std::cin.ignore();
        std::getline(std::cin, msg);
        node.createAndSend(tip, msg);
    }
    t.join();
    return 0;
}
#endif

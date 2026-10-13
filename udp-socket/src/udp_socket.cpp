#include <iostream>
#include <cstring>
#include <cstdlib>
#include <chrono>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "udp_socket.hpp"
#include "config.hpp"
#include "kryp.hpp"
#include "json.hpp"
#include "json.h"

// Hjælpefunktione Json
extern int returnbytes;

// Hjælpefunktion Kryp
/*
std::string string_to_hex(const std::string& data);
std::string hex_to_string(const std::string& hex);
std::string xor_cryption(const std::string& text, const std::string& key);
*/

// Unikt id (fundet på stackoverflow)
static uint32_t generateMessageId()
{
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    return static_cast<uint32_t>(now ^ (rand() & 0xFFFFFFFF));
}

/// @brief Node constructor
/// @param ip node ip-address
/// @param bcastAddr broadcast address (default: BROADCAST_ADDR from config)
MeshNode::MeshNode(const std::string &ip, const std::string &bcastAddr)
    : myIp(ip), broadcastAddr(bcastAddr)
{
    // Create a socket
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    { // ERR: Failed creating socket
        // perror("socket");
        exit(EXIT_FAILURE);
    }

    // Allow broadcast on mesh
    int broadcastEnable = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
               &broadcastEnable, sizeof(broadcastEnable));

    // Allow multiple processes to bind to the same port (local testing)
    int reusePort = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &reusePort, sizeof(reusePort));

    // Node configurations
    nodeAddr.sin_family = AF_INET;
    nodeAddr.sin_addr.s_addr = INADDR_ANY;
    nodeAddr.sin_port = htons(PORT);

    // Bind the socket
    if (bind(sock, reinterpret_cast<struct sockaddr *>(&nodeAddr),
             sizeof(nodeAddr)) < 0)
    {
        // perror("bind");
        exit(EXIT_FAILURE);
    }

    // Node is ready
    // std::cout << "Node Lytter på port " << PORT
    //          << " | Egen IP: " << myIp << std::endl;
}

/// @brief Check if MSG ID has already been used!
/// @param id ID to check
/// @return true: if the ID is duplicate, false: if check fails
bool MeshNode::isDuplicate(uint32_t id)
{
    std::lock_guard<std::mutex> lock(mtx);
    if (seenMessages.count(id))
        return true;
    // Insert to set
    seenMessages.insert(id);
    if (seenMessages.size() > 100000)
        seenMessages.erase(seenMessages.begin());
    return false;
}

/// @brief Broadcasting package through mesh
/// @param pkt package to broadcast
void MeshNode::broadcastPacket(const MeshPacket &pkt)
{
    // Add socket addr
    struct sockaddr_in bcAddr{};
    // Configure socket
    bcAddr.sin_family = AF_INET;
    bcAddr.sin_addr.s_addr = inet_addr(broadcastAddr.c_str());
    bcAddr.sin_port = htons(PORT);
    // Broadcast socket
    sendto(sock, &pkt, sizeof(pkt), 0,
           reinterpret_cast<struct sockaddr *>(&bcAddr), sizeof(bcAddr));
}

/// @brief The receiver threaded loop of the node
void MeshNode::receiveAndSend()
{
    while (true)
    {
        MeshPacket incoming{};
        struct sockaddr_in from{};
        socklen_t fromLen = sizeof(from);

        int n = recvfrom(sock, &incoming, sizeof(incoming), 0,
                         reinterpret_cast<struct sockaddr *>(&from), &fromLen);
        if (n < 0 && errno == EWOULDBLOCK)
            continue;
        if (isDuplicate(incoming.messageId))
            continue;

        std::string target(incoming.targetIp);
        std::string sender(incoming.senderIp);

        if (target == myIp)
        {
            std::string payloadStr(incoming.payload);

            // Detekter indkommende ACKs — spring videre behandling over
            bool isAck = payloadStr.rfind("ACK_RECV", 0) == 0 || payloadStr.rfind("ACK_DECRYPTED", 0) == 0;
            if (isAck)
            {
                uint32_t refId = 0;
                auto col = payloadStr.find(':');
                if (col != std::string::npos)
                {
                    try
                    {
                        refId = static_cast<uint32_t>(std::stoul(payloadStr.substr(col + 1)));
                    }
                    catch (...)
                    {
                    }
                }
                std::string ackType = payloadStr.substr(0, col != std::string::npos ? col : payloadStr.size());
                if (on_ack)
                    on_ack(sender, ackType, refId);
                continue;
            }

            // 1. Send ACK_RECV straks ved modtagelse
            {
                MeshPacket ack{};
                ack.messageId = generateMessageId();
                strncpy(ack.targetIp, sender.c_str(), sizeof(ack.targetIp) - 1);
                strncpy(ack.senderIp, myIp.c_str(), sizeof(ack.senderIp) - 1);
                std::string ackPayload = "ACK_RECV:" + std::to_string(incoming.messageId);
                strncpy(ack.payload, ackPayload.c_str(), sizeof(ack.payload) - 1);
                isDuplicate(ack.messageId);
                broadcastPacket(ack);
            }

            // 2. Hex-afkod payload
            std::string rawEncrypted = kryp::hex_to_string(payloadStr);
            if (!rawEncrypted.empty())
            {
                // 3. Send ACK_DECRYPTED kun efter vellykket dekryptering
                bool ok = on_receive && on_receive(sender, rawEncrypted);
                if (ok)
                {
                    MeshPacket ackDec{};
                    ackDec.messageId = generateMessageId();
                    strncpy(ackDec.targetIp, sender.c_str(), sizeof(ackDec.targetIp) - 1);
                    strncpy(ackDec.senderIp, myIp.c_str(), sizeof(ackDec.senderIp) - 1);
                    std::string ackDecPayload = "ACK_DECRYPTED:" + std::to_string(incoming.messageId);
                    strncpy(ackDec.payload, ackDecPayload.c_str(), sizeof(ackDec.payload) - 1);
                    isDuplicate(ackDec.messageId);
                    broadcastPacket(ackDec);
                }
            }
        }
        else
        {
            broadcastPacket(incoming);
        }
    }
}

/// @brief Create a package and send through the mesh to target
/// @param target the ip-address of the target
/// @param text the message to send
/// @param encryptionKey the encryption key to encrypt msg
uint32_t MeshNode::createAndSend(
    const std::string &target,
    const std::string &text,
    const std::string &encryptionKey)
{
    // Try to write data to JSON
    std::string jsonData = jsonwriteread::jsonwrite(target, text);
    if (jsonData.empty())
    { // ERR: Couldn't write package
        return 0;
    }

    if (encryptionKey.empty())
    { // ERR: No enc key!
        return 0;
    }

    // Transform JSON to hex payload
    std::string hexPayload = kryp::string_to_hex(kryp::xor_cryption(jsonData, encryptionKey));

    if (hexPayload.size() >= 512)
    { // ERR: Message length
        return 0;
    }

    // Generate package
    MeshPacket pkt{};
    pkt.messageId = generateMessageId();
    strncpy(pkt.targetIp, target.c_str(), sizeof(pkt.targetIp) - 1);
    strncpy(pkt.senderIp, myIp.c_str(), sizeof(pkt.senderIp) - 1);
    strncpy(pkt.payload, hexPayload.c_str(), sizeof(pkt.payload) - 1);

    // Check if pkg ID is duplicate, if yes continue
    if (isDuplicate(pkt.messageId))
        return 0;

    // Broadcast pkg
    broadcastPacket(pkt);
    return pkt.messageId;
}

// Stupid helper function can't be in main
std::string MeshNode::getMsg(std::string &sender, std::string &decryptedJson)
{
    return jsonwriteread::jsonread(sender, decryptedJson);
}

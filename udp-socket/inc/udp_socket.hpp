#pragma once

#include <string>
#include <set>
#include <mutex>
#include <functional>
#include <queue>
#include <netinet/in.h>

#include "config.hpp"

//Mesh-pakke
struct MeshPacket {
    uint32_t messageId = -1;
    char     targetIp[16];
    char     senderIp[16];
    char     payload[512];
};

//MeshNode
class MeshNode {
private:
    int                sock;
    struct sockaddr_in nodeAddr;
    std::string        myIp;
    std::string        broadcastAddr;
    std::set<uint32_t> seenMessages;
    std::mutex         mtx;

    bool isDuplicate(uint32_t id);
    void broadcastPacket(const MeshPacket& pkt);

public:
    explicit MeshNode(const std::string& ip,
                      const std::string& bcastAddr = BROADCAST_ADDR);
    
    std::string getMsg(std::string &sender, std::string &decryptedJson);

    std::function<bool(const std::string&, const std::string&)> on_receive;
    std::function<void(const std::string&, const std::string&, uint32_t)> on_ack;

    std::queue<MeshPacket> recievedMsgs;
    void receiveAndSend();
    uint32_t createAndSend(const std::string& target, const std::string& text, const std::string& encryptionKey);
};

std::string readIpFromFile();
#pragma once
#include <functional>
#include <string>
#include <set>
#include <mutex>
#include <cstdint>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8888

struct MeshPacket {
    uint32_t messageId;
    char targetIp[16];
    char senderIp[16];
    char payload[512];
};

class MeshNode {
public:
    // Called on the receive thread when a message arrives for this node
    std::function<void(const std::string&, const std::string&)> on_receive;

    explicit MeshNode(const std::string& ip);
    void broadcast(MeshPacket pkt);
    void receiveLoop();
    void createAndSend(const std::string& target, const std::string& text);

private:
    int sock;
    struct sockaddr_in nodeAddr;
    std::string myIp;
    std::set<uint32_t> seenMessages;
    std::mutex mtx;

    bool isDuplicate(uint32_t id);
};

#pragma once
#include "draw_tui.h"
#include <string>
#include <vector>
#include <mutex>
#include <cstdint>

class msg_handler {
public:
    msg_handler(Screen& scr, std::vector<Chat>& chats, int& cidx, std::mutex& mtx);

    // incoming message from network
    bool receive_message(const std::string& from_ip, const std::string& text);

    // incoming ACK from network
    void receive_ack(const std::string& from_ip, const std::string& ack_type, uint32_t msgId);

    // outgoing message typed by user
    void send_message(const std::string& to_ip, const std::string& text);

private:
    Screen&            g_scr;
    std::vector<Chat>& g_chats;
    int&               g_cidx;
    std::mutex&        g_chats_mtx;
};

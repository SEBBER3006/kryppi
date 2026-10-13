#pragma once
#include "draw_tui.h"
#include "msg_handler.h"
#include <functional>
#include <string>
#include <cstdint>

class Run_TUI {
public:
    Run_TUI();
    void run();
    bool receive_message(const std::string& from_ip, const std::string& text);
    void receive_ack(const std::string& from_ip, const std::string& ack_type, uint32_t msgId);
    void send_message(const std::string& to_ip, const std::string& text);
    void set_send_callback(std::function<uint32_t(const std::string&, const std::string&, const std::string&)> cb);
    void set_my_ip(const std::string& ip);

private:
    Draw_TUI   draw;
    msg_handler handler;
};

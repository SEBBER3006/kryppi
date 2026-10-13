#include "msg_handler.h"
#include "../../udp-socket/inc/kryp.hpp"
#include "../../udp-socket/inc/json.h"
#include <ctime>

static std::string hhmm() {
    time_t t = time(nullptr);
    char b[6]; strftime(b, 6, "%H:%M", localtime(&t));
    return b;
}

msg_handler::msg_handler(Screen& scr, std::vector<Chat>& chats, int& cidx, std::mutex& mtx)
    : g_scr(scr), g_chats(chats), g_cidx(cidx), g_chats_mtx(mtx) {}

bool msg_handler::receive_message(const std::string& from_ip, const std::string& text) {
    std::lock_guard<std::mutex> lock(g_chats_mtx);

    int idx = -1;
    for (int i = 0; i < (int)g_chats.size(); i++) {
        if (g_chats[i].ip == from_ip) { idx = i; break; }
    }

    if (idx < 0) {
        Chat c; c.ip = from_ip; c.up = true;
        g_chats.push_back(c);
        idx = (int)g_chats.size() - 1;
    }

    // Store raw ciphertext — draw_chat dekrypterer ved render-tid med chat.key
    g_chats[idx].msgs.push_back({text, hhmm(), false});
    if (g_scr != Screen::Chat || g_cidx != idx)
        g_chats[idx].unread++;

    // Returnér true kun hvis dekryptering med den kendte nøgle lykkes (non-empty JSON)
    const std::string& key = g_chats[idx].key;
    if (key.empty()) return false;
    std::string plain = jsonwriteread::jsonread(from_ip, kryp::xor_cryption(text, key));
    return !plain.empty();
}

void msg_handler::receive_ack(const std::string& from_ip, const std::string& ack_type, uint32_t msgId) {
    std::lock_guard<std::mutex> lock(g_chats_mtx);

    int idx = -1;
    for (int i = 0; i < (int)g_chats.size(); i++)
        if (g_chats[i].ip == from_ip) { idx = i; break; }
    if (idx < 0) {
        Chat c; c.ip = from_ip; c.up = true;
        g_chats.push_back(c);
        idx = (int)g_chats.size() - 1;
    }

    bool marked = false;
    for (auto& m : g_chats[idx].msgs) {
        if (!m.self || m.msgId != msgId) continue;
        if (ack_type == "ACK_RECV")      { m.ack_recv = true;      marked = true; break; }
        if (ack_type == "ACK_DECRYPTED") { m.ack_decrypted = true; marked = true; break; }
    }

    if (!marked) g_chats[idx].ack_excess++;
}

void msg_handler::send_message(const std::string& to_ip, const std::string& text) {
    // Start index
    int idx = -1;

    // Loop through chats to find correct one
    for (int i = 0; i < (int)g_chats.size(); i++) {
        if (g_chats[i].ip == to_ip) { idx = i; break; }
    }

    // If not found create new
    if (idx < 0) {
        Chat c; c.ip = to_ip; c.up = true;
        g_chats.push_back(c);
        idx = (int)g_chats.size() - 1;
    }

    // Push new message back
    g_chats[idx].msgs.push_back({text, hhmm(), true}); 
}


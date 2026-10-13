#include "run_tui.h"
#include <ncurses.h>

Run_TUI::Run_TUI() : handler(draw.g_scr, draw.g_chats, draw.g_cidx, draw.chats_mtx) {}

bool Run_TUI::receive_message(const std::string& from_ip, const std::string& text) {
    return handler.receive_message(from_ip, text);
}

void Run_TUI::receive_ack(const std::string& from_ip, const std::string& ack_type, uint32_t msgId) {
    handler.receive_ack(from_ip, ack_type, msgId);
}

void Run_TUI::send_message(const std::string& to_ip, const std::string& text) {
    handler.send_message(to_ip, text);
}

void Run_TUI::set_send_callback(std::function<uint32_t(const std::string&, const std::string&, const std::string&)> cb) {
    draw.on_send = cb;
}

void Run_TUI::set_my_ip(const std::string& ip) {
    draw.g_myip = ip;
}

void Run_TUI::run() {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    draw.run_splash(rows, cols);
    timeout(100);

    bool running = true;
    while (running) {
        getmaxyx(stdscr, rows, cols);
        erase();

        switch (draw.g_scr) {
            case Screen::Menu:       draw.draw_menu(rows, cols);        break;
            case Screen::Chats:      draw.draw_chats(rows, cols);       break;
            case Screen::NewChat:    draw.draw_new_chat(rows, cols);    break;
            case Screen::ChatUnlock: draw.draw_chat_unlock(rows, cols); break;
            case Screen::Chat:       draw.draw_chat(rows, cols);        break;
            case Screen::Setup:      draw.draw_setup(rows, cols);       break;
            default: break;
        }

        refresh();
        int ch = getch();

        switch (draw.g_scr) {
            case Screen::Menu:       running = draw.handle_menu(ch);  break;
            case Screen::Chats:      draw.handle_chats(ch);           break;
            case Screen::NewChat:    draw.handle_new_chat(ch);        break;
            case Screen::ChatUnlock: draw.handle_chat_unlock(ch);     break;
            case Screen::Chat:       draw.handle_chat(ch);            break;
            case Screen::Setup:      draw.handle_setup(ch);           break;
            default: break;
        }
    }
}

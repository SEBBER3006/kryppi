#pragma once
#include <string>
#include <vector>

// colors
enum { CP_ACCENT=1, CP_TITLE, CP_SEL, CP_WARN, CP_SELF, CP_PEER, CP_STAT, CP_DIM };

// screens
enum class Screen { Splash, Menu, Chats, Chat, Setup, NewChat };

// ── data ──────────────────────────────────────────────────────────────────────
struct Msg  { std::string text, ts; bool self; };
struct Chat { std::string ip; std::vector<Msg> msgs; int unread=0; bool up=false; };



class TUI {

    public:
        TUI();
        ~TUI();

        void run();
        void receive_message(const std::string& from_ip, const std::string& text);

    private:
        // ── global state som alle alle screen tilgår for at vise rigtige stadier.
        Screen            g_scr      = Screen::Splash; //hvilken screen er vi på
        std::vector<Chat> g_chats;                     //Liste over alle chat!
        int               g_menu_sel = 0;              //hvilket menu item er vi på i menu
        int               g_chat_sel = 0;              //hvilken chat er vi på i menu
        int               g_cidx     = -1;             // hvilken chat er åben i chat screen, -1 hvis ingen
        int               g_scroll   = 0;              // scroll i chat screen, 0 = bunde.
        std::string       g_input;                     //Hvad er skrevet i input feltet
        std::string       g_newip;                     //Den nye ip adresse skrevet i new chat

        // ── helpers ───────────────────────────────────────────────────────────────────
        void init_colors();
        void box_at(int y, int x, int h, int w, const char *title=nullptr);
        void sep_at(int y, int cols);
        void draw_status(int rows, int cols, const std::string& left, const std::string& right="");
        void draw_logo(int y, int x);

        // start animation
        void run_splash(int rows, int cols);

        // mainmenu
        void draw_menu(int rows, int cols);
        bool handle_menu(int ch);

        // liste over chats
        void draw_chats(int rows, int cols);
        void handle_chats(int ch);

        // ── NEW CHAT DIALOG ───────────────────────────────────────────────────────────
        void draw_new_chat(int rows, int cols);
        void handle_new_chat(int ch);

        //chat rum screen
        void draw_chat(int rows, int cols);
        void handle_chat(int ch);

        // setup screen
        void draw_setup(int rows, int cols);
        void handle_setup(int ch);
};

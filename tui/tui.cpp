#include "tui.h"
#include <locale.h>
#include <ncurses.h>
#include <chrono>
#include <thread>
#include <ctime>
#include <algorithm>

// ── logo (KRYPPI in block style, visual width = 44 cells) ────────────────────
static constexpr int LOGO_H  = 6;
static constexpr int LOGO_VW = 44;
static const char *LOGO[LOGO_H] = {
    "██╗  ██╗██████╗ ██╗   ██╗██████╗ ██████╗ ██╗",
    "██║ ██╔╝██╔══██╗╚██╗ ██╔╝██╔══██╗██╔══██╗██║",
    "█████╔╝ ██████╔╝ ╚████╔╝ ██████╔╝██████╔╝██║",
    "██╔═██╗ ██╔══██╗  ╚██╔╝  ██╔═══╝ ██╔═══╝ ██║",
    "██║  ██╗██║  ██║   ██║   ██║     ██║     ██║",
    "╚═╝  ╚═╝╚═╝  ╚═╝   ╚═╝   ╚═╝     ╚═╝     ╚═╝",
};

// MAIN MENU
static const char *MENU_ITEMS[] = { "chats", "setup", "quit" };
static constexpr int MENU_N = 3;

// ── helpers ───────────────────────────────────────────────────────────────────
static void msleep(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

//time stamp
static std::string hhmm() {
    time_t t = time(nullptr);
    char b[6]; strftime(b, 6, "%H:%M", localtime(&t));
    return b;
}

TUI::TUI() {
    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    if (has_colors()) init_colors();
}

TUI::~TUI() {
    endwin();
}

void TUI::init_colors() {
    start_color();
    use_default_colors();
    init_pair(CP_ACCENT, COLOR_GREEN,  -1);
    init_pair(CP_TITLE,  COLOR_CYAN,   -1);
    init_pair(CP_SEL,    COLOR_BLACK,  COLOR_GREEN);
    init_pair(CP_WARN,   COLOR_YELLOW, -1);
    init_pair(CP_SELF,   COLOR_GREEN,  -1);
    init_pair(CP_PEER,   COLOR_CYAN,   -1);
    init_pair(CP_STAT,   COLOR_BLACK,  COLOR_GREEN);
    init_pair(CP_DIM,    COLOR_YELLOW,  -1);
}

// draw a bordered box; title centred in top border when non-null
void TUI::box_at(int y, int x, int h, int w, const char *title) {
    attron(COLOR_PAIR(CP_ACCENT));
    mvaddch(y,     x,     ACS_ULCORNER);
    mvaddch(y,     x+w-1, ACS_URCORNER);
    mvaddch(y+h-1, x,     ACS_LLCORNER);
    mvaddch(y+h-1, x+w-1, ACS_LRCORNER);
    mvhline(y,     x+1, ACS_HLINE, w-2);
    mvhline(y+h-1, x+1, ACS_HLINE, w-2);
    mvvline(y+1,   x,   ACS_VLINE, h-2);
    mvvline(y+1,   x+w-1, ACS_VLINE, h-2);
    attroff(COLOR_PAIR(CP_ACCENT));
    if (title) {
        std::string t = std::string(" ") + title + " ";
        int tx = x + (w - (int)t.size()) / 2;
        attron(COLOR_PAIR(CP_ACCENT) | A_BOLD);
        mvaddstr(y, tx, t.c_str());
        attroff(COLOR_PAIR(CP_ACCENT) | A_BOLD);
    }
}

// interior horizontal separator using T-piece connectors
void TUI::sep_at(int y, int cols) {
    attron(COLOR_PAIR(CP_ACCENT));
    mvaddch(y, 0, ACS_LTEE);
    mvhline(y, 1, ACS_HLINE, cols-2);
    mvaddch(y, cols-1, ACS_RTEE);
    attroff(COLOR_PAIR(CP_ACCENT));
}

// bottom status bar (reversed green)
void TUI::draw_status(int rows, int cols,
                      const std::string& left,
                      const std::string& right) {
    attron(COLOR_PAIR(CP_STAT) | A_BOLD);
    std::string bar(cols, ' ');
    mvaddstr(rows-1, 0, bar.c_str());
    if (!left.empty())  mvaddstr(rows-1, 1, left.c_str());
    if (!right.empty()) mvaddstr(rows-1, cols-(int)right.size()-1, right.c_str());
    attroff(COLOR_PAIR(CP_STAT) | A_BOLD);
}

// print the logo block, top-left at (y, x)
void TUI::draw_logo(int y, int x) {
    attron(COLOR_PAIR(CP_TITLE) | A_BOLD);
    for (int i = 0; i < LOGO_H; i++)
        mvaddstr(y+i, x, LOGO[i]);
    attroff(COLOR_PAIR(CP_TITLE) | A_BOLD);
}

// ── SPLASH ANIMATION ─────────────────────────────────────────────────────────
void TUI::run_splash(int rows, int cols) {
    int lx = (cols - LOGO_VW) / 2;
    int ly = (rows - LOGO_H - 7) / 2;
    if (ly < 1) ly = 1;

    // logo lines appear one at a time
    attron(COLOR_PAIR(CP_TITLE) | A_BOLD);
    for (int i = 0; i < LOGO_H; i++) {
        mvaddstr(ly+i, lx, LOGO[i]);
        refresh(); msleep(65);
    }
    attroff(COLOR_PAIR(CP_TITLE) | A_BOLD);

    refresh(); msleep(1020);
    g_scr = Screen::Menu;
}

// MAIN MENU
void TUI::draw_menu(int rows, int cols) {
    int lx = (cols - LOGO_VW) / 2;
    int ly = rows/2 - LOGO_H - 4;
    if (ly < 1) ly = 1;

    draw_logo(ly, lx);

    const char *sub = "  ·  encrypted peer-to-peer mesh network ·  ";
    attron(COLOR_PAIR(CP_DIM));
    mvaddstr(ly+LOGO_H, (cols-(int)strlen(sub))/2, sub);
    attroff(COLOR_PAIR(CP_DIM));

    // notification count for chats badge
    int unread = 0;
    for (auto& c : g_chats) unread += c.unread;

    int mw=24, mh=MENU_N+2;
    int my=ly+LOGO_H+2, mx=(cols-mw)/2;
    box_at(my, mx, mh, mw);

    for (int i = 0; i < MENU_N; i++) {
        std::string item;
        if (i==0 && unread>0)
            item = "  chats [" + std::to_string(unread) + "]  ";
        else
            item = std::string("  ") + MENU_ITEMS[i] + "  ";

        // centre text within the menu box interior
        std::string row(mw-2, ' ');
        int off = (mw-2-(int)item.size()) / 2;
        if (off < 0) off = 0;
        for (int j = 0; j < (int)item.size() && off+j < mw-2; j++)
            row[off+j] = item[j];

        bool sel = (i == g_menu_sel);
        bool hot = (i==0 && unread>0 && !sel);

        if      (sel) attron(COLOR_PAIR(CP_SEL) | A_BOLD);
        else if (hot) attron(COLOR_PAIR(CP_WARN) | A_BOLD);
        mvaddstr(my+1+i, mx+1, row.c_str());
        if      (sel) attroff(COLOR_PAIR(CP_SEL) | A_BOLD);
        else if (hot) attroff(COLOR_PAIR(CP_WARN) | A_BOLD);
    }

    draw_status(rows, cols, "↑↓ navigate  ·  enter select  ·  q quit", hhmm());
}

bool TUI::handle_menu(int ch) {
    switch (ch) {
        case KEY_UP:   g_menu_sel = (g_menu_sel-1+MENU_N) % MENU_N; break;
        case KEY_DOWN: g_menu_sel = (g_menu_sel+1) % MENU_N; break;
        case '\n': case KEY_ENTER:
            if (g_menu_sel==0) g_scr = Screen::Chats;
            if (g_menu_sel==1) g_scr = Screen::Setup;
            if (g_menu_sel==2) return false;
            break;
        case 'q': case 'Q': return false;
    }
    return true;
}

// CHATS LIST
void TUI::draw_chats(int rows, int cols) {
    box_at(0, 0, rows-1, cols, "CHATS");

    attron(COLOR_PAIR(CP_DIM));
    const char *hint = "N new  ·  ESC back";
    mvaddstr(1, cols-(int)strlen(hint)-2, hint);
    attroff(COLOR_PAIR(CP_DIM));

    sep_at(2, cols);

    if (g_chats.empty()) {
        attron(COLOR_PAIR(CP_DIM));
        const char *msg = "no chats yet  ·  press N to connect to a peer";
        mvaddstr((rows-1)/2, (cols-(int)strlen(msg))/2, msg);
        attroff(COLOR_PAIR(CP_DIM));
    } else {
        int ly=3, vis=rows-6;
        for (int i=0; i<(int)g_chats.size() && i<vis; i++) {
            auto& c = g_chats[i];
            std::string dot  = c.up ? "● " : "○ ";
            std::string row  = " " + dot + c.ip;
            if (c.unread > 0)
                row += "  [" + std::to_string(c.unread) + " new]";
            row.resize(cols-2, ' ');

            bool sel = (i == g_chat_sel);
            bool hot = (c.unread>0 && !sel);

            if      (sel) attron(COLOR_PAIR(CP_SEL) | A_BOLD);
            else if (hot) attron(COLOR_PAIR(CP_WARN) | A_BOLD);
            mvaddstr(ly+i, 1, row.c_str());
            if      (sel) attroff(COLOR_PAIR(CP_SEL) | A_BOLD);
            else if (hot) attroff(COLOR_PAIR(CP_WARN) | A_BOLD);
        }
    }

    draw_status(rows, cols,
        "↑↓ select  ·  enter open  ·  N new chat  ·  ESC back", hhmm());
}

void TUI::handle_chats(int ch) {
    switch (ch) {
        case KEY_UP:   if (g_chat_sel > 0) g_chat_sel--; break;
        case KEY_DOWN: if (g_chat_sel < (int)g_chats.size()-1) g_chat_sel++; break;
        case '\n': case KEY_ENTER:
            if (!g_chats.empty()) {
                g_cidx = g_chat_sel;
                g_chats[g_cidx].unread = 0;
                g_scroll = 0; g_input.clear();
                g_scr = Screen::Chat;
            }
            break;
        case 'n': case 'N':
            g_newip.clear();
            g_scr = Screen::NewChat;
            break;
        case 27: g_scr = Screen::Menu; break;
    }
}

// ── NEW CHAT DIALOG ───────────────────────────────────────────────────────────
void TUI::draw_new_chat(int rows, int cols) {
    int dw=54, dh=8;
    int dy=(rows-dh)/2, dx=(cols-dw)/2;

    box_at(dy, dx, dh, dw, "NEW CONNECTION");

    mvaddstr(dy+2, dx+3, "peer IP / hostname:");

    // inner input field box
    box_at(dy+3, dx+2, 3, dw-4);
    attron(A_BOLD);
    std::string disp = g_newip + "_";
    mvaddstr(dy+4, dx+3, disp.c_str());
    attroff(A_BOLD);

    attron(COLOR_PAIR(CP_DIM));
    mvaddstr(dy+6, dx+3, "enter to connect  ·  ESC to cancel");
    attroff(COLOR_PAIR(CP_DIM));

    draw_status(rows, cols,
        "type peer address  ·  enter connect  ·  ESC cancel", hhmm());
}

void TUI::handle_new_chat(int ch) {
    if (ch == 27) { g_scr = Screen::Chats; return; }

    if (ch == '\n' || ch == KEY_ENTER) {
        if (!g_newip.empty()) {
            int found = -1;
            for (int i=0; i<(int)g_chats.size(); i++)
                if (g_chats[i].ip == g_newip) { found=i; break; }
            if (found < 0) {
                Chat c; c.ip = g_newip; c.up = true;
                g_chats.push_back(c);
                found = (int)g_chats.size()-1;
            }
            g_cidx = found;
            g_chats[g_cidx].unread = 0;
            g_scroll = 0; g_input.clear();
            g_scr = Screen::Chat;
        }
        return;
    }
    if ((ch == KEY_BACKSPACE || ch == 127) && !g_newip.empty()) {
        g_newip.pop_back(); return;
    }
    if (ch >= 32 && ch < 127 && (int)g_newip.size() < 45)
        g_newip += (char)ch;
}

// ── CHAT ROOM ─────────────────────────────────────────────────────────────────
void TUI::draw_chat(int rows, int cols) {
    if (g_cidx < 0 || g_cidx >= (int)g_chats.size()) return;
    auto& chat = g_chats[g_cidx];

    int panel_h = rows - 4;   // message panel height
    int inp_y   = rows - 4;   // input box top row

    // message panel
    box_at(0, 0, panel_h, cols);

    std::string title = std::string(chat.up ? " ● " : " ○ ") +
                        chat.ip +
                        (chat.up ? "  [ connected ]" : "  [ offline ]");
    attron(COLOR_PAIR(CP_TITLE) | A_BOLD);
    mvaddstr(1, 2, title.c_str());
    attroff(COLOR_PAIR(CP_TITLE) | A_BOLD);

    sep_at(2, cols);

    // message viewport
    int msg_y0 = 3;
    int msg_vis = panel_h - 3 - 1;   // rows available between sep and bottom border
    auto& msgs  = chat.msgs;
    int   total = (int)msgs.size();

    // clamp scroll so we never scroll past the oldest message
    g_scroll = std::min(g_scroll, std::max(0, total - msg_vis));
    int start = std::max(0, total - msg_vis - g_scroll);
    int end   = total - g_scroll;

    if (msgs.empty()) {
        attron(COLOR_PAIR(CP_DIM));
        const char *ph = "no messages yet  ·  say hello!";
        mvaddstr(msg_y0 + msg_vis/2, (cols-(int)strlen(ph))/2, ph);
        attroff(COLOR_PAIR(CP_DIM));
    }

    for (int i = start; i < end; i++) {
        auto& m  = msgs[i];
        int   dy = msg_y0 + (i - start);
        if (dy >= msg_y0 + msg_vis) break;

        std::string ts = "[" + m.ts + "]";
        if (m.self) {
            // right-aligned: timestamp left | text right, "you" prefix coloured
            std::string prefix = "you : ";
            std::string body   = prefix + m.text;
            int px = cols - 2 - (int)body.size();
            if (px < 1) px = 1;

            attron(COLOR_PAIR(CP_SELF) | A_BOLD);
            mvaddstr(dy, px, prefix.c_str());
            attroff(COLOR_PAIR(CP_SELF) | A_BOLD);
            mvaddstr(dy, px+(int)prefix.size(), m.text.c_str());

            attron(COLOR_PAIR(CP_DIM));
            mvaddstr(dy, 2, ts.c_str());
            attroff(COLOR_PAIR(CP_DIM));
        } else {
            // left-aligned: coloured peer prefix, timestamp right
            std::string prefix = chat.ip + " : ";
            attron(COLOR_PAIR(CP_PEER) | A_BOLD);
            mvaddstr(dy, 2, prefix.c_str());
            attroff(COLOR_PAIR(CP_PEER) | A_BOLD);
            mvaddstr(dy, 2+(int)prefix.size(), m.text.c_str());

            attron(COLOR_PAIR(CP_DIM));
            mvaddstr(dy, cols-(int)ts.size()-2, ts.c_str());
            attroff(COLOR_PAIR(CP_DIM));
        }
    }

    // scroll indicator
    if (g_scroll > 0) {
        attron(COLOR_PAIR(CP_WARN));
        std::string sc = " ↑ " + std::to_string(g_scroll) + " older ";
        mvaddstr(msg_y0, cols-(int)sc.size()-1, sc.c_str());
        attroff(COLOR_PAIR(CP_WARN));
    }

    // input box
    box_at(inp_y, 0, 3, cols);
    std::string inp = g_input + "_";
    // truncate display if too long
    if ((int)inp.size() > cols-4) inp = inp.substr(inp.size()-(cols-4));
    attron(A_BOLD);
    mvaddstr(inp_y+1, 2, inp.c_str());
    attroff(A_BOLD);

    draw_status(rows, cols,
        "enter send  ·  ↑↓ scroll  ·  ESC back  │  " + chat.ip, hhmm());
}

void TUI::handle_chat(int ch) {
    if (g_cidx < 0) return;
    auto& chat = g_chats[g_cidx];
    switch (ch) {
        case 27:
            g_scr = Screen::Chats;
            g_input.clear();
            break;
        case '\n': case KEY_ENTER:
            if (!g_input.empty()) {
                chat.msgs.push_back({g_input, hhmm(), true});
                g_input.clear(); g_scroll = 0;
            }
            break;
        case KEY_UP:   g_scroll++; break;
        case KEY_DOWN: if (g_scroll > 0) g_scroll--; break;
        case KEY_BACKSPACE: case 127:
            if (!g_input.empty()) g_input.pop_back();
            break;
        default:
            if (ch >= 32 && ch < 127 && (int)g_input.size() < 200)
                g_input += (char)ch;
            break;
    }
}

// SETUP
void TUI::draw_setup(int rows, int cols) {
    box_at(0, 0, rows-1, cols, "SETUP");

    attron(COLOR_PAIR(CP_DIM));
    mvaddstr(1, 2, "my node settings");
    attroff(COLOR_PAIR(CP_DIM));

    sep_at(2, cols);

    struct Row { const char *key; const char *val; };
    static const Row cfg[] = {
        { "protocol     ",      "    kryppi/1.0"        },
        { "encryption key ",    "    XXXXXXXXXX"       },
        { "node id      ",      "    a3:f1:9c:2b:e7:4d" },
    };

    int y = 4;
    for (auto& r : cfg) {
        attron(COLOR_PAIR(CP_ACCENT));
        mvaddstr(y, 4, r.key);
        attroff(COLOR_PAIR(CP_ACCENT));
        attron(A_BOLD);
        mvaddstr(y, 4+14, r.val);
        attroff(A_BOLD);
        y += 2;
    }

    draw_status(rows, cols, "ESC back", hhmm());
}

void TUI::handle_setup(int ch) {
    if (ch == 27 || ch == 'q') g_scr = Screen::Menu;
}

void TUI::receive_message(const std::string& from_ip, const std::string& text) {
    // find or create the chat for this peer
    int idx = -1;
    for (int i = 0; i < (int)g_chats.size(); i++) {
        if (g_chats[i].ip == from_ip) { idx = i; break; }
    }
    if (idx < 0) {
        Chat c; c.ip = from_ip; c.up = true;
        g_chats.push_back(c);
        idx = (int)g_chats.size() - 1;
    }

    g_chats[idx].msgs.push_back({text, hhmm(), false});

    // only mark unread if this chat isn't the one currently open
    if (g_scr != Screen::Chat || g_cidx != idx)
        g_chats[idx].unread++;
}

// ── MAIN ──────────────────────────────────────────────────────────────────────
void TUI::run() {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    // startup animation
    run_splash(rows, cols);

    // main loop
    bool running = true;
    while (running) {
        getmaxyx(stdscr, rows, cols);
        erase();

        switch (g_scr) {
            case Screen::Menu:    draw_menu(rows, cols);     break;
            case Screen::Chats:   draw_chats(rows, cols);    break;
            case Screen::NewChat: draw_new_chat(rows, cols); break;
            case Screen::Chat:    draw_chat(rows, cols);     break;
            case Screen::Setup:   draw_setup(rows, cols);    break;
            default: break;
        }

        refresh();
        int ch = getch();

        switch (g_scr) {
            case Screen::Menu:    running = handle_menu(ch);   break;
            case Screen::Chats:   handle_chats(ch);             break;
            case Screen::NewChat: handle_new_chat(ch);          break;
            case Screen::Chat:    handle_chat(ch);              break;
            case Screen::Setup:   handle_setup(ch);             break;
            default: break;
        }
    }
}

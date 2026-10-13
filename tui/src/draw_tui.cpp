#include "draw_tui.h"
#include "../../udp-socket/inc/config.hpp"
#include "../../udp-socket/inc/kryp.hpp"
#include "../../udp-socket/inc/json.h"
#include <locale.h>
#include <ncurses.h>
#include <chrono>
#include <thread>
#include <ctime>
#include <algorithm>
#include <cstring>

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

// ── logo gradient ────────────────────────────────────────────────────────────
// init_colors() builds a 6-stop vertical fade for the splash logo. On a terminal
// that lets us redefine palette entries (can_change_color()) the stops are exact
// RGB; otherwise we approximate with the xterm-256 colour cube; otherwise the
// logo just uses the flat title colour.
static constexpr int CP_LOGO0 = 16;      // first of LOGO_H consecutive colour pairs
static bool  g_logo_grad      = false;   // true once gradient pairs are set up
static bool  g_logo_truecolor = false;   // true if we redefined palette entries
static int   g_logo_c0        = 0;       // first redefined colour index (truecolor path)
static short g_logo_saved[LOGO_H][3];    // original RGB of those entries, for restore

static inline int xterm_cube(int r, int g, int b) { return 16 + 36*r + 6*g + b; }

// map a 0..255 component to its nearest xterm-256 cube level index (0..5)
static inline int cube_level(int v) {
    return (v < 48) ? 0 : (v < 115) ? 1 : (v - 35) / 40;
}

// attribute for logo row i: gradient pair when available, else the flat title colour
static inline chtype logo_attr(int i) {
    return (g_logo_grad ? COLOR_PAIR(CP_LOGO0 + i) : COLOR_PAIR(CP_TITLE)) | A_BOLD;
}

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

Draw_TUI::Draw_TUI() {
    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    if (has_colors()) init_colors();
}

Draw_TUI::~Draw_TUI() {
    if (g_logo_truecolor)
        for (int i = 0; i < LOGO_H; ++i)
            init_color((short)(g_logo_c0 + i),
                       g_logo_saved[i][0], g_logo_saved[i][1], g_logo_saved[i][2]);
    endwin();
}

void Draw_TUI::init_colors() {
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

    // 6-stop vertical fade for the splash logo: light cyan #5fd7ff → deep blue #0a6fa4.
    if (COLORS >= 256) {
        const int top[3]    = { 0x5f, 0xd7, 0xff };
        const int bottom[3] = { 0x0a, 0x6f, 0xa4 };

        if (can_change_color()) {
            // exact RGB: redefine LOGO_H high palette entries, remembering the
            // originals so the destructor can put them back.
            g_logo_truecolor = true;
            g_logo_c0 = COLORS - LOGO_H;
            for (int i = 0; i < LOGO_H; ++i) {
                color_content((short)(g_logo_c0 + i),
                              &g_logo_saved[i][0], &g_logo_saved[i][1], &g_logo_saved[i][2]);
                double t = double(i) / (LOGO_H - 1);
                int R = (int)(top[0] + t * (bottom[0] - top[0]) + 0.5);
                int G = (int)(top[1] + t * (bottom[1] - top[1]) + 0.5);
                int B = (int)(top[2] + t * (bottom[2] - top[2]) + 0.5);
                init_color((short)(g_logo_c0 + i), (short)(R * 1000 / 255),
                                                   (short)(G * 1000 / 255),
                                                   (short)(B * 1000 / 255));
                init_pair(CP_LOGO0 + i, g_logo_c0 + i, -1);
            }
        } else {
            // fallback: nearest entries in the xterm-256 colour cube (6×6×6).
            for (int i = 0; i < LOGO_H; ++i) {
                double t = double(i) / (LOGO_H - 1);
                int R = (int)(top[0] + t * (bottom[0] - top[0]) + 0.5);
                int G = (int)(top[1] + t * (bottom[1] - top[1]) + 0.5);
                int B = (int)(top[2] + t * (bottom[2] - top[2]) + 0.5);
                init_pair(CP_LOGO0 + i, xterm_cube(cube_level(R), cube_level(G), cube_level(B)), -1);
            }
        }
        g_logo_grad = true;
    }
}

// draw a bordered box; title centred in top border when non-null
void Draw_TUI::box_at(int y, int x, int h, int w, const char *title) {
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
void Draw_TUI::sep_at(int y, int cols) {
    attron(COLOR_PAIR(CP_ACCENT));
    mvaddch(y, 0, ACS_LTEE);
    mvhline(y, 1, ACS_HLINE, cols-2);
    mvaddch(y, cols-1, ACS_RTEE);
    attroff(COLOR_PAIR(CP_ACCENT));
}

// bottom status bar (reversed green)
void Draw_TUI::draw_status(int rows, int cols,
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
void Draw_TUI::draw_logo(int y, int x) {
    for (int i = 0; i < LOGO_H; i++) {
        attron(logo_attr(i));
        mvaddstr(y+i, x, LOGO[i]);
        attroff(logo_attr(i));
    }
}

// ── SPLASH ANIMATION ─────────────────────────────────────────────────────────
void Draw_TUI::run_splash(int rows, int cols) {
    int lx = (cols - LOGO_VW) / 2;
    int ly = (rows - LOGO_H - 7) / 2;
    if (ly < 1) ly = 1;

    // logo lines appear one at a time
    for (int i = 0; i < LOGO_H; i++) {
        attron(logo_attr(i));
        mvaddstr(ly+i, lx, LOGO[i]);
        attroff(logo_attr(i));
        refresh(); msleep(65);
    }

    refresh(); msleep(1020);
    g_scr = Screen::Menu;
}

// MAIN MENU
void Draw_TUI::draw_menu(int rows, int cols) {
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
    {
        std::lock_guard<std::mutex> lock(chats_mtx);
        for (auto& c : g_chats) unread += c.unread;
    }

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

bool Draw_TUI::handle_menu(int ch) {
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
void Draw_TUI::draw_chats(int rows, int cols) {
    std::lock_guard<std::mutex> lock(chats_mtx);
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

void Draw_TUI::handle_chats(int ch) {
    std::lock_guard<std::mutex> lock(chats_mtx);
    switch (ch) {
        case KEY_UP:   if (g_chat_sel > 0) g_chat_sel--; break;
        case KEY_DOWN: if (g_chat_sel < (int)g_chats.size()-1) g_chat_sel++; break;
        case '\n': case KEY_ENTER:
            if (!g_chats.empty()) {
                g_cidx = g_chat_sel;
                g_scroll = 0; g_input.clear();
                if (g_chats[g_cidx].unlocked) {
                    g_chats[g_cidx].unread = 0;
                    g_scr = Screen::Chat;
                } else {
                    g_unlock_input.clear();
                    g_scr = Screen::ChatUnlock;
                }
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
void Draw_TUI::draw_new_chat(int rows, int cols) {
    int dw=54, dh=13;
    int dy=(rows-dh)/2, dx=(cols-dw)/2;

    box_at(dy, dx, dh, dw, "NEW CONNECTION");


    mvaddstr(dy+2, dx+3, "peer IP / hostname:");

    // inner input field box (IP)
    box_at(dy+3, dx+2, 3, dw-4);
    attron(A_BOLD);
    std::string disp = g_newip + (g_newchat_field == 0 ? "_" : "");
    mvaddstr(dy+4, dx+3, disp.c_str());
    attroff(A_BOLD);

    // Set encryption key
    mvaddstr(dy+6, dx+3, "Encryption Key");

    // input field box (key)
    box_at(dy+7, dx+2, 3, dw-4);
    attron(A_BOLD);
    disp = g_newkey + (g_newchat_field == 1 ? "_" : "");
    mvaddstr(dy+8, dx+3, disp.c_str());
    attroff(A_BOLD);


    attron(COLOR_PAIR(CP_DIM));
    mvaddstr(dy+10, dx+3, "enter to connect  ·  ESC to cancel");
    attroff(COLOR_PAIR(CP_DIM));

    draw_status(rows, cols,
        "tab switch field  ·  enter connect  ·  ESC cancel", hhmm());
}

void Draw_TUI::handle_new_chat(int ch) {
    if (ch == 27) { g_scr = Screen::Chats; return; }

    if (ch == '\t' || ch == KEY_BTAB) {
        g_newchat_field = 1 - g_newchat_field;
        return;
    }

    if (ch == '\n' || ch == KEY_ENTER) {
        if (!g_newip.empty()) {
            int found = -1;
            for (int i=0; i<(int)g_chats.size(); i++)
                if (g_chats[i].ip == g_newip) { found=i; break; }
            if (found < 0) {
                Chat c; c.ip = g_newip; c.up = true;
                c.key = g_newkey.empty() ? CRYPTO_KEY : g_newkey;
                g_chats.push_back(c);
                found = (int)g_chats.size()-1;
            }
            g_cidx = found;
            g_chats[g_cidx].unread = 0;
            g_chats[g_cidx].unlocked = true;
            g_scroll = 0; g_input.clear();
            g_newkey.clear();
            g_newchat_field = 0;
            g_scr = Screen::Chat;
        }
        return;
    }

    std::string& buf = (g_newchat_field == 0) ? g_newip : g_newkey;
    if ((ch == KEY_BACKSPACE || ch == 127) && !buf.empty()) {
        buf.pop_back(); return;
    }
    if (ch >= 32 && ch < 127 && (int)buf.size() < 45)
        buf += (char)ch;
}

// ── CHAT ROOM ─────────────────────────────────────────────────────────────────
void Draw_TUI::draw_chat(int rows, int cols) {
    std::lock_guard<std::mutex> lock(chats_mtx);
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

    // Spoofer-advarsel hvis for mange uventede ACKs
    int warn_shown = (chat.ack_excess >= 3) ? 1 : 0;
    if (warn_shown) {
        attron(COLOR_PAIR(CP_WARN) | A_BOLD);
        std::string w = " \xe2\x9a\xa0  ADVARSEL: for mange ACKs \xe2\x80\x94 mulig spoofer ("
                      + std::to_string(chat.ack_excess) + " overskydende)";
        mvaddstr(2, 1, w.substr(0, cols-2).c_str());
        attroff(COLOR_PAIR(CP_WARN) | A_BOLD);
        sep_at(3, cols);
    } else {
        sep_at(2, cols);
    }

    // message viewport
    int msg_y0 = 3 + warn_shown;
    int msg_vis = panel_h - 3 - 1 - warn_shown;
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

            // ACK-indikator: ✓ (ACK_RECV) eller ✓✓ (ACK_DECRYPTED)
            if (m.ack_recv || m.ack_decrypted) {
                // UTF-8 checkmarks: ✓ = \xe2\x9c\x93, ✓✓ = to af dem
                const char* mark = m.ack_decrypted
                    ? " \xe2\x9c\x93\xe2\x9c\x93"
                    : " \xe2\x9c\x93";
                int ack_col = 2 + (int)ts.size();
                if (m.ack_decrypted) attron(COLOR_PAIR(CP_SELF) | A_BOLD);
                else                 attron(COLOR_PAIR(CP_WARN));
                mvaddstr(dy, ack_col, mark);
                if (m.ack_decrypted) attroff(COLOR_PAIR(CP_SELF) | A_BOLD);
                else                 attroff(COLOR_PAIR(CP_WARN));
            }
        } else {
            // left-aligned: coloured peer prefix, timestamp right.
            // Peer payloads are stored as ciphertext — decrypt on the fly
            // using the key the user typed at unlock.
            std::string plain = jsonwriteread::jsonread(
                chat.ip, kryp::xor_cryption(m.text, chat.key));
            std::string prefix = chat.ip + " : ";
            attron(COLOR_PAIR(CP_PEER) | A_BOLD);
            mvaddstr(dy, 2, prefix.c_str());
            attroff(COLOR_PAIR(CP_PEER) | A_BOLD);
            mvaddstr(dy, 2+(int)prefix.size(), plain.c_str());

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
        // truncate display if too long
    if ((int)g_input.size() > cols-4) g_input = g_input.substr(g_input.size()-(cols-4));
    attron(A_BOLD);
    mvaddstr(inp_y+1, 2, g_input.c_str());
    attroff(A_BOLD);

    draw_status(rows, cols,
        "enter send  ·  ↑↓ scroll  ·  ESC back  │  " + chat.ip, hhmm());
}

void Draw_TUI::handle_chat(int ch) {
    std::lock_guard<std::mutex> lock(chats_mtx);
    if (g_cidx < 0) return;
    auto& chat = g_chats[g_cidx];
    switch (ch) {
        case 27:
            g_scr = Screen::Chats;
            g_input.clear();
            break;
        case '\n': case KEY_ENTER:
            if (!g_input.empty()) {
                Msg m{g_input, hhmm(), true};
                if (on_send) m.msgId = on_send(chat.ip, g_input, chat.key);
                chat.msgs.push_back(std::move(m));
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

// ── UNLOCK PROMPT ─────────────────────────────────────────────────────────────
void Draw_TUI::draw_chat_unlock(int rows, int cols) {
    std::lock_guard<std::mutex> lock(chats_mtx);
    if (g_cidx < 0 || g_cidx >= (int)g_chats.size()) return;
    auto& chat = g_chats[g_cidx];

    int dw=54, dh=10;
    int dy=(rows-dh)/2, dx=(cols-dw)/2;

    box_at(dy, dx, dh, dw, "UNLOCK CHAT");

    std::string label = "enter key for " + chat.ip + ":";
    mvaddstr(dy+2, dx+3, label.c_str());

    box_at(dy+3, dx+2, 3, dw-4);
    attron(A_BOLD);
    std::string masked(g_unlock_input.size(), '*');
    std::string disp = masked + "_";
    mvaddstr(dy+4, dx+3, disp.c_str());
    attroff(A_BOLD);

    if (g_unlock_err) {
        attron(COLOR_PAIR(CP_WARN) | A_BOLD);
        mvaddstr(dy+6, dx+3, "wrong key — try again");
        attroff(COLOR_PAIR(CP_WARN) | A_BOLD);
    } else {
        attron(COLOR_PAIR(CP_DIM));
        mvaddstr(dy+6, dx+3, "enter to unlock  ·  ESC to cancel");
        attroff(COLOR_PAIR(CP_DIM));
    }

    draw_status(rows, cols,
        "type encryption key  ·  enter unlock  ·  ESC cancel", hhmm());
}

// Verify the candidate key by trying to decrypt+parse the most recent peer
// message. Returns true if a peer message decodes to a non-empty plaintext,
// or if there are no peer messages yet (no way to verify — accept blindly).
static bool verify_key(const Chat& chat, const std::string& candidate) {
    for (auto it = chat.msgs.rbegin(); it != chat.msgs.rend(); ++it) {
        if (it->self) continue;
        std::string plain = jsonwriteread::jsonread(
            chat.ip, kryp::xor_cryption(it->text, candidate));
        return !plain.empty();
    }
    return true;
}

void Draw_TUI::handle_chat_unlock(int ch) {
    std::lock_guard<std::mutex> lock(chats_mtx);
    if (g_cidx < 0 || g_cidx >= (int)g_chats.size()) {
        g_unlock_input.clear();
        g_unlock_err = false;
        g_scr = Screen::Chats;
        return;
    }

    if (ch == 27) {
        g_unlock_input.clear();
        g_unlock_err = false;
        g_scr = Screen::Chats;
        return;
    }

    if (ch == '\n' || ch == KEY_ENTER) {
        auto& chat = g_chats[g_cidx];
        if (g_unlock_input.empty()) return;
        if (verify_key(chat, g_unlock_input)) {
            chat.key = g_unlock_input;
            chat.unlocked = true;
            chat.unread = 0;
            g_scroll = 0; g_input.clear();
            g_unlock_input.clear();
            g_unlock_err = false;
            g_scr = Screen::Chat;
        } else {
            g_unlock_input.clear();
            g_unlock_err = true;
        }
        return;
    }

    if ((ch == KEY_BACKSPACE || ch == 127) && !g_unlock_input.empty()) {
        g_unlock_input.pop_back();
        g_unlock_err = false;
        return;
    }
    if (ch >= 32 && ch < 127 && (int)g_unlock_input.size() < 45) {
        g_unlock_input += (char)ch;
        g_unlock_err = false;
    }
}

// SETUP
void Draw_TUI::draw_setup(int rows, int cols) {
    box_at(0, 0, rows-1, cols, "SETUP");

    attron(COLOR_PAIR(CP_DIM));
    mvaddstr(1, 2, "my node settings");
    attroff(COLOR_PAIR(CP_DIM));

    sep_at(2, cols);

    struct Row { const char *key; std::string val; };
    std::vector<Row> cfg = {
        { "protocol     ",   "kryppi/2.0"                    },
        { "encryption",      "XXXXXXXXXX"                    },
        { "mesh ip      ",   g_myip.empty() ? "ikke sat" : g_myip },
    };

    int y = 4;
    for (auto& r : cfg) {
        attron(COLOR_PAIR(CP_ACCENT));
        mvaddstr(y, 4, r.key);
        attroff(COLOR_PAIR(CP_ACCENT));
        attron(A_BOLD);
        mvaddstr(y, 4+14, r.val.c_str());
        attroff(A_BOLD);
        y += 2;
    }

    draw_status(rows, cols, "ESC back", hhmm());
}

void Draw_TUI::handle_setup(int ch) {
    if (ch == 27 || ch == 'q') g_scr = Screen::Menu;
}




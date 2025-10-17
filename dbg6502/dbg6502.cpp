#include <cstdarg>
#include <cstring>
#include <ctype.h>
#include <locale.h>
#include <ncurses.h>
#include <stdint.h>
#include <string>
#include <vector>

#include "../mos6502.h"

uint8_t ram[65536] = { 0x4C, 0, 0 } ;
uint8_t write_protect[65536 / 8] = { 0 };
uint8_t breakpoint[65536 / 8] = { 0 };
mos6502 *cpu = NULL;

class NcWin {
   public:
      NcWin(int x, int y, int w, int h) : win(newwin(h, w, y, x)) {
         if (!win) return;
         box(win, 0, 0);
         wrefresh(win);
      }
      virtual ~NcWin() { if (win) delwin(win); }

      void xyprintf(int x, int y, const char *fmt, ...) {
         if (!win) return;
         va_list args; va_start(args, fmt);
         wmove(win, y, x);
         vw_printw(win, fmt, args);
         va_end(args);
         wrefresh(win);
      }

   protected:
      WINDOW *win{};
};

class RegisterWin : public NcWin {
   public:
      RegisterWin(int x, int y, int w, int h)
         : NcWin(x, y, w, h) {
            update();
         }

      void update() {
         static char srtext[] =  { 'N', 'V', '-', 'B', 'D', 'I', 'Z', 'C', 0 };
         uint8_t a = cpu->GetA();
         uint8_t x = cpu->GetX();
         uint8_t y = cpu->GetY();
         uint8_t pc = cpu->GetPC();
         uint8_t sr = cpu->GetP();
         uint8_t sp = cpu->GetS();
         for (int i = 0; i < 8; i++) {
            if (sr & (1 << (7 - i))) {
               srtext[i] = toupper(srtext[i]);
            }
            else {
               srtext[i] = tolower(srtext[i]);
            }
         }
         xyprintf(1, 1, "PC:%04x SP:%02x A:%02x X:%02x Y:%02x SR:%s", pc, sp, a, x, y, srtext);
      }

};

class MemoryWin : public NcWin {
   public:
      MemoryWin(int x, int y, int w, int h)
         : NcWin(x, y, w, h), address(0) {
            set_address(0);
         }

      void set_address(uint16_t addr) {
         int h, w; getmaxyx(win, h, w);
         address = addr;
         for (int i = 0; i < h - 2; i++) {
            xyprintf(1, i + 1, "%04x: ", addr + 16 * i);
            for (int j = 0; j < 16; j++) {
               xyprintf(1 + 7 + j * 3, i + 1, "%02x",
                  ram[addr + 16 * i + j]);
            }
         }
      }

   private:
      uint16_t address;
};

// A simple terminal-like window with scrolling + input history.
class TerminalWin : public NcWin {
public:
    TerminalWin(int x, int y, int w, int h) : NcWin(x, y, w, h) {
        int H, W; getmaxyx(win, H, W);
        inner = derwin(win, H - 2, W - 2, 1, 1);   // safe drawing region
        scrollok(inner, TRUE);
        keypad(inner, TRUE);
        wsetscrreg(inner, 0, (H - 2) - 1);
        wmove(inner, 0, 0);
        wrefresh(win);
        wrefresh(inner);
    }

    ~TerminalWin() override {
        if (inner) { delwin(inner); inner = nullptr; }
    }

    void printf(const char* fmt, ...) {
        va_list args; va_start(args, fmt);
        vw_printw(inner, fmt, args);
        va_end(args);
        waddch(inner, '\n');
        wrefresh(inner);
    }

    // Reads a line with prompt; echoes input; supports backspace and ↑/↓ history (64 lines).
    // NOTE: Caller should provide a buffer large enough for expected input.
    void getline(const char* prompt, char* buffer) {
        // Ensure cursor visible (blinking is terminal-dependent)
        curs_set(1);

        // Move to next line if not at column 0
        int cy, cx; getyx(inner, cy, cx);
        if (cx != 0) { waddch(inner, '\n'); getyx(inner, cy, cx); }

        // Write prompt and start fresh input
        std::string current;
        int H, W; getmaxyx(inner, H, W);
        draw_line(prompt, current);

        // history navigation: index -1 means editing current line
        int hist_index = -1; // 0 = oldest, size()-1 = newest

        while (true) {
            int ch = wgetch(inner);
            if (ch == '\n' || ch == '\r') {
                // finalize line
                waddch(inner, '\n');
                // store into history (skip empty duplicates at tail)
                if (!current.empty()) {
                    if (history.empty() || history.back() != current) {
                        history.push_back(current);
                        if (history.size() > kMaxHistory) history.erase(history.begin());
                    }
                }
                // copy to caller buffer
                std::strcpy(buffer, current.c_str());
                wrefresh(inner);
                curs_set(0);
                return;
            }

            switch (ch) {
                case KEY_BACKSPACE:
                case 127:
                case 8:
                    if (!current.empty()) {
                        current.pop_back();
                        draw_line(prompt, current);
                    }
                    break;

                case KEY_UP:
                    if (!history.empty()) {
                        if (hist_index < 0) hist_index = static_cast<int>(history.size()) - 1;
                        else if (hist_index > 0) hist_index--;
                        current = history[hist_index];
                        draw_line(prompt, current);
                    }
                    break;

                case KEY_DOWN:
                    if (!history.empty() && hist_index >= 0) {
                        if (hist_index < static_cast<int>(history.size()) - 1) {
                            hist_index++;
                            current = history[hist_index];
                        } else {
                            hist_index = -1;
                            current.clear();
                        }
                        draw_line(prompt, current);
                    }
                    break;

                case KEY_LEFT:
                case KEY_RIGHT:
                case KEY_HOME:
                case KEY_END:
                    // Keep it simple: no in-line editing; ignore navigation keys.
                    // (Could be added later by tracking an insertion cursor.)
                    break;

                default:
                    if (is_printable(ch)) {
                        // Prevent drawing beyond the line width; wrap naturally by newline if needed.
                        // We’ll clamp visual width to W-1 (prompt + text).
                        int prompt_len = visual_len(prompt);
                        if (prompt_len + (int)current.size() < W - 1) {
                            current.push_back(static_cast<char>(ch));
                            draw_line(prompt, current);
                        } else {
                            // If full, behave like a bell
                            beep();
                        }
                    }
                    break;
            }
        }
    }

private:
    WINDOW* inner{};
    static constexpr size_t kMaxHistory = 64;
    std::vector<std::string> history;

    static bool is_printable(int ch) {
        return ch >= 32 && ch <= 126;
    }

    static int visual_len(const char* s) {
        // ASCII only here; if you need UTF-8 width, use wcwidth/mbstowcs.
        int n = 0; while (s && *s) { n++; s++; } return n;
    }

    void draw_line(const char* prompt, const std::string& text) {
        int y, x; getyx(inner, y, x);
        // Move to start of current line (create one if we’re at end)
        // If we’re beyond last line, inner will scroll automatically.
        wmove(inner, y, 0);
        wclrtoeol(inner);
        waddstr(inner, prompt);
        waddnstr(inner, text.c_str(), (int)text.size());
        // Place cursor at end of input
        getyx(inner, y, x);
        wrefresh(inner);
    }
};

int main() {
   setlocale(LC_ALL, "");     // enable line/box chars in UTF-8 terminals
   initscr();
   noecho(); curs_set(0);
   refresh();                 // ensure base screen is pushed once

   int H, W; getmaxyx(stdscr, H, W);
   TerminalWin term(0, H - 8, W, 8);    // guaranteed on-screen
   term.printf("Hello, ncurses!");

   MemoryWin *memwin = new MemoryWin(W - 57, H - 18 - 8, 57, 18);

   cpu = new mos6502(NULL, NULL, NULL);

   RegisterWin *regwin = new RegisterWin(W - 57, H - 18 - 8 - 3, 57, 3);

    term.printf("Welcome. Type lines; use Up/Down to browse history. Enter accepts.");
    char buf[1024];

    for (int i = 0; i < 10; ++i) {
        term.getline(">>> ", buf);
        term.printf("You typed: %s", buf);
    }

   mvprintw(H-1, 2, "Press any key...");
   refresh();
   getch();
   endwin();

   return 0;
}

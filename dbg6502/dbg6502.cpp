#include <locale.h>
#include <ncurses.h>
#include <cstdarg>
#include <stdint.h>
#include <ctype.h>

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
      ~NcWin() { if (win) delwin(win); }

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

class TerminalWin : public NcWin {
   public:
      TerminalWin(int x, int y, int w, int h) : NcWin(x, y, w, h) {
         // Create an inner window that excludes the border (1px margin all around)
         int H, W; getmaxyx(win, H, W);

         inner = derwin(win, H - 2, W - 2, 1, 1);
         scrollok(inner, TRUE);
         // Optional: define explicit scroll region matching inner’s full height
         wsetscrreg(inner, 0, (H - 2) - 1);
         wmove(inner, 0, 0);
         wrefresh(win);
         wrefresh(inner);
      }

      ~TerminalWin() {
         if (inner) { delwin(inner); inner = nullptr; }
      }

      // Print a formatted line; first at top, subsequent below; scrolls when needed.
      void printf(const char* fmt, ...) {
         va_list args;
         va_start(args, fmt);
         vw_printw(inner, fmt, args);
         va_end(args);
         // Force a new line to advance; scrollok() will scroll when needed.
         waddch(inner, '\n');
         // Refresh inner so content appears without redrawing the border.
         wrefresh(inner);
      }

   private:
      WINDOW* inner{};
};

int main() {
   setlocale(LC_ALL, "");     // enable line/box chars in UTF-8 terminals
   initscr();
   noecho(); curs_set(0);
   refresh();                 // ensure base screen is pushed once

   int H, W; getmaxyx(stdscr, H, W);
   TerminalWin box1(0, H - 8, W, 8);    // guaranteed on-screen
   box1.printf("Hello, ncurses!");

   MemoryWin *memwin = new MemoryWin(W - 57, H - 18 - 8, 57, 18);

   cpu = new mos6502(NULL, NULL, NULL);

   RegisterWin *regwin = new RegisterWin(W - 57, H - 18 - 8 - 3, 57, 3);

   mvprintw(H-1, 2, "Press any key...");
   refresh();
   getch();
   endwin();

   return 0;
}

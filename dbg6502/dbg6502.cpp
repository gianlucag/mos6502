#include <algorithm>
#include <cstdarg>
#include <cstring>
#include <ctype.h>
#include <dirent.h>
#include <locale.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdint.h>
#include <string>
#include <sys/stat.h>
#include <vector>

#include "../mos6502.h"

uint8_t verbosity = 0;

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
         uint16_t pc = cpu->GetPC();
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

         addr &= 0xFFF0;
         addr -= 32;

         for (int i = 0; i < h - 2; i++) {
            xyprintf(1, i + 1, "%04x: ", (uint16_t)(addr + 16 * i));
            for (int j = 0; j < 16; j++) {
               if ((uint16_t) (addr + 16 * i + j) == address) {
                  wattron(win, A_REVERSE);
               }
               xyprintf(1 + 7 + j * 3, i + 1, "%02x",
                     ram[(uint16_t)(addr + 16 * i + j)]);
               if ((uint16_t) (addr + 16 * i + j) == address) {
                  wattroff(win, A_REVERSE);
               }
            }
         }
      }

      void update(void) {
         set_address(address); // minor kludge, but gets the job done
      }

   private:
      uint16_t address;
};

// --- TerminalWin: scrolling printf + getline with history and TAB completion ---
class TerminalWin : public NcWin {
   public:
      TerminalWin(int x, int y, int w, int h) : NcWin(x, y, w, h) {
         int H, W; getmaxyx(win, H, W);
         inner = derwin(win, H - 2, W - 2, 1, 1);   // draw inside border
         scrollok(inner, TRUE);
         keypad(inner, TRUE);
         wsetscrreg(inner, 0, (H - 2) - 1);
         wmove(inner, 0, 0);
         wrefresh(win);
         wrefresh(inner);
      }

      ~TerminalWin() override { if (inner) { delwin(inner); inner = nullptr; } }

      void printf(const char* fmt, ...) {
         va_list args; va_start(args, fmt);
         vw_printw(inner, fmt, args);
         va_end(args);
         waddch(inner, '\n');
         wrefresh(inner);
      }

      // getline: prompt + echo + backspace + 64-entry history + DOS-style TAB cycling
      void getline(const char* prompt, char* buffer) {
         curs_set(1);

         // Start new line if cursor not at column 0
         int cy, cx; getyx(inner, cy, cx);
         if (cx != 0) { waddch(inner, '\n'); }

         std::string current;
         draw_line(prompt, current);

         int hist_index = -1;

         // Completion session state
         bool comp_active = false;           // true after first TAB until any edit
         int  comp_word_start = 0;
         std::string comp_base;
         std::vector<std::string> comp_matches;
         int  comp_index = 0;

         auto reset_completion = [&](){
            comp_active = false;
            comp_matches.clear();
            comp_index = 0;
         };

         while (true) {
            int ch = wgetch(inner);

            // ENTER: accept input
            if (ch == '\n' || ch == '\r') {
               waddch(inner, '\n');
               if (!current.empty()) {
                  if (history.empty() || history.back() != current) {
                     history.push_back(current);
                     if (history.size() > kMaxHistory) history.erase(history.begin());
                  }
               }
               std::strcpy(buffer, current.c_str());
               wrefresh(inner);
               curs_set(0);
               return;
            }

            // TAB: compute or cycle filename completions for last token
            if (ch == '\t') {
               prepare_completion_state(current,
                     comp_active,
                     comp_word_start,
                     comp_base,
                     comp_matches,
                     comp_index);
               if (!comp_matches.empty()) {
                  // Replace token with the candidate at comp_index, then advance index
                  current.replace(comp_word_start, current.size() - comp_word_start,
                        comp_matches[comp_index]);
                  draw_line(prompt, current);
                  comp_index = (comp_index + 1) % static_cast<int>(comp_matches.size());
               } else {
                  beep();
               }
               continue;
            }

            // History navigation
            if (ch == KEY_UP) {
               if (!history.empty()) {
                  if (hist_index < 0) hist_index = static_cast<int>(history.size()) - 1;
                  else if (hist_index > 0) hist_index--;
                  current = history[hist_index];
                  draw_line(prompt, current);
               }
               reset_completion();
               continue;
            }
            if (ch == KEY_DOWN) {
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
               reset_completion();
               continue;
            }

            // Backspace
            if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
               if (!current.empty()) {
                  current.pop_back();
                  draw_line(prompt, current);
               } else {
                  beep();
               }
               reset_completion();
               continue;
            }

            // Ignore lateral nav for now (keeps logic simple)
            if (ch == KEY_LEFT || ch == KEY_RIGHT || ch == KEY_HOME || ch == KEY_END) {
               reset_completion();
               continue;
            }

            // Printable ASCII
            if (is_printable(ch)) {
               int H, W; getmaxyx(inner, H, W);
               int prompt_len = (int)std::strlen(prompt);
               if (prompt_len + (int)current.size() < W - 1) {
                  current.push_back((char)ch);
                  draw_line(prompt, current);
               } else {
                  beep();
               }
               reset_completion();
               continue;
            }

            // Unknown key: ignore, but end any completion session
            reset_completion();
         }
      }

   private:
      WINDOW* inner{};
      static constexpr size_t kMaxHistory = 64;
      std::vector<std::string> history;

      static bool is_printable(int ch) { return ch >= 32 && ch <= 126; }

      void draw_line(const char* prompt, const std::string& text) {
         int y, x; getyx(inner, y, x);
         wmove(inner, y, 0);
         wclrtoeol(inner);
         waddstr(inner, prompt);
         waddnstr(inner, text.c_str(), (int)text.size());
         wrefresh(inner);
      }

      // -------- Completion core --------

      // Build candidates once per TAB session; do NOT rebuild while cycling.
      void prepare_completion_state(const std::string& current,
            bool& comp_active,
            int& comp_word_start,
            std::string& comp_base,
            std::vector<std::string>& comp_matches,
            int& comp_index)
      {
         if (comp_active) return; // already cycling on this input

         comp_matches.clear();
         comp_index = 0;

         // Last space-delimited token
         comp_word_start = (int)current.find_last_of(' ');
         comp_word_start = (comp_word_start == (int)std::string::npos) ? 0 : comp_word_start + 1;
         comp_base = current.substr(comp_word_start);

         // Split into dir + base; if token ends with slash, browse that dir (base="")
         std::string dirpath, base;
         bool ends_with_slash = !comp_base.empty() &&
            (comp_base.back() == '/' || comp_base.back() == '\\');
         split_path(comp_base, dirpath, base);
         if (ends_with_slash) base.clear();

         // List matches; when browsing a directory after trailing slash, list all entries
         comp_matches = list_matches(dirpath, base, /*browse_all=*/ends_with_slash);

         // Convert to replacements relative to original token
         for (std::string& m : comp_matches) m = join_path(dirpath, m);
         std::sort(comp_matches.begin(), comp_matches.end());

         comp_active = true; // start cycling
      }

      static void split_path(const std::string& token, std::string& dir, std::string& base) {
         auto pos = token.find_last_of("/\\");
         if (pos == std::string::npos) {
            dir = ".";
            base = token;
         } else {
            dir  = token.substr(0, pos + 1);
            base = token.substr(pos + 1);
            if (dir.empty()) dir = "/";
         }
      }

      static std::string join_path(const std::string& dir, const std::string& name) {
         if (dir == "." || dir.empty()) return name;
         char last = dir.back();
         if (last == '/' || last == '\\') return dir + name;
         return dir + "/" + name;
      }

      static bool is_dir(const std::string& path) {
         struct stat st{};
         if (stat(path.c_str(), &st) == 0) return S_ISDIR(st.st_mode);
         return false;
      }

      // List directory entries starting with base; if browse_all, list everything.
      // Skip "." and ".." to avoid creeping with "../<TAB>".
      static std::vector<std::string> list_matches(const std::string& dir,
            const std::string& base,
            bool browse_all)
      {
         std::vector<std::string> out;
         DIR* d = opendir(dir.c_str());
         if (!d) return out;

         bool show_hidden = browse_all ? true : (!base.empty() && base[0] == '.');

         for (dirent* ent; (ent = readdir(d)); ) {
            std::string name = ent->d_name;
            if (name == "." || name == "..") continue;
            if (!show_hidden && !name.empty() && name[0] == '.') continue;

            if (browse_all || starts_with(name, base)) {
               std::string full = (dir == ".") ? name : join_path(dir, name);
               if (is_dir((dir == ".") ? name : full)) name += "/";
               out.push_back(name);
            }
         }
         closedir(d);
         return out;
      }

      static bool starts_with(const std::string& s, const std::string& prefix) {
         if (prefix.size() > s.size()) return false;
         return std::equal(prefix.begin(), prefix.end(), s.begin());
      }
};

void setup_ncurses(void) {
   setlocale(LC_ALL, "");     // enable line/box chars in UTF-8 terminals
   initscr();
   noecho();
   keypad(stdscr, TRUE);
   curs_set(0);
   refresh();                 // ensure base screen is pushed once
}

void teardown_ncurses(void) {
   refresh();
   endwin();
}

bool done = false;
bool bus_error = false;
bool breakpoint_hit = false;
MemoryWin *memwin = NULL;
TerminalWin *termwin = NULL;
RegisterWin *regwin = NULL;
TerminalWin *displaywin = NULL;

void bus_write(uint16_t addr, uint8_t data) {
   if (verbosity >= 3) {
      displaywin->printf("write %04x %02x", addr, data);
   }
   if (verbosity >= 2) {
      memwin->set_address(addr);
   }
   if (write_protect[addr >> 3] & (1 << (addr & 0x7))) {
      bus_error = true;
      displaywin->printf("BUS ERROR");
   }
   else {
      ram[addr] = data;
   }
}

uint8_t bus_read(uint16_t addr) {
   if (verbosity >= 3) {
      displaywin->printf("read %04x %02x", addr, ram[addr]);
   }
   if (verbosity >= 2) {
      memwin->set_address(addr);
   }
   return ram[addr];
}

void tick(mos6502 *) {
}

typedef void (*handler)(char *p);

void do_quit(char *) {
   done = 1;
}

void do_reset(char *) {
   cpu->Reset();
}

int parsenum(char *p) {
   switch (*p) {
      case 'x':
      case '$':
         return strtoul(p + 1, NULL, 16);
      case 'o':
      case '@':
         return strtoul(p + 1, NULL, 8);
      case 'b':
      case '%':
         return strtoul(p + 1, NULL, 2);
      default:
         return strtoul(p, NULL, 0); // intentional, to allow 0x hex, 0b binary, and 0 octal
   }
}

void do_a(char *p) {
   p = strchr(p, '=');
   if (p) {
      p++;
      cpu->SetA(parsenum(p));
   }
   else {
      termwin->printf("parse error");
   }
}

void do_x(char *p) {
   p = strchr(p, '=');
   if (p) {
      p++;
      cpu->SetX(parsenum(p));
   }
   else {
      termwin->printf("parse error");
   }
}

void do_y(char *p) {
   p = strchr(p, '=');
   if (p) {
      p++;
      cpu->SetY(parsenum(p));
   }
   else {
      termwin->printf("parse error");
   }
}

void do_pc(char *p) {
   p = strchr(p, '=');
   if (p) {
      p++;
      cpu->SetPC(parsenum(p));
   }
   else {
      termwin->printf("parse error");
   }
}

void do_sp(char *p) {
   p = strchr(p, '=');
   if (p) {
      p++;
      cpu->SetS(parsenum(p));
   }
   else {
      termwin->printf("parse error");
   }
}

uint8_t flagmask(char *p) {
   static const char *match1 = "nv-bdizc";
   static const char *match2 = "NV-BDIZC";
   uint8_t ret = 0;

   while (*p) {
      char *q = strchr((char *)match1, *p);
      if (q) {
         ret |= 1 << (7 - (q - match1));
      }
      q = strchr((char *)match2, *p);
      if (q) {
         ret |= 1 << (7 - (q - match1));
      }
      p++;
   }

   return ret;
}

uint8_t flagval(char *p) {
   static const char *match = "NV-BDIZC";
   uint8_t ret = 0;

   while (*p) {
      const char *q = strchr((char *)match, *p);
      if (q) {
         ret |= 1 << (7 - (q - match));
      }
      p++;
   }

   return ret;
}

void do_sr(char *p) {
   static const char *match = "nvbdizcNVBDIZC";

   p = strchr(p, '=');
   if (p) {
      p++;
      if (strchr(match, *p)) {
         cpu->SetP((cpu->GetP() & ~flagmask(p)) | flagval(p));
      }
      else {
         cpu->SetP(parsenum(p));
      }
   }
   else {
      termwin->printf("parse error");
   }
}

void do_wpset(char *p) {
   int begin, end;
   p = strchr(p, ' ');
   if (p) {
      p++;
      char *q = strchr(p, '-');
      if (q) {
         *q = 0;
         q++;
         begin = parsenum(p);
         end   = parsenum(q);

         displaywin->printf("wp set %04x to %04x", begin, end);
         while (begin <= end && (begin & 0x7)) {
            write_protect[begin >> 3] |= (1 << (begin & 0x7));
            begin++;
         }
         while (!(begin & 0x7) && (begin + 8) <= end) {
            write_protect[begin >> 3] = 0xFF;
            begin += 8;
         }
         while (begin <= end && (begin & 0x7)) {
            write_protect[begin >> 3] |= (1 << (begin & 0x7));
            begin++;
         }
      }
      else {
         displaywin->printf("wp set %04x", begin);
         begin = parsenum(p);
         write_protect[begin >> 3] |= (1 << (begin & 0x7));
      }
   }
   else {
      termwin->printf("parse error");
   }
}

void do_wpclr(char *p) {
   int begin, end;
   p = strchr(p, ' ');
   if (p) {
      p++;
      char *q = strchr(p, '-');
      if (q) {
         *q = 0;
         q++;
         begin = parsenum(p);
         end   = parsenum(q);

         displaywin->printf("wp clr %04x to %04x", begin, end);
         while (begin <= end && (begin & 0x7)) {
            write_protect[begin >> 3] &= ~(1 << (begin & 0x7));
            begin++;
         }
         while (!(begin & 0x7) && (begin + 8) <= end) {
            write_protect[begin >> 3] = 0x00;
            begin += 8;
         }
         while (begin <= end && (begin & 0x7)) {
            write_protect[begin >> 3] &= ~(1 << (begin & 0x7));
            begin++;
         }
      }
      else {
         begin = parsenum(p);
         displaywin->printf("wp clr %04x", begin);
         write_protect[begin >> 3] &= ~(1 << (begin & 0x7));
      }
   }
   else {
      termwin->printf("parse error");
   }
}

void do_breakset(char *p) {
   p = strchr(p, ' ');
   if (p) {
      p++;
      int pc = parsenum(p);
      displaywin->printf("break set %04x", pc);
      breakpoint[pc >> 3] |= (1 << (pc & 0x7));
   }
   else {
      termwin->printf("parse error");
   }
}

void do_breakclr(char *p) {
   p = strchr(p, ' ');
   if (p) {
      p++;
      int pc = parsenum(p);
      displaywin->printf("break clr %04x", pc);
      breakpoint[pc >> 3] &= ~(1 << (pc & 0x7));
   }
   else {
      termwin->printf("parse error");
   }
}

void do_mem(char *p) {
   p = strchr(p, ' ');
   if (p) {
      p++;
      memwin->set_address(parsenum(p) & ~0x0F);
   }
   else {
      termwin->printf("parse error");
   }
}

uint32_t fetch(const char *s, uint16_t offset, uint8_t count)
{
   uint32_t ret = 0;
   uint32_t val;
   for (int i = 0; i < count; i++) {
      ret <<= 4;
      if (s[offset + i] <= '9') {
         val = s[offset + i] - '0';
      }
      else if (s[offset + i] <= 'F') {
         val = s[offset + i] - 'A' + 10;
      }
      else if (s[offset + i] <= 'f') {
         val = s[offset + i] - 'a' + 10;
      }
      ret |= val;
   }
   return ret;
}

void do_ihex(char *p) {
   p = strchr(p, ' ');
   if (p) {
      p++;

      char buf[1024];
      FILE *f = fopen(p, "r");
      if (f) {
         while (NULL != fgets(buf, sizeof(buf), f)) {
            if (buf[0] != ':') {
               termwin->printf("unexpected start code in hex file");
               return;
            }

            // TODO FIX verify checksum for line here!

            int length = fetch(buf, 1, 2);
            int address = fetch(buf, 3, 4);
            int record = fetch(buf, 7, 2);
            if (record == 0x00) {
               for (int i = 0; i < length; i++) {
                  ram[address + i] = fetch(buf, 9 + 2 * i, 2);
               }
            }
            else if (record == 0x01) {
               // do nothing
            }
            else {
               termwin->printf("unexpected record type in hex file");
               return;
            }
         }
         fclose(f);
      }
      else {
         termwin->printf("error opening file");
      }
   }
   else {
      termwin->printf("parse error");
   }
}

void do_load(char *p) {
   p = strchr(p, ' ');
   if (p) {
      p++;
      int addr = parsenum(p);
      if (addr < 65536) {
         char *q = strchr(p, ' ');
         if (q) {
            q++;
            FILE *f = fopen(q, "r");
            if (f) {
               int ret = fread(ram + addr, 1, sizeof(ram) - addr, f);
               termwin->printf("loaded %d bytes at %04x", ret, addr);
            }
            else {
               termwin->printf("file open error %s", q);
            }
         }
         else {
            termwin->printf("parse error");
         }
      }
      else {
            termwin->printf("address out of range");
      }
   }
   else {
      termwin->printf("parse error");
   }
}

void step(void) {
   bus_error = false;
   breakpoint_hit = false;

   // TODO test for and break on illegal instructions

   uint64_t dummy;
   cpu->Run(1, dummy, mos6502::INST_COUNT);

   uint16_t pc = cpu->GetPC();
   if (breakpoint[pc >> 3] & (1 << (pc & 0x7))) {
      displaywin->printf("break at %04x", pc);
      breakpoint_hit = true;
   }

   // also break on eternal loops
   if (ram[pc] == 0x4C && ram[pc+1] == (pc & 0xFF) && ram[pc+2] == (pc >> 8)) {
      displaywin->printf("self jmp break at %04x", pc);
      breakpoint_hit = true;
   }
}

void do_run(char *) {
   termwin->printf("running, hit space to break");
   nodelay(stdscr, TRUE);  // make getch() non-blocking
   while (!bus_error && !breakpoint_hit) {
      step();
      if (verbosity >= 1) {
         regwin->update();
      }
      if (verbosity >= 2) {
         memwin->update();
      }

      int ch = getch();
      if (ch != ERR && ch == ' ') {
         break;
      }
   }
   nodelay(stdscr, FALSE);  // make getch() blocking again
}

void do_step(char *) {
   step();
}

void do_verbose(char *p) {
   p = strchr(p, ' ');
   if (p) {
      p++;
      verbosity = parsenum(p);
   }
   else {
      termwin->printf("parse error");
   }
}

void do_help(char *);

struct Commands {
   const char *cmd;
   const char *args;
   const char *help;
   handler exe;
} commands[] = {
   { "A=",       "<num>", "set the A register", do_a },
   { "PC=",      "<num>", "set the PC", do_pc },
   { "SP=",      "<num>", "set the STACK POINTER", do_sp },
   { "SR=",      "<num>", "set the STATUS", do_sr },
   { "X=",       "<num>", "set the X register", do_x },
   { "Y=",       "<num>", "set the Y register", do_y },
   { "break",    " <num>", "set a breakpoint", do_breakset },
   { "ihex",     " <file>", "load ihex file", do_ihex },
   { "load",     " <num> <file>", "load binary file at addr", do_load },
   { "mem",      " <num>", "show memory at addr", do_mem },
   { "reset", NULL,    "reset the cpu", do_reset },
   { "run",   NULL,    "run program to breakpoint", do_run },
   { "step",   NULL,    "single step program", do_step },
   { "unbreak",  " <num>", "clr a breakpoint", do_breakclr },
   { "verbose",  " <num>", "set verbosity (0-3)", do_verbose },
   { "wpclr",    " <num>[-<num>]", "clr write protect on address or range of addresses", do_wpclr },
   { "wpset",    " <num>[-<num>]", "set write protect on address or range of addresses", do_wpset },

   { "quit",  NULL,    "quit the program", do_quit },
   { "exit",  NULL,    "same as quit", do_quit },
   { "help",  NULL,    "print help", do_help },
   { "?",     NULL,    "same as help", do_help },
};

void do_help(char *) {
   displaywin->printf("=== COMMANDS ===");
   for (int i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
      displaywin->printf("%s%s\t%s", commands[i].cmd, commands[i].args ? commands[i].args : "", commands[i].help);
   }
   displaywin->printf("================");
   displaywin->printf("numbers may be entered as $xx (hex), %%bbbbbbbb (binary),");
   displaywin->printf("   @ooo (octal) or decimal (no prefix)");
   displaywin->printf("================");
}

void command(char *p) {
   for (int i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
      if (!strncasecmp(p, commands[i].cmd, strlen(commands[i].cmd))) {
         commands[i].exe(p);
         return;
      }
   }
   termwin->printf("huh? type '?' for help");
}

int main() {
   setup_ncurses();

   cpu = new mos6502(bus_read, bus_write, tick);

   int H, W; getmaxyx(stdscr, H, W);

   termwin = new TerminalWin(0, H - 8, W, 8);
   memwin = new MemoryWin(W - 57, H - 18 - 8, 57, 18);
   regwin = new RegisterWin(W - 57, H - 18 - 8 - 3, 57, 3);
   displaywin = new TerminalWin(0, 0, W-57, H - 8);

   termwin->printf("Welcome. Type lines; use Up/Down to browse history. Enter accepts. ? for help");
   char buf[1024];

   while (!done) {
      termwin->getline(">>> ", buf);
      command(buf);

      regwin->update(); // make sure it is up to date no matter what
      memwin->update(); // make sure it is up to date no matter what
   }

   teardown_ncurses();

   return 0;
}

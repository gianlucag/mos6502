// compile with "g++ main.cpp ../mos6502.cpp -o main"

#include "../mos6502.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define NAME "\"name\": \""
#define CYCLES "\"cycles\": ["
#define INITIAL "\"initial\": {"
#define FINAL "\"final\": {"

#define PC "\"pc\": "
#define S  "\"s\": "
#define A  "\"a\": "
#define X  "\"x\": "
#define Y  "\"y\": "
#define P  "\"p\": "
#define RAM  "\"ram\": ["

bool quiet = false;
uint8_t ram[65536] = {0};
mos6502 *cpu = NULL;
int linenum;
int cycles;
uint64_t actual_cycles;

void writeRam(uint16_t addr, uint8_t val)
{
   ram[addr] = val;
}

uint8_t readRam(uint16_t addr)
{
   return ram[addr];
}

void tick(mos6502*)
{
}

void bail(const char *s)
{
   fprintf(stderr, "%s\n", s);
   exit(-1);
}

const char *locate(const char *haystack, const char *needle, const char *name)
{
   const char *ret = strstr(haystack, needle);
   if (ret) {
      ret += strlen(needle);
      return ret;
   }
   else {
      char buf[1024];
      sprintf(buf, "cannot find %s at %d", name, linenum);
      bail(buf);
   }
   return NULL;
}

void handle_name(char *copy)
{
   char *p = (char *) locate(copy, NAME, "NAME");
   char *q = strchr(p, '\"');
   if (q) {
      *q = 0;
      printf("NAME: %s\n", p);
   }
   else {
      char buf[1024];
      sprintf(buf, "cannot parse NAME at %d", linenum);
      bail(buf);
   }
   free(copy);
}

void handle_cycles(char *copy)
{
   char *p = (char *) locate(copy, CYCLES, "CYCLES");
   cycles = 0;

   while (*p) {
      if (*p == '[') {
         cycles++;
      }
      p++;
   }

   if (cycles < 2) {
      char buf[1024];
      sprintf(buf, "cannot parse CYCLES at %d", linenum);
      bail(buf);
   }

   printf("CYCLES: %d\n", cycles);

   free(copy);
}

void handle_initial(char *copy)
{
   char *p = (char *) locate(copy, INITIAL, "INITIAL");

   cpu->SetPC(atoi(locate(p, PC, "INITIAL_PC")));
   cpu->SetS(atoi(locate(p, S, "INITIAL_S")));
   cpu->SetA(atoi(locate(p, A, "INITIAL_A")));
   cpu->SetX(atoi(locate(p, X, "INITIAL_X")));
   cpu->SetY(atoi(locate(p, Y, "INITIAL_Y")));
   cpu->SetP(atoi(locate(p, P, "INITIAL_P")));

   bzero(ram, sizeof(ram));
   const char *q = locate(p, RAM, "INITIAL_RAM");

   while (*q != '}') {
      int addr, val;
      if (*q == '[') {
         sscanf(q+1, "%d, %d", &addr, &val);
         ram[addr] = val;
      }
      q++;
   }

   free(copy);
}

bool is_jam(uint8_t val) {
   // Instruction codes: 02, 12, 22, 32, 42, 52, 62, 72, 92, B2, D2, F2
   if ((val & 0x0F) == 0x02) {
      if ((val & 0x80) == 0) {
         return true;
      }
      if ((val & 0x10) == 0x10) {
         return true;
      }
   }
   return false;
}

void handle_final(char *copy, bool jammed)
{
   char buf[1024];
   char *p = (char *) locate(copy, FINAL, "FINAL");
   uint8_t val;
   uint16_t pcval;

   if (!jammed && cpu->GetPC() != (pcval = /* ASSIGN */ atoi(locate(p, PC, "FINAL_PC")))) {
      sprintf(buf, "FAIL: PC %04x != %04x at %d", cpu->GetPC(), pcval, linenum);
      bail(buf);
   }
   if (cpu->GetS() != (val = /* ASSIGN */ atoi(locate(p, S, "FINAL_S")))) {
      sprintf(buf, "FAIL: S %02x != %02x at %d", cpu->GetS(), val, linenum);
      bail(buf);
   }
   if (cpu->GetA() != (val = /* ASSIGN */ atoi(locate(p, A, "FINAL_A")))) {
      sprintf(buf, "FAIL: A %02x != %02x at %d", cpu->GetA(), val, linenum);
      bail(buf);
   }
   if (cpu->GetX() != (val = /* ASSIGN */ atoi(locate(p, X, "FINAL_X")))) {
      sprintf(buf, "FAIL: X %02x != %02x at %d", cpu->GetX(), val, linenum);
      bail(buf);
   }
   if (cpu->GetY() != (val = /* ASSIGN */ atoi(locate(p, Y, "FINAL_Y")))) {
      sprintf(buf, "FAIL: Y %02x != %02x at %d", cpu->GetY(), val, linenum);
      bail(buf);
   }
   if (cpu->GetP() != (val = /* ASSIGN */ atoi(locate(p, P, "FINAL_P")))) {
      sprintf(buf, "FAIL: P %02x != %02x at %d", cpu->GetP(), val, linenum);
      bail(buf);
   }

   const char *q = locate(p, RAM, "FINAL_RAM");

   while (*q != '}') {
      int addr, val;
      if (*q == '[') {
         sscanf(q+1, "%d, %d", &addr, &val);
         if (ram[addr] != val) {
            sprintf(buf, "FAIL: RAM[%04x] %02x != %02x at %d", addr, ram[addr], val, linenum);
            bail(buf);
         }
      }
      q++;
   }

   free(copy);

   printf("pass\n");
}

void handle_line(const char *line)
{
   handle_name(strdup(line));
   handle_cycles(strdup(line));
   handle_initial(strdup(line));

   actual_cycles = 0;

   bool jammed = false;

   if (!is_jam(ram[cpu->GetPC()])) {
      cpu->Run(1, actual_cycles, mos6502::INST_COUNT);

      if (cycles != actual_cycles) {
         char buf[1024];
         sprintf(buf, "FAIL: actual %d != cycles at %d", (int) actual_cycles, (int) cycles, linenum);
         bail(buf);
      }
   }
   else {
      jammed = true;
   }
   
   handle_final(strdup(line), jammed);
}

void handle_json(const char *fname)
{
   char buf[4096];
   char *p;
   linenum = 1;
   FILE *f = fopen(fname, "r");
   if (!f) {
      bail("could not open json file");
   }
   while (NULL != fgets(buf, sizeof(buf), f)) {
      // we assume a lot here...

      p = buf;
      if (p[0] == '[') {
         p++;
      }

      switch(p[0]) {
         case 0:
         case 0x0A:
         case 0x0D:
         case '[':
         case ']':
            break;
         case '{':
            handle_line(p);
            break;
         default:
            sprintf(buf, "parse error at line %d, %02x", linenum, p[0]);
            bail(buf);
            break;
      }
      linenum++;
   }
   fclose(f);
}

int main(int argc, char **argv) {
   if (argc != 2 && argc != 3) {
      fprintf(stderr, "Usage: %s <file>.json [quiet]\n", argv[0]);
      return -1;
   }

   if (argc == 3 && !strcmp(argv[2], "quiet")) {
      quiet = true;
   }

   cpu = new mos6502(readRam, writeRam, tick);
   cpu->Reset();

   handle_json(argv[1]);

   return 0;
}

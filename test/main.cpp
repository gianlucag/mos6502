// compile with "g++ main.cpp ../mos6502.cpp -o main"

#include "../mos6502.h"

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

uint8_t ram[65536] = {0};

mos6502 *cpu = NULL;

int start = -1;
int success = -1;
int retaddr = -1;

void writeRam(uint16_t addr, uint8_t val) {
   ram[addr] = val;
}

uint8_t readRam(uint16_t addr) {
   return ram[addr];
}

void tick(mos6502*) {
   static uint16_t lastpc = 0xFFFF;
   static int count = 0;
   uint16_t pc = cpu->GetPC();
   if (pc != lastpc) {
      printf("PC=%04x\r", pc);
   }
   if (pc == success) {
      printf("\nsuccess\n");
      exit(0);
   }
   if (pc == lastpc) {
      count++;
      if (count > 100) {
         if (retaddr != -1) {
            if (ram[retaddr]) {
               printf("\ncode %02X\n", ram[retaddr]);
               printf("Y=%02x\n",cpu->GetY());
               printf("N1=%02x N2=%02x\n", ram[0], ram[1]);
               printf("HA=%02x HNVZC=%02x\n", ram[2], ram[3]);
               printf("DA=%02x DNVZC=%02x\n", ram[4], ram[5]);
               printf("AR=%02x NF=%02x VF=%02x ZF=%02x CF=%02x\n", ram[6], ram[7], ram[8], ram[9], ram[10]);
               printf("FAIL\n");
               exit(-1);
            }
            else {
               printf("\nsuccess\n");
               exit(0);
            }
         }
         else {
            printf("\nFAIL\n");
            exit(-1);
         }
      }
   }
   else {
      count = 0;
   }
   lastpc = pc;
}

void bail(const char *s) {
   fprintf(stderr, "%s\n", s);
   exit(-1);
}

uint32_t fetch(const char *s, uint16_t offset, uint8_t count) {
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

void handle_hex(const char *fname) {
   char buf[1024];
   FILE *f = fopen(fname, "r");
   if (!f) {
      bail("could not open hex file");
   }
   while (NULL != fgets(buf, sizeof(buf), f)) {
      if (buf[0] != ':') {
         bail("unexpected start code in hex file");
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
         bail("unexpected record type in hex file");
      }
   }
   fclose(f);
}

int main(int argc, char **argv) {
   if (argc != 4) {
      fprintf(stderr, "Usage: %s <file>.hex <start> <success>\n", argv[0]);
      return -1;
   }

   handle_hex(argv[1]);
   start = strtoul(argv[2], NULL, 0);

   if (argv[3][0] == '?') {
      retaddr = strtoul(argv[3]+1, NULL, 0);
   }
   else {
      success = strtoul(argv[3], NULL, 0);
   }

   printf("start=%04X\n", start);
   ram[0xFFFC] = start & 0xFF;
   ram[0xFFFD] = start >> 8;

   cpu = new mos6502(readRam, writeRam, tick);
   cpu->Reset();
   cpu->RunEternally();

   return 0;
}

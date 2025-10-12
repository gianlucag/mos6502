// compile with "g++ main.cpp ../mos6502.cpp -o main"

#include "../mos6502.h"

#include <stddef.h>
#include <stdint.h>

uint8_t ram[65536];

mos6502 *cpu = NULL;

void writeRam(uint16_t addr, uint8_t val) {
   ram[addr] = val;
}

uint8_t readRam(uint16_t addr) {
   return ram[addr];
}

int main(int argc, char **argv) {
   cpu = new mos6502(readRam, writeRam);
   cpu->RunEternally();
}

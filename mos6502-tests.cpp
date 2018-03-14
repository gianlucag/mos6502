#include <iostream>
#include <iomanip>

#include "mos6502.h"

// Test our templates.
template mos::basic_memory<65536, 0xFF>;
template mos::system_bus<mos::basic_memory<65536, 0xFF>>;
template mos::mos6502<mos::basic_memory<65536, 0xFF>>;

using test_machine_t = mos::mos6502<mos::basic_memory<65536, 0xFF>>;

struct program
{
    using extra_init_type = void(*)(test_machine_t&);

    const char *label;
    int expected_cycles;
    int x_override;
    int y_override;
    std::initializer_list<std::uint8_t> code;
    extra_init_type extra_init = nullptr;
};

struct test
{
    const char *label;
    std::initializer_list<program> programs;
};

static const std::uint16_t BASE = 0x0200;
static test_machine_t machine;

void run_tests(const std::initializer_list<test> &tests);
void run_programs(const test &test);

int main()
{
    static auto rtiPrep = [](test_machine_t &machine)
    {
        machine.push_word(0x0500);  // Return address
        machine.push(0x20);         // Processor status
    };

    static auto rtsPrep = [](test_machine_t &machine)
    {
        machine.push_word(0x0500);  // Return address
    };

    static auto setCarry = [](test_machine_t &machine)
    {
        machine.status().carry(true);
    };

    static auto setZero = [](test_machine_t &machine)
    {
        machine.status().zero(true);
    };

    static auto setNegative = [](test_machine_t &machine)
    {
        machine.status().negative(true);
    };

    static auto setOverflow = [](test_machine_t &machine)
    {
        machine.status().overflow(true);
    };

    run_tests({
        test { "ADC, ADd with Carry", {
            program { "ADC #$44",       2, 0x00, 0x00, { 0x69, 0x44 } },
            program { "ADC $44",        3, 0x00, 0x00, { 0x65, 0x44 } },
            program { "ADC $44,X",      4, 0x00, 0x00, { 0x75, 0x44 } },
            program { "ADC $4408",      4, 0x00, 0x00, { 0x6D, 0x08, 0x44 } },
            program { "ADC $4408,X=00", 4, 0x00, 0x00, { 0x7D, 0x08, 0x44 } },
            program { "ADC $4408,X=FF", 5, 0xFF, 0x00, { 0x7D, 0x08, 0x44 } },
            program { "ADC $4408,Y=00", 4, 0x00, 0x00, { 0x79, 0x08, 0x44 } },
            program { "ADC $4408,Y=FF", 5, 0x00, 0xFF, { 0x79, 0x08, 0x44 } },
            program { "ADC ($44,X)",    6, 0x00, 0x00, { 0x61, 0x44 } },
            program { "ADC ($44),Y=00", 5, 0x00, 0x00, { 0x71, 0x44 } },
            program { "ADC ($44),Y=FF", 6, 0x00, 0xFF, { 0x71, 0x44 } },
        }},
        test { "AND, bitwise AND with accumulator", {
            program { "AND #$44",       2, 0x00, 0x00, { 0x29, 0x44 } },
            program { "AND $44",        3, 0x00, 0x00, { 0x25, 0x44 } },
            program { "AND $44,X",      4, 0x00, 0x00, { 0x35, 0x44 } },
            program { "AND $4408",      4, 0x00, 0x00, { 0x2D, 0x08, 0x44 } },
            program { "AND $4408,X=00", 4, 0x00, 0x00, { 0x3D, 0x08, 0x44 } },
            program { "AND $4408,X=FF", 5, 0xFF, 0x00, { 0x3D, 0x08, 0x44 } },
            program { "AND $4408,Y=00", 4, 0x00, 0x00, { 0x39, 0x08, 0x44 } },
            program { "AND $4408,Y=FF", 5, 0x00, 0xFF, { 0x39, 0x08, 0x44 } },
            program { "AND ($44,X)",    6, 0x00, 0x00, { 0x21, 0x44 } },
            program { "AND ($44),Y=00", 5, 0x00, 0x00, { 0x31, 0x44 } },
            program { "AND ($44),Y=FF", 6, 0x00, 0xFF, { 0x31, 0x44 } },
        }},
        test { "ASL, Arithmetic Shift Left", {
            program { "ASL A",          2, 0x00, 0x00, { 0x0A } },
            program { "ASL $44",        5, 0x00, 0x00, { 0x06, 0x44 } },
            program { "ASL $44,X",      6, 0x00, 0x00, { 0x16, 0x44 } },
            program { "ASL $4408",      6, 0x00, 0x00, { 0x0E, 0x08, 0x44 } },
            program { "ASL $4408,X=00", 7, 0x00, 0x00, { 0x1E, 0x08, 0x44 } },
            program { "ASL $4408,X=FF", 7, 0xFF, 0x00, { 0x1E, 0x08, 0x44 } },
        }},
        test { "BCC, Branch if Carry Cleared", {
            program { "BCC $08,C=1",    2, 0x00, 0x00, { 0x90, 0x08 }, setCarry },
            program { "BCC $08,C=0",    3, 0x00, 0x00, { 0x90, 0x08 } },
            program { "BCC $80,C=0",    4, 0x00, 0x00, { 0x90, 0x80 } },
        }},
        test { "BCS, Branch if Carry Set", {
            program { "BCS $08,C=0",    2, 0x00, 0x00, { 0xB0, 0x08 } },
            program { "BCS $08,C=1",    3, 0x00, 0x00, { 0xB0, 0x08 }, setCarry },
            program { "BCS $80,C=1",    4, 0x00, 0x00, { 0xB0, 0x80 }, setCarry },
        }},
        test { "BEQ, Branch if EQual", {
            program { "BEQ $08,Z=0",    2, 0x00, 0x00, { 0xF0, 0x08 } },
            program { "BEQ $08,Z=1",    3, 0x00, 0x00, { 0xF0, 0x08 }, setZero },
            program { "BEQ $80,Z=1",    4, 0x00, 0x00, { 0xF0, 0x80 }, setZero },
        }},
        test { "BIT, test BITs", {
            program { "BIT $44",        3, 0x00, 0x00, { 0x24, 0x44 } },
            program { "BIT $4408",      4, 0x00, 0x00, { 0x2C, 0x08, 0x44 } },
        }},
        test { "BMI, Branch if MInus", {
            program { "BMI $08,N=0",    2, 0x00, 0x00, { 0x30, 0x08 } },
            program { "BMI $08,N=1",    3, 0x00, 0x00, { 0x30, 0x08 }, setNegative },
            program { "BMI $80,N=1",    4, 0x00, 0x00, { 0x30, 0x80 }, setNegative },
        }},
        test { "BNE, Branch if Not Equal", {
            program { "BNE $08,Z=1",    2, 0x00, 0x00, { 0xD0, 0x08 }, setZero },
            program { "BNE $08,Z=0",    3, 0x00, 0x00, { 0xD0, 0x08 } },
            program { "BNE $80,Z=0",    4, 0x00, 0x00, { 0xD0, 0x80 } },
        }},
        test { "BPL, Branch if PLus", {
            program { "BPL $08,N=1",    2, 0x00, 0x00, { 0x10, 0x08 }, setNegative },
            program { "BPL $08,N=0",    3, 0x00, 0x00, { 0x10, 0x08 } },
            program { "BPL $80,N=0",    4, 0x00, 0x00, { 0x10, 0x80 } },
        }},
        test { "BReaK", {
            program { "BRK",            7, 0x00, 0x00, { 0x00 } },
        }},
        test { "BVC, Branch if oVerflow Cleared", {
            program { "BVC $08,V=1",    2, 0x00, 0x00, { 0x50, 0x08 }, setOverflow },
            program { "BVC $08,V=0",    3, 0x00, 0x00, { 0x50, 0x08 } },
            program { "BVC $80,V=0",    4, 0x00, 0x00, { 0x50, 0x80 } },
        }},
        test { "BCS, Branch if oVerflow Set", {
            program { "BVS $08,V=0",    2, 0x00, 0x00, { 0x70, 0x08 } },
            program { "BVS $08,V=1",    3, 0x00, 0x00, { 0x70, 0x08 }, setOverflow },
            program { "BVS $80,V=1",    4, 0x00, 0x00, { 0x70, 0x80 }, setOverflow },
        }},
        test { "CLear and SEt flags", {
            program { "CLC",            2, 0x00, 0x00, { 0x18 } },
            program { "CLD",            2, 0x00, 0x00, { 0xD8 } },
            program { "CLI",            2, 0x00, 0x00, { 0x58 } },
            program { "CLV",            2, 0x00, 0x00, { 0xB8 } },
            program { "SEC",            2, 0x00, 0x00, { 0x38 } },
            program { "SED",            2, 0x00, 0x00, { 0xF8 } },
            program { "SEI",            2, 0x00, 0x00, { 0x78 } },
        }},
        test { "CMP, CoMPare", {
            program { "CMP #$44",       2, 0x00, 0x00, { 0xC9, 0x44 } },
            program { "CMP $44",        3, 0x00, 0x00, { 0xC5, 0x44 } },
            program { "CMP $44,X",      4, 0x00, 0x00, { 0xD5, 0x44 } },
            program { "CMP $4408",      4, 0x00, 0x00, { 0xCD, 0x08, 0x44 } },
            program { "CMP $4408,X=00", 4, 0x00, 0x00, { 0xDD, 0x08, 0x44 } },
            program { "CMP $4408,X=FF", 5, 0xFF, 0x00, { 0xDD, 0x08, 0x44 } },
            program { "CMP $4408,Y=00", 4, 0x00, 0x00, { 0xD9, 0x08, 0x44 } },
            program { "CMP $4408,Y=FF", 5, 0x00, 0xFF, { 0xD9, 0x08, 0x44 } },
            program { "CMP ($44,X)",    6, 0x00, 0x00, { 0xC1, 0x44 } },
            program { "CMP ($44),Y=00", 5, 0x00, 0x00, { 0xD1, 0x44 } },
            program { "CMP ($44),Y=FF", 6, 0x00, 0xFF, { 0xD1, 0x44 } },
        }},
        test { "CPX, ComPare to X", {
            program { "CPX #$44",       2, 0x00, 0x00, { 0xE0, 0x44 } },
            program { "CPX $44",        3, 0x00, 0x00, { 0xE4, 0x44 } },
            program { "CPX $4408",      4, 0x00, 0x00, { 0xEC, 0x08, 0x44 } },
        }},
        test { "CPY, ComPare to Y", {
            program { "CPY #$44",       2, 0x00, 0x00, { 0xC0, 0x44 } },
            program { "CPY $44",        3, 0x00, 0x00, { 0xC4, 0x44 } },
            program { "CPY $4408",      4, 0x00, 0x00, { 0xCC, 0x08, 0x44 } },
        }},
        test { "DEC, DECrement in memory", {
            program { "DEC $44",        5, 0x00, 0x00, { 0xC6, 0x44 } },
            program { "DEC $44,X",      6, 0x00, 0x00, { 0xD6, 0x44 } },
            program { "DEC $4408",      6, 0x00, 0x00, { 0xCE, 0x08, 0x44 } },
            program { "DEC $4408,X=00", 7, 0x00, 0x00, { 0xDE, 0x08, 0x44 } },
            program { "DEC $4408,X=FF", 7, 0xFF, 0x00, { 0xDE, 0x08, 0x44 } },
        }},
        test { "DEcrement in X or Y registers", {
            program { "DEX",            2, 0x00, 0x00, { 0xCA } },
            program { "DEY",            2, 0x00, 0x00, { 0x88 } },
        }},
        test { "EOR, Exclusive OR", {
            program { "EOR #$44",       2, 0x00, 0x00, { 0x49, 0x44 } },
            program { "EOR $44",        3, 0x00, 0x00, { 0x45, 0x44 } },
            program { "EOR $44,X",      4, 0x00, 0x00, { 0x55, 0x44 } },
            program { "EOR $4408",      4, 0x00, 0x00, { 0x4D, 0x08, 0x44 } },
            program { "EOR $4408,X=00", 4, 0x00, 0x00, { 0x5D, 0x08, 0x44 } },
            program { "EOR $4408,X=FF", 5, 0xFF, 0x00, { 0x5D, 0x08, 0x44 } },
            program { "EOR $4408,Y=00", 4, 0x00, 0x00, { 0x59, 0x08, 0x44 } },
            program { "EOR $4408,Y=FF", 5, 0x00, 0xFF, { 0x59, 0x08, 0x44 } },
            program { "EOR ($44,X)",    6, 0x00, 0x00, { 0x41, 0x44 } },
            program { "EOR ($44),Y=00", 5, 0x00, 0x00, { 0x51, 0x44 } },
            program { "EOR ($44),Y=FF", 6, 0x00, 0xFF, { 0x51, 0x44 } },
        }},
        test { "INC, INCrement in memory", {
            program { "INC $44",        5, 0x00, 0x00, { 0xE6, 0x44 } },
            program { "INC $44,X",      6, 0x00, 0x00, { 0xF6, 0x44 } },
            program { "INC $4408",      6, 0x00, 0x00, { 0xEE, 0x08, 0x44 } },
            program { "INC $4408,X=00", 7, 0x00, 0x00, { 0xFE, 0x08, 0x44 } },
            program { "INC $4408,X=FF", 7, 0xFF, 0x00, { 0xFE, 0x08, 0x44 } },
        }},
        test { "INcrement in X or Y registers", {
            program { "INX",            2, 0x00, 0x00, { 0xE8 } },
            program { "INY",            2, 0x00, 0x00, { 0xC8 } },
        }},
        test { "JMP, JuMP", {
            program { "JMP $0500",      3, 0x00, 0x00, { 0x4C, 0x00, 0x05 } },
            program { "JMP ($FFFE)",    5, 0x00, 0x00, { 0x6C, 0xFE, 0xFF } },
        }},
        test { "JSR, Jump to SubRoutine", {
            program { "JSR $0500",      6, 0x00, 0x00, { 0x20, 0x00, 0x05 } },
        }},
        test { "LDA, LoaD Accumulator", {
            program { "LDA #$44",       2, 0x00, 0x00, { 0xA9, 0x44 } },
            program { "LDA $44",        3, 0x00, 0x00, { 0xA5, 0x44 } },
            program { "LDA $44,X",      4, 0x00, 0x00, { 0xB5, 0x44 } },
            program { "LDA $4408",      4, 0x00, 0x00, { 0xAD, 0x08, 0x44 } },
            program { "LDA $4408,X=00", 4, 0x00, 0x00, { 0xBD, 0x08, 0x44 } },
            program { "LDA $4408,X=FF", 5, 0xFF, 0x00, { 0xBD, 0x08, 0x44 } },
            program { "LDA $4408,Y=00", 4, 0x00, 0x00, { 0xB9, 0x08, 0x44 } },
            program { "LDA $4408,Y=FF", 5, 0x00, 0xFF, { 0xB9, 0x08, 0x44 } },
            program { "LDA ($44,X)",    6, 0x00, 0x00, { 0xA1, 0x44 } },
            program { "LDA ($44),Y=00", 5, 0x00, 0x00, { 0xB1, 0x44 } },
            program { "LDA ($44),Y=FF", 6, 0x00, 0xFF, { 0xB1, 0x44 } },
        }},
        test { "LDX, LoaD X register", {
            program { "LDY #$44",       2, 0x00, 0x00, { 0xA2, 0x44 } },
            program { "LDY $44",        3, 0x00, 0x00, { 0xA6, 0x44 } },
            program { "LDY $44,Y",      4, 0x00, 0x00, { 0xB6, 0x44 } },
            program { "LDY $4408",      4, 0x00, 0x00, { 0xAE, 0x08, 0x44 } },
            program { "LDY $4408,Y=00", 4, 0x00, 0x00, { 0xBE, 0x08, 0x44 } },
            program { "LDY $4408,Y=FF", 5, 0x00, 0xFF, { 0xBE, 0x08, 0x44 } },
        }},
        test { "LDY, LoaD Y register", {
            program { "LDY #$44",       2, 0x00, 0x00, { 0xA0, 0x44 } },
            program { "LDY $44",        3, 0x00, 0x00, { 0xA4, 0x44 } },
            program { "LDY $44,X",      4, 0x00, 0x00, { 0xB4, 0x44 } },
            program { "LDY $4408",      4, 0x00, 0x00, { 0xAC, 0x08, 0x44 } },
            program { "LDY $4408,X=00", 4, 0x00, 0x00, { 0xBC, 0x08, 0x44 } },
            program { "LDY $4408,X=FF", 5, 0xFF, 0x00, { 0xBC, 0x08, 0x44 } },
        }},
        test { "LSR, Logical Shift Right", {
            program { "LSR A",          2, 0x00, 0x00, { 0x4A } },
            program { "LSR $44",        5, 0x00, 0x00, { 0x46, 0x44 } },
            program { "LSR $44,X",      6, 0x00, 0x00, { 0x56, 0x44 } },
            program { "LSR $4408",      6, 0x00, 0x00, { 0x4E, 0x08, 0x44 } },
            program { "LSR $4408,X=00", 7, 0x00, 0x00, { 0x5E, 0x08, 0x44 } },
            program { "LSR $4408,X=FF", 7, 0xFF, 0x00, { 0x5E, 0x08, 0x44 } },
        }},
        test { "miscellaneous", {
            program { "NOP",            2, 0x00, 0x00, { 0xEA } },
        }},
        test { "ORA, logical inclusive OR with accumulator", {
            program { "ORA #$44",       2, 0x00, 0x00, { 0x09, 0x44 } },
            program { "ORA $44",        3, 0x00, 0x00, { 0x05, 0x44 } },
            program { "ORA $44,X",      4, 0x00, 0x00, { 0x15, 0x44 } },
            program { "ORA $4408",      4, 0x00, 0x00, { 0x0D, 0x08, 0x44 } },
            program { "ORA $4408,X=00", 4, 0x00, 0x00, { 0x1D, 0x08, 0x44 } },
            program { "ORA $4408,X=FF", 5, 0xFF, 0x00, { 0x1D, 0x08, 0x44 } },
            program { "ORA $4408,Y=00", 4, 0x00, 0x00, { 0x19, 0x08, 0x44 } },
            program { "ORA $4408,Y=FF", 5, 0x00, 0xFF, { 0x19, 0x08, 0x44 } },
            program { "ORA ($44,X)",    6, 0x00, 0x00, { 0x01, 0x44 } },
            program { "ORA ($44),Y=00", 5, 0x00, 0x00, { 0x11, 0x44 } },
            program { "ORA ($44),Y=FF", 6, 0x00, 0xFF, { 0x11, 0x44 } },
        }},
        test { "PusH and PulL to and from the stack", {
            program { "PHA",            3, 0x00, 0x00, { 0x48 } },
            program { "PHP",            3, 0x00, 0x00, { 0x08 } },
            program { "PLA",            4, 0x00, 0x00, { 0x68 } },
            program { "PLP",            4, 0x00, 0x00, { 0x28 } },
        }},
        test { "ROL, ROtate Left", {
            program { "ROL A",          2, 0x00, 0x00, { 0x2A } },
            program { "ROL $44",        5, 0x00, 0x00, { 0x26, 0x44 } },
            program { "ROL $44,X",      6, 0x00, 0x00, { 0x36, 0x44 } },
            program { "ROL $4408",      6, 0x00, 0x00, { 0x2E, 0x08, 0x44 } },
            program { "ROL $4408,X=00", 7, 0x00, 0x00, { 0x3E, 0x08, 0x44 } },
            program { "ROL $4408,X=FF", 7, 0xFF, 0x00, { 0x3E, 0x08, 0x44 } },
        }},
        test { "ROR, ROtate Right",{
            program { "ROR A",          2, 0x00, 0x00, { 0x6A } },
            program { "ROR $44",        5, 0x00, 0x00, { 0x66, 0x44 } },
            program { "ROR $44,X",      6, 0x00, 0x00, { 0x76, 0x44 } },
            program { "ROR $4408",      6, 0x00, 0x00, { 0x6E, 0x08, 0x44 } },
            program { "ROR $4408,X=00", 7, 0x00, 0x00, { 0x7E, 0x08, 0x44 } },
            program { "ROR $4408,X=FF", 7, 0xFF, 0x00, { 0x7E, 0x08, 0x44 } },
        }},
        test { "ReTurning", {
            program { "RTI",            6, 0x00, 0x00, { 0x40 }, rtiPrep },
            program { "RTS",            6, 0x00, 0x00, { 0x60 }, rtsPrep },
        }},
        test { "SBC, SuBtract with Carry", {
            program { "SBC #$44",       2, 0x00, 0x00, { 0xE9, 0x44 } },
            program { "SBC $44",        3, 0x00, 0x00, { 0xE5, 0x44 } },
            program { "SBC $44,X",      4, 0x00, 0x00, { 0xF5, 0x44 } },
            program { "SBC $4408",      4, 0x00, 0x00, { 0xED, 0x08, 0x44 } },
            program { "SBC $4408,X=00", 4, 0x00, 0x00, { 0xFD, 0x08, 0x44 } },
            program { "SBC $4408,X=FF", 5, 0xFF, 0x00, { 0xFD, 0x08, 0x44 } },
            program { "SBC $4408,Y=00", 4, 0x00, 0x00, { 0xF9, 0x08, 0x44 } },
            program { "SBC $4408,Y=FF", 5, 0x00, 0xFF, { 0xF9, 0x08, 0x44 } },
            program { "SBC ($44,X)",    6, 0x00, 0x00, { 0xE1, 0x44 } },
            program { "SBC ($44),Y=00", 5, 0x00, 0x00, { 0xF1, 0x44 } },
            program { "SBC ($44),Y=FF", 6, 0x00, 0xFF, { 0xF1, 0x44 } },
        }},
        test { "STA, STore Accumulator", {
            program { "STA $44",        3, 0x00, 0x00, { 0x85, 0x44 } },
            program { "STA $44,X",      4, 0x00, 0x00, { 0x95, 0x44 } },
            program { "STA $4408",      4, 0x00, 0x00, { 0x8D, 0x08, 0x44 } },
            program { "STA $4408,X=00", 5, 0x00, 0x00, { 0x9D, 0x08, 0x44 } },
            program { "STA $4408,X=FF", 5, 0xFF, 0x00, { 0x9D, 0x08, 0x44 } },
            program { "STA $4408,Y=00", 5, 0x00, 0x00, { 0x99, 0x08, 0x44 } },
            program { "STA $4408,Y=FF", 5, 0x00, 0xFF, { 0x99, 0x08, 0x44 } },
            program { "STA ($44,X)",    6, 0x00, 0x00, { 0x81, 0x44 } },
            program { "STA ($44),Y=00", 6, 0x00, 0x00, { 0x91, 0x44 } },
            program { "STA ($44),Y=FF", 6, 0x00, 0xFF, { 0x91, 0x44 } },
        }},
        test { "STX, STore X register", {
            program { "STX $44",        3, 0x00, 0x00, { 0x86, 0x44 } },
            program { "STX $44,Y",      4, 0x00, 0x00, { 0x96, 0x44 } },
            program { "STX $4408",      4, 0x00, 0x00, { 0x8E, 0x08, 0x44 } },
        }},
        test { "STY, STore Y register", {
            program { "STY $44",        3, 0x00, 0x00, { 0x84, 0x44 } },
            program { "STY $44,X",      4, 0x00, 0x00, { 0x94, 0x44 } },
            program { "STY $4408",      4, 0x00, 0x00, { 0x8C, 0x08, 0x44 } },
        }},
        test { "Transfer registers", {
            program { "TAX",            2, 0x00, 0x00, { 0xAA } },
            program { "TAY",            2, 0x00, 0x00, { 0xA8 } },
            program { "TSX",            2, 0x00, 0x00, { 0xBA } },
            program { "TXA",            2, 0x00, 0x00, { 0x8A } },
            program { "TXS",            2, 0x00, 0x00, { 0x9A } },
            program { "TYA",            2, 0x00, 0x00, { 0x98 } },
        }},
        // TODO: asserted /IRQ,
        // TODO: asserted /NMI,
    });

    return 0;
}

void run_tests(const std::initializer_list<test> &tests)
{
    for (const auto &test : tests)
    {
        std::cout << "# " << test.label << std::endl;
        run_programs(test);
    }
}

void run_programs(const test &test)
{
    std::size_t needToClean = 0;
    for (const auto &program : test.programs)
    {
        std::cout << "- " << std::left << std::setw(16) << program.label << "| ";

        // Setup the machine state for the program.
        machine.bus().write_word(0xFFFE, 0x0500);   // Interrupt and BRK vector.
        machine.bus().write_word(0xFFFC, BASE);     // Reset vector, where we start.
        machine.bus().write_word(0x4400, 0x0808);   // Just some data for testing.
        machine.bus().write_word(0x0044, 0x0808);   // Just some data for testing.
        machine.bus().load_input(BASE, program.code.begin(), program.code.end());
        machine.reset();

        // Handle any X or Y overrides.
        if (program.x_override > 0) machine.x(static_cast<std::uint8_t>(program.x_override));
        if (program.y_override > 0) machine.y(static_cast<std::uint8_t>(program.y_override));

        // Handle any special initialization.
        if (program.extra_init) program.extra_init(machine);

        // Run the test program.
        machine.run();

        // Number of cycles is the current count, sans the reset (6),
        // sans the illegal op-code fetch (1).
        size_t cyclesUsed = machine.cycles() - 7;
        std::cout << std::setw(3) << cyclesUsed;

        // Did the test pass or fail to meet the cycle count?
        std::cout << "| " << (cyclesUsed == program.expected_cycles ? "PASSED" : "FAILED");

        std::cout << std::endl;

        // Clear the last program to run so non of its instructions are left.
        needToClean = std::distance(program.code.begin(), program.code.end());
        machine.bus().fill(BASE, static_cast<std::uint16_t>(needToClean), 0xFF);
    }
}

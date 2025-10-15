#include "mos6502.h"

#define NEGATIVE  0x80
#define OVERFLOW  0x40
#define CONSTANT  0x20
#define BREAK     0x10
#define DECIMAL   0x08
#define INTERRUPT 0x04
#define ZERO      0x02
#define CARRY     0x01

#define SET_NEGATIVE(x) (x ? (status |= NEGATIVE) : (status &= (~NEGATIVE)) )
#define SET_OVERFLOW(x) (x ? (status |= OVERFLOW) : (status &= (~OVERFLOW)) )
//#define SET_CONSTANT(x) (x ? (status |= CONSTANT) : (status &= (~CONSTANT)) )
//#define SET_BREAK(x) (x ? (status |= BREAK) : (status &= (~BREAK)) )
#define SET_DECIMAL(x) (x ? (status |= DECIMAL) : (status &= (~DECIMAL)) )
#define SET_INTERRUPT(x) (x ? (status |= INTERRUPT) : (status &= (~INTERRUPT)) )
#define SET_ZERO(x) (x ? (status |= ZERO) : (status &= (~ZERO)) )
#define SET_CARRY(x) (x ? (status |= CARRY) : (status &= (~CARRY)) )

#define IF_NEGATIVE() ((status & NEGATIVE) ? true : false)
#define IF_OVERFLOW() ((status & OVERFLOW) ? true : false)
#define IF_CONSTANT() ((status & CONSTANT) ? true : false)
#define IF_BREAK() ((status & BREAK) ? true : false)
#define IF_DECIMAL() ((status & DECIMAL) ? true : false)
#define IF_INTERRUPT() ((status & INTERRUPT) ? true : false)
#define IF_ZERO() ((status & ZERO) ? true : false)
#define IF_CARRY() ((status & CARRY) ? true : false)

mos6502::Instr mos6502::InstrTable[256];

mos6502::mos6502(BusRead r, BusWrite w, ClockCycle c)
   : reset_A(0x00)
   , reset_X(0x00)
   , reset_Y(0x00)
   , reset_sp(0xFD)
   , reset_status(CONSTANT)
   , irq_line(true)
   , nmi_request(false)
   , nmi_inhibit(false)
   , nmi_line(true)
{
   Write = (BusWrite)w;
   Read = (BusRead)r;
   Cycle = (ClockCycle)c;

   static bool initialized = false;
   if (initialized) return;
   initialized = true;

   Instr instr;
   // fill jump table with ILLEGALs
   instr.addr = &mos6502::Addr_IMP;
   instr.saddr = "(null)";
   instr.code = &mos6502::Op_ILLEGAL;
   instr.scode = "(null)";
   instr.penalty = false;
   for(int i = 0; i < 256; i++)
   {
      InstrTable[i] = instr;
   }

   // insert opcodes
#define MAKE_INSTR(HEX, CODE, MODE, CYCLES, PENALTY) \
   instr.code = &mos6502::Op_ ## CODE; \
   instr.scode = # CODE; \
   instr.addr = &mos6502::Addr_ ## MODE; \
   instr.saddr = # MODE; \
   instr.cycles = CYCLES; \
   instr.penalty = PENALTY; \
   InstrTable[HEX] = instr;

// ADC
// Add Memory to Accumulator with Carry
// 
// A + M + C -> A, C
// N    Z       C       I       D       V
// +    +       +       -       -       +
// addressing   assembler       opc     bytes   cycles
// immediate    ADC #oper       69      2       2
// zeropage     ADC oper        65      2       3
// zeropage,X   ADC oper,X      75      2       4
// absolute     ADC oper        6D      3       4
// absolute,X   ADC oper,X      7D      3       4*
// absolute,Y   ADC oper,Y      79      3       4*
// (indirect,X) ADC (oper,X)    61      2       6
// (indirect),Y ADC (oper),Y    71      2       5*

   MAKE_INSTR(0x69, ADC, IMM, 2, false);
   MAKE_INSTR(0x65, ADC, ZER, 3, false);
   MAKE_INSTR(0x75, ADC, ZEX, 4, false);
   MAKE_INSTR(0x6D, ADC, ABS, 4, false);
   MAKE_INSTR(0x7D, ADC, ABX, 4, true);
   MAKE_INSTR(0x79, ADC, ABY, 4, true);
   MAKE_INSTR(0x61, ADC, INX, 6, false);
   MAKE_INSTR(0x71, ADC, INY, 5, true);

// AND
// AND Memory with Accumulator
// 
// A AND M -> A
// N    Z       C       I       D       V
// +    +       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// immediate    AND #oper       29      2       2
// zeropage     AND oper        25      2       3
// zeropage,X   AND oper,X      35      2       4
// absolute     AND oper        2D      3       4
// absolute,X   AND oper,X      3D      3       4*
// absolute,Y   AND oper,Y      39      3       4*
// (indirect,X) AND (oper,X)    21      2       6
// (indirect),Y AND (oper),Y    31      2       5*

   MAKE_INSTR(0x29, AND, IMM, 2, false);
   MAKE_INSTR(0x25, AND, ZER, 3, false);
   MAKE_INSTR(0x35, AND, ZEX, 4, false);
   MAKE_INSTR(0x2D, AND, ABS, 4, false);
   MAKE_INSTR(0x3D, AND, ABX, 4, true);
   MAKE_INSTR(0x39, AND, ABY, 4, true);
   MAKE_INSTR(0x21, AND, INX, 6, false);
   MAKE_INSTR(0x31, AND, INY, 5, true);

// ASL
// Shift Left One Bit (Memory or Accumulator)
// 
// C <- [76543210] <- 0
// N    Z       C       I       D       V
// +    +       +       -       -       -
// addressing   assembler       opc     bytes   cycles
// accumulator  ASL A           0A      1       2
// zeropage     ASL oper        06      2       5
// zeropage,X   ASL oper,X      16      2       6
// absolute     ASL oper        0E      3       6
// absolute,X   ASL oper,X      1E      3       7

   MAKE_INSTR(0x0A, ASL_ACC, ACC, 2, false);
   MAKE_INSTR(0x06, ASL, ZER, 5, false);
   MAKE_INSTR(0x16, ASL, ZEX, 6, false);
   MAKE_INSTR(0x0E, ASL, ABS, 6, false);
   MAKE_INSTR(0x1E, ASL, ABX, 7, false);

// BCC
// Branch on Carry Clear
// 
// branch on C = 0
// N    Z       C       I       D       V
// -    -       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// relative     BCC oper        90      2       2**

   MAKE_INSTR(0x90, BCC, REL, 2, true);

// BCS
// Branch on Carry Set
// 
// branch on C = 1
// N    Z       C       I       D       V
// -    -       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// relative     BCS oper        B0      2       2**

   MAKE_INSTR(0xB0, BCS, REL, 2, true);

// BEQ
// Branch on Result Zero
// 
// branch on Z = 1
// N    Z       C       I       D       V
// -    -       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// relative     BEQ oper        F0      2       2**

   MAKE_INSTR(0xF0, BEQ, REL, 2, true);

// BIT
// Test Bits in Memory with Accumulator
// 
// bits 7 and 6 of operand are transfered to bit 7 and 6 of SR (N,V);
// the zero-flag is set according to the result of the operand AND
// the accumulator (set, if the result is zero, unset otherwise).
// This allows a quick check of a few bits at once without affecting
// any of the registers, other than the status register (SR).
// 
// → Further details.
// 
// A AND M -> Z, M7 -> N, M6 -> V
// N    Z       C       I       D       V
// M7   +       -       -       -       M6
// addressing   assembler       opc     bytes   cycles
// zeropage     BIT oper        24      2       3
// absolute     BIT oper        2C      3       4

   MAKE_INSTR(0x24, BIT, ZER, 3, false);
   MAKE_INSTR(0x2C, BIT, ABS, 4, false);

// BMI
// Branch on Result Minus
// 
// branch on N = 1
// N    Z       C       I       D       V
// -    -       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// relative     BMI oper        30      2       2**

   MAKE_INSTR(0x30, BMI, REL, 2, true);

// BNE
// Branch on Result not Zero
// 
// branch on Z = 0
// N    Z       C       I       D       V
// -    -       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// relative     BNE oper        D0      2       2**

   MAKE_INSTR(0xD0, BNE, REL, 2, true);

// BPL
// Branch on Result Plus
// 
// branch on N = 0
// N    Z       C       I       D       V
// -    -       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// relative     BPL oper        10      2       2**

   MAKE_INSTR(0x10, BPL, REL, 2, true);

// BRK
// Force Break
// 
// BRK initiates a software interrupt similar to a hardware
// interrupt (IRQ). The return address pushed to the stack is
// PC+2, providing an extra byte of spacing for a break mark
// (identifying a reason for the break.)
// The status register will be pushed to the stack with the break
// flag set to 1. However, when retrieved during RTI or by a PLP
// instruction, the break flag will be ignored.
// The interrupt disable flag is not set automatically.
// 
// → Further details.
// 
// interrupt,
// push PC+2, push SR
// N    Z       C       I       D       V
// -    -       -       1       -       -
// addressing   assembler       opc     bytes   cycles
// implied      BRK     00      1       7

   MAKE_INSTR(0x00, BRK, IMP, 7, false);

// BVC
// Branch on Overflow Clear
// 
// branch on V = 0
// N    Z       C       I       D       V
// -    -       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// relative     BVC oper        50      2       2**

   MAKE_INSTR(0x50, BVC, REL, 2, true);

// BVS
// Branch on Overflow Set
// 
// branch on V = 1
// N    Z       C       I       D       V
// -    -       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// relative     BVS oper        70      2       2**

   MAKE_INSTR(0x70, BVS, REL, 2, true);

// CLC
// Clear Carry Flag
// 
// 0 -> C
// N    Z       C       I       D       V
// -    -       0       -       -       -
// addressing   assembler       opc     bytes   cycles
// implied      CLC     18      1       2

   MAKE_INSTR(0x18, CLC, IMP, 2, false);

// CLD
// Clear Decimal Mode
// 
// 0 -> D
// N    Z       C       I       D       V
// -    -       -       -       0       -
// addressing   assembler       opc     bytes   cycles
// implied      CLD     D8      1       2

   MAKE_INSTR(0xD8, CLD, IMP, 2, false);

// CLI
// Clear Interrupt Disable Bit
// 
// 0 -> I
// N    Z       C       I       D       V
// -    -       -       0       -       -
// addressing   assembler       opc     bytes   cycles
// implied      CLI     58      1       2

   MAKE_INSTR(0x58, CLI, IMP, 2, false);

// CLV
// Clear Overflow Flag
// 
// 0 -> V
// N    Z       C       I       D       V
// -    -       -       -       -       0
// addressing   assembler       opc     bytes   cycles
// implied      CLV     B8      1       2

   MAKE_INSTR(0xB8, CLV, IMP, 2, false);

// CMP
// Compare Memory with Accumulator
// 
// A - M
// N    Z       C       I       D       V
// +    +       +       -       -       -
// addressing   assembler       opc     bytes   cycles
// immediate    CMP #oper       C9      2       2
// zeropage     CMP oper        C5      2       3
// zeropage,X   CMP oper,X      D5      2       4
// absolute     CMP oper        CD      3       4
// absolute,X   CMP oper,X      DD      3       4*
// absolute,Y   CMP oper,Y      D9      3       4*
// (indirect,X) CMP (oper,X)    C1      2       6
// (indirect),Y CMP (oper),Y    D1      2       5*

   MAKE_INSTR(0xC9, CMP, IMM, 2, false);
   MAKE_INSTR(0xC5, CMP, ZER, 3, false);
   MAKE_INSTR(0xD5, CMP, ZEX, 4, false);
   MAKE_INSTR(0xCD, CMP, ABS, 4, false);
   MAKE_INSTR(0xDD, CMP, ABX, 4, true);
   MAKE_INSTR(0xD9, CMP, ABY, 4, true);
   MAKE_INSTR(0xC1, CMP, INX, 6, false);
   MAKE_INSTR(0xD1, CMP, INY, 5, true);

// CPX
// Compare Memory and Index X
// 
// X - M
// N    Z       C       I       D       V
// +    +       +       -       -       -
// addressing   assembler       opc     bytes   cycles
// immediate    CPX #oper       E0      2       2
// zeropage     CPX oper        E4      2       3
// absolute     CPX oper        EC      3       4

   MAKE_INSTR(0xE0, CPX, IMM, 2, false);
   MAKE_INSTR(0xE4, CPX, ZER, 3, false);
   MAKE_INSTR(0xEC, CPX, ABS, 4, false);

// CPY
// Compare Memory and Index Y
// 
// Y - M
// N    Z       C       I       D       V
// +    +       +       -       -       -
// addressing   assembler       opc     bytes   cycles
// immediate    CPY #oper       C0      2       2
// zeropage     CPY oper        C4      2       3
// absolute     CPY oper        CC      3       4

   MAKE_INSTR(0xC0, CPY, IMM, 2, false);
   MAKE_INSTR(0xC4, CPY, ZER, 3, false);
   MAKE_INSTR(0xCC, CPY, ABS, 4, false);

// DEC
// Decrement Memory by One
// 
// M - 1 -> M
// N    Z       C       I       D       V
// +    +       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// zeropage     DEC oper        C6      2       5
// zeropage,X   DEC oper,X      D6      2       6
// absolute     DEC oper        CE      3       6
// absolute,X   DEC oper,X      DE      3       7

   MAKE_INSTR(0xC6, DEC, ZER, 5, false);
   MAKE_INSTR(0xD6, DEC, ZEX, 6, false);
   MAKE_INSTR(0xCE, DEC, ABS, 6, false);
   MAKE_INSTR(0xDE, DEC, ABX, 7, false);

// DEX
// Decrement Index X by One
// 
// X - 1 -> X
// N    Z       C       I       D       V
// +    +       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// implied      DEX     CA      1       2

   MAKE_INSTR(0xCA, DEX, IMP, 2, false);

// DEY
// Decrement Index Y by One
// 
// Y - 1 -> Y
// N    Z       C       I       D       V
// +    +       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// implied      DEY     88      1       2

   MAKE_INSTR(0x88, DEY, IMP, 2, false);

// EOR
// Exclusive-OR Memory with Accumulator
// 
// A EOR M -> A
// N    Z       C       I       D       V
// +    +       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// immediate    EOR #oper       49      2       2
// zeropage     EOR oper        45      2       3
// zeropage,X   EOR oper,X      55      2       4
// absolute     EOR oper        4D      3       4
// absolute,X   EOR oper,X      5D      3       4*
// absolute,Y   EOR oper,Y      59      3       4*
// (indirect,X) EOR (oper,X)    41      2       6
// (indirect),Y EOR (oper),Y    51      2       5*

   MAKE_INSTR(0x49, EOR, IMM, 2, false);
   MAKE_INSTR(0x45, EOR, ZER, 3, false);
   MAKE_INSTR(0x55, EOR, ZEX, 4, false);
   MAKE_INSTR(0x4D, EOR, ABS, 4, false);
   MAKE_INSTR(0x5D, EOR, ABX, 4, true);
   MAKE_INSTR(0x59, EOR, ABY, 4, true);
   MAKE_INSTR(0x41, EOR, INX, 6, false);
   MAKE_INSTR(0x51, EOR, INY, 5, true);

// INC
// Increment Memory by One
// 
// M + 1 -> M
// N    Z       C       I       D       V
// +    +       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// zeropage     INC oper        E6      2       5
// zeropage,X   INC oper,X      F6      2       6
// absolute     INC oper        EE      3       6
// absolute,X   INC oper,X      FE      3       7

   MAKE_INSTR(0xE6, INC, ZER, 5, false);
   MAKE_INSTR(0xF6, INC, ZEX, 6, false);
   MAKE_INSTR(0xEE, INC, ABS, 6, false);
   MAKE_INSTR(0xFE, INC, ABX, 7, false);

// INX
// Increment Index X by One
// 
// X + 1 -> X
// N    Z       C       I       D       V
// +    +       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// implied      INX     E8      1       2

   MAKE_INSTR(0xE8, INX, IMP, 2, false);

// INY
// Increment Index Y by One
// 
// Y + 1 -> Y
// N    Z       C       I       D       V
// +    +       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// implied      INY     C8      1       2

   MAKE_INSTR(0xC8, INY, IMP, 2, false);

// JMP
// Jump to New Location
// 
// operand 1st byte -> PCL
// operand 2nd byte -> PCH
// N    Z       C       I       D       V
// -    -       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// absolute     JMP oper        4C      3       3
// indirect     JMP (oper)      6C      3       5

   MAKE_INSTR(0x4C, JMP, ABS, 3, false);
   MAKE_INSTR(0x6C, JMP, ABI, 5, false);

// JSR
// Jump to New Location Saving Return Address
// 
// push (PC+2),
// operand 1st byte -> PCL
// operand 2nd byte -> PCH
// N    Z       C       I       D       V
// -    -       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// absolute     JSR oper        20      3       6

   MAKE_INSTR(0x20, JSR, ABS, 6, false);

// LDA
// Load Accumulator with Memory
// 
// M -> A
// N    Z       C       I       D       V
// +    +       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// immediate    LDA #oper       A9      2       2
// zeropage     LDA oper        A5      2       3
// zeropage,X   LDA oper,X      B5      2       4
// absolute     LDA oper        AD      3       4
// absolute,X   LDA oper,X      BD      3       4*
// absolute,Y   LDA oper,Y      B9      3       4*
// (indirect,X) LDA (oper,X)    A1      2       6
// (indirect),Y LDA (oper),Y    B1      2       5*

   MAKE_INSTR(0xA9, LDA, IMM, 2, false);
   MAKE_INSTR(0xA5, LDA, ZER, 3, false);
   MAKE_INSTR(0xB5, LDA, ZEX, 4, false);
   MAKE_INSTR(0xAD, LDA, ABS, 4, false);
   MAKE_INSTR(0xBD, LDA, ABX, 4, true);
   MAKE_INSTR(0xB9, LDA, ABY, 4, true);
   MAKE_INSTR(0xA1, LDA, INX, 6, false);
   MAKE_INSTR(0xB1, LDA, INY, 5, true);

// LDX
// Load Index X with Memory
// 
// M -> X
// N    Z       C       I       D       V
// +    +       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// immediate    LDX #oper       A2      2       2
// zeropage     LDX oper        A6      2       3
// zeropage,Y   LDX oper,Y      B6      2       4
// absolute     LDX oper        AE      3       4
// absolute,Y   LDX oper,Y      BE      3       4*

   MAKE_INSTR(0xA2, LDX, IMM, 2, false);
   MAKE_INSTR(0xA6, LDX, ZER, 3, false);
   MAKE_INSTR(0xB6, LDX, ZEY, 4, false);
   MAKE_INSTR(0xAE, LDX, ABS, 4, false);
   MAKE_INSTR(0xBE, LDX, ABY, 4, true);

// LDY
// Load Index Y with Memory
// 
// M -> Y
// N    Z       C       I       D       V
// +    +       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// immediate    LDY #oper       A0      2       2
// zeropage     LDY oper        A4      2       3
// zeropage,X   LDY oper,X      B4      2       4
// absolute     LDY oper        AC      3       4
// absolute,X   LDY oper,X      BC      3       4*

   MAKE_INSTR(0xA0, LDY, IMM, 2, false);
   MAKE_INSTR(0xA4, LDY, ZER, 3, false);
   MAKE_INSTR(0xB4, LDY, ZEX, 4, false);
   MAKE_INSTR(0xAC, LDY, ABS, 4, false);
   MAKE_INSTR(0xBC, LDY, ABX, 4, true);

// LSR
// Shift One Bit Right (Memory or Accumulator)
// 
// 0 -> [76543210] -> C
// N    Z       C       I       D       V
// 0    +       +       -       -       -
// addressing   assembler       opc     bytes   cycles
// accumulator  LSR A           4A      1       2
// zeropage     LSR oper        46      2       5
// zeropage,X   LSR oper,X      56      2       6
// absolute     LSR oper        4E      3       6
// absolute,X   LSR oper,X      5E      3       7

   MAKE_INSTR(0x4A, LSR_ACC, ACC, 2, false);
   MAKE_INSTR(0x46, LSR, ZER, 5, false);
   MAKE_INSTR(0x56, LSR, ZEX, 6, false);
   MAKE_INSTR(0x4E, LSR, ABS, 6, false);
   MAKE_INSTR(0x5E, LSR, ABX, 7, false);

// NOP
// No Operation
// 
// ---
// N    Z       C       I       D       V
// -    -       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// implied      NOP     EA      1       2

   MAKE_INSTR(0xEA, NOP, IMP, 2, false);

// ORA
// OR Memory with Accumulator
// 
// A OR M -> A
// N    Z       C       I       D       V
// +    +       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// immediate    ORA #oper       09      2       2
// zeropage     ORA oper        05      2       3
// zeropage,X   ORA oper,X      15      2       4
// absolute     ORA oper        0D      3       4
// absolute,X   ORA oper,X      1D      3       4*
// absolute,Y   ORA oper,Y      19      3       4*
// (indirect,X) ORA (oper,X)    01      2       6
// (indirect),Y ORA (oper),Y    11      2       5*

   MAKE_INSTR(0x09, ORA, IMM, 2, false);
   MAKE_INSTR(0x05, ORA, ZER, 3, false);
   MAKE_INSTR(0x15, ORA, ZEX, 4, false);
   MAKE_INSTR(0x0D, ORA, ABS, 4, false);
   MAKE_INSTR(0x1D, ORA, ABX, 4, true);
   MAKE_INSTR(0x19, ORA, ABY, 4, true);
   MAKE_INSTR(0x01, ORA, INX, 6, false);
   MAKE_INSTR(0x11, ORA, INY, 5, true);

// PHA
// Push Accumulator on Stack
// 
// push A
// N    Z       C       I       D       V
// -    -       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// implied      PHA     48      1       3

   MAKE_INSTR(0x48, PHA, IMP, 3, false);

// PHP
// Push Processor Status on Stack
// 
// The status register will be pushed with the break
// flag and bit 5 set to 1.
// 
// push SR
// N    Z       C       I       D       V
// -    -       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// implied      PHP     08      1       3

   MAKE_INSTR(0x08, PHP, IMP, 3, false);

// PLA
// Pull Accumulator from Stack
// 
// pull A
// N    Z       C       I       D       V
// +    +       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// implied      PLA     68      1       4

   MAKE_INSTR(0x68, PLA, IMP, 4, false);

// PLP
// Pull Processor Status from Stack
// 
// The status register will be pulled with the break
// flag and bit 5 ignored.
// 
// pull SR
// N    Z       C       I       D       V
// from stack
// addressing   assembler       opc     bytes   cycles
// implied      PLP     28      1       4

   MAKE_INSTR(0x28, PLP, IMP, 4, false);

// ROL
// Rotate One Bit Left (Memory or Accumulator)
// 
// C <- [76543210] <- C
// N    Z       C       I       D       V
// +    +       +       -       -       -
// addressing   assembler       opc     bytes   cycles
// accumulator  ROL A           2A      1       2
// zeropage     ROL oper        26      2       5
// zeropage,X   ROL oper,X      36      2       6
// absolute     ROL oper        2E      3       6
// absolute,X   ROL oper,X      3E      3       7

   MAKE_INSTR(0x2A, ROL_ACC, ACC, 2, false);
   MAKE_INSTR(0x26, ROL, ZER, 5, false);
   MAKE_INSTR(0x36, ROL, ZEX, 6, false);
   MAKE_INSTR(0x2E, ROL, ABS, 6, false);
   MAKE_INSTR(0x3E, ROL, ABX, 7, false);

// ROR
// Rotate One Bit Right (Memory or Accumulator)
// 
// C -> [76543210] -> C
// N    Z       C       I       D       V
// +    +       +       -       -       -
// addressing   assembler       opc     bytes   cycles
// accumulator  ROR A           6A      1       2
// zeropage     ROR oper        66      2       5
// zeropage,X   ROR oper,X      76      2       6
// absolute     ROR oper        6E      3       6
// absolute,X   ROR oper,X      7E      3       7

   MAKE_INSTR(0x6A, ROR_ACC, ACC, 2, false);
   MAKE_INSTR(0x66, ROR, ZER, 5, false);
   MAKE_INSTR(0x76, ROR, ZEX, 6, false);
   MAKE_INSTR(0x6E, ROR, ABS, 6, false);
   MAKE_INSTR(0x7E, ROR, ABX, 7, false);

// RTI
// Return from Interrupt
// 
// The status register is pulled with the break flag
// and bit 5 ignored. Then PC is pulled from the stack.
// 
// pull SR, pull PC
// N    Z       C       I       D       V
// from stack
// addressing   assembler       opc     bytes   cycles
// implied      RTI     40      1       6

   MAKE_INSTR(0x40, RTI, IMP, 6, false);

// RTS
// Return from Subroutine
// 
// pull PC, PC+1 -> PC
// N    Z       C       I       D       V
// -    -       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// implied      RTS     60      1       6

   MAKE_INSTR(0x60, RTS, IMP, 6, false);

// SBC
// Subtract Memory from Accumulator with Borrow
// 
// A - M - C̅ -> A
// N    Z       C       I       D       V
// +    +       +       -       -       +
// addressing   assembler       opc     bytes   cycles
// immediate    SBC #oper       E9      2       2
// zeropage     SBC oper        E5      2       3
// zeropage,X   SBC oper,X      F5      2       4
// absolute     SBC oper        ED      3       4
// absolute,X   SBC oper,X      FD      3       4*
// absolute,Y   SBC oper,Y      F9      3       4*
// (indirect,X) SBC (oper,X)    E1      2       6
// (indirect),Y SBC (oper),Y    F1      2       5*

   MAKE_INSTR(0xE9, SBC, IMM, 2, false);
   MAKE_INSTR(0xE5, SBC, ZER, 3, false);
   MAKE_INSTR(0xF5, SBC, ZEX, 4, false);
   MAKE_INSTR(0xED, SBC, ABS, 4, false);
   MAKE_INSTR(0xFD, SBC, ABX, 4, true);
   MAKE_INSTR(0xF9, SBC, ABY, 4, true);
   MAKE_INSTR(0xE1, SBC, INX, 6, false);
   MAKE_INSTR(0xF1, SBC, INY, 5, true);

// SEC
// Set Carry Flag
// 
// 1 -> C
// N    Z       C       I       D       V
// -    -       1       -       -       -
// addressing   assembler       opc     bytes   cycles
// implied      SEC     38      1       2

   MAKE_INSTR(0x38, SEC, IMP, 2, false);

// SED
// Set Decimal Flag
// 
// 1 -> D
// N    Z       C       I       D       V
// -    -       -       -       1       -
// addressing   assembler       opc     bytes   cycles
// implied      SED     F8      1       2

   MAKE_INSTR(0xF8, SED, IMP, 2, false);

// SEI
// Set Interrupt Disable Status
// 
// 1 -> I
// N    Z       C       I       D       V
// -    -       -       1       -       -
// addressing   assembler       opc     bytes   cycles
// implied      SEI     78      1       2

   MAKE_INSTR(0x78, SEI, IMP, 2, false);

// STA
// Store Accumulator in Memory
// 
// A -> M
// N    Z       C       I       D       V
// -    -       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// zeropage     STA oper        85      2       3
// zeropage,X   STA oper,X      95      2       4
// absolute     STA oper        8D      3       4
// absolute,X   STA oper,X      9D      3       5
// absolute,Y   STA oper,Y      99      3       5
// (indirect,X) STA (oper,X)    81      2       6
// (indirect),Y STA (oper),Y    91      2       6

   MAKE_INSTR(0x85, STA, ZER, 3, false);
   MAKE_INSTR(0x95, STA, ZEX, 4, false);
   MAKE_INSTR(0x8D, STA, ABS, 4, false);
   MAKE_INSTR(0x9D, STA, ABX, 5, false);
   MAKE_INSTR(0x99, STA, ABY, 5, false);
   MAKE_INSTR(0x81, STA, INX, 6, false);
   MAKE_INSTR(0x91, STA, INY, 6, false);

// STX
// Store Index X in Memory
// 
// X -> M
// N    Z       C       I       D       V
// -    -       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// zeropage     STX oper        86      2       3
// zeropage,Y   STX oper,Y      96      2       4
// absolute     STX oper        8E      3       4

   MAKE_INSTR(0x86, STX, ZER, 3, false);
   MAKE_INSTR(0x96, STX, ZEY, 4, false);
   MAKE_INSTR(0x8E, STX, ABS, 4, false);

// STY
// Sore Index Y in Memory
// 
// Y -> M
// N    Z       C       I       D       V
// -    -       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// zeropage     STY oper        84      2       3
// zeropage,X   STY oper,X      94      2       4
// absolute     STY oper        8C      3       4

   MAKE_INSTR(0x84, STY, ZER, 3, false);
   MAKE_INSTR(0x94, STY, ZEX, 4, false);
   MAKE_INSTR(0x8C, STY, ABS, 4, false);

// TAX
// Transfer Accumulator to Index X
// 
// A -> X
// N    Z       C       I       D       V
// +    +       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// implied      TAX     AA      1       2

   MAKE_INSTR(0xAA, TAX, IMP, 2, false);

// TAY
// Transfer Accumulator to Index Y
// 
// A -> Y
// N    Z       C       I       D       V
// +    +       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// implied      TAY     A8      1       2

   MAKE_INSTR(0xA8, TAY, IMP, 2, false);

// TSX
// Transfer Stack Pointer to Index X
// 
// SP -> X
// N    Z       C       I       D       V
// +    +       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// implied      TSX     BA      1       2

   MAKE_INSTR(0xBA, TSX, IMP, 2, false);

// TXA
// Transfer Index X to Accumulator
// 
// X -> A
// N    Z       C       I       D       V
// +    +       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// implied      TXA     8A      1       2

   MAKE_INSTR(0x8A, TXA, IMP, 2, false);

// TXS
// Transfer Index X to Stack Register
// 
// X -> SP
// N    Z       C       I       D       V
// -    -       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// implied      TXS     9A      1       2

   MAKE_INSTR(0x9A, TXS, IMP, 2, false);

// TYA
// Transfer Index Y to Accumulator
// 
// Y -> A
// N    Z       C       I       D       V
// +    +       -       -       -       -
// addressing   assembler       opc     bytes   cycles
// implied      TYA     98      1       2

   MAKE_INSTR(0x98, TYA, IMP, 2, false);

   return;
}

uint16_t mos6502::Addr_ACC()
{
   return 0; // not used
}

uint16_t mos6502::Addr_IMM()
{
   return pc++;
}

uint16_t mos6502::Addr_ABS()
{
   uint16_t addrL;
   uint16_t addrH;
   uint16_t addr;

   addrL = Read(pc++);
   addrH = Read(pc++);

   addr = addrL + (addrH << 8);

   return addr;
}

uint16_t mos6502::Addr_ZER()
{
   return Read(pc++);
}

uint16_t mos6502::Addr_IMP()
{
   return 0; // not used
}

uint16_t mos6502::Addr_REL()
{
   uint16_t offset;
   uint16_t addr;

   offset = (uint16_t)Read(pc++);
   if (offset & 0x80) offset |= 0xFF00;
   addr = pc + (int16_t)offset;
   crossed = (addr & 0xFF00) != (pc & 0xFF00);

   return addr;
}

uint16_t mos6502::Addr_ABI()
{
   uint16_t addrL;
   uint16_t addrH;
   uint16_t effL;
   uint16_t effH;
   uint16_t abs;
   uint16_t addr;

   addrL = Read(pc++);
   addrH = Read(pc++);

   abs = (addrH << 8) | addrL;

   effL = Read(abs);

#ifndef CMOS_INDIRECT_JMP_FIX
   effH = Read((abs & 0xFF00) + ((abs + 1) & 0x00FF) );
#else
   effH = Read(abs + 1);
#endif

   addr = effL + 0x100 * effH;

   return addr;
}

uint16_t mos6502::Addr_ZEX()
{
   uint16_t addr = (Read(pc++) + X) & 0xFF;
   return addr;
}

uint16_t mos6502::Addr_ZEY()
{
   uint16_t addr = (Read(pc++) + Y) & 0xFF;
   return addr;
}

uint16_t mos6502::Addr_ABX()
{
   uint16_t addr;
   uint16_t addrL;
   uint16_t addrH;

   addrL = Read(pc++);
   addrH = Read(pc++);

   addr = addrL + (addrH << 8) + X;
   crossed = (addrL + X) > 255;
   return addr;
}

uint16_t mos6502::Addr_ABY()
{
   uint16_t addr;
   uint16_t addrL;
   uint16_t addrH;

   addrL = Read(pc++);
   addrH = Read(pc++);

   addr = addrL + (addrH << 8) + Y;
   crossed = (addrL + Y) > 255;
   return addr;
}

uint16_t mos6502::Addr_INX()
{
   uint16_t zeroL;
   uint16_t zeroH;
   uint16_t addr;

   zeroL = (Read(pc++) + X) & 0xFF;
   zeroH = (zeroL + 1) & 0xFF;
   addr = Read(zeroL) + (Read(zeroH) << 8);

   return addr;
}

uint16_t mos6502::Addr_INY()
{
   uint16_t baseL;
   uint16_t zeroL;
   uint16_t zeroH;
   uint16_t addr;

   zeroL = Read(pc++);
   zeroH = (zeroL + 1) & 0xFF;
   addr = (baseL = /* ASSIGN */ Read(zeroL)) + (Read(zeroH) << 8) + Y;
   crossed = (baseL + Y) > 255;

   return addr;
}

void mos6502::IRQ(bool line)
{
   irq_line = line;
}

void mos6502::NMI(bool line)
{
   // falling edge triggered
   if (nmi_line == true && line == false) {
      if (!nmi_inhibit) {
         nmi_request = true;
      }
   }
   nmi_line = line;
}

void mos6502::Reset()
{
   // do not set or clear irq_line, that's external to us
   // do not set or clear nmi_line, that's external to us
   nmi_request = false;
   nmi_inhibit = false;

   A = reset_A;
   Y = reset_Y;
   X = reset_X;

   // load PC from reset vector
   uint8_t pcl = Read(rstVectorL);
   uint8_t pch = Read(rstVectorH);
   pc = (pch << 8) + pcl;

   sp = reset_sp;

   status = reset_status | CONSTANT | BREAK;

   illegalOpcode = false;

   return;
}

void mos6502::StackPush(uint8_t byte)
{
   Write(0x0100 + sp, byte);
   if(sp == 0x00) sp = 0xFF;
   else sp--;
}

uint8_t mos6502::StackPop()
{
   if(sp == 0xFF) sp = 0x00;
   else sp++;
   return Read(0x0100 + sp);
}

void mos6502::Svc_IRQ()
{
   //SET_BREAK(0);
   StackPush((pc >> 8) & 0xFF);
   StackPush(pc & 0xFF);
   StackPush((status & ~BREAK) | CONSTANT);
   SET_INTERRUPT(1);

   // load PC from interrupt request vector
   uint8_t pcl = Read(irqVectorL);
   uint8_t pch = Read(irqVectorH);
   pc = (pch << 8) + pcl;
   return;
}

void mos6502::Svc_NMI()
{
   //SET_BREAK(0);
   StackPush((pc >> 8) & 0xFF);
   StackPush(pc & 0xFF);
   StackPush((status & ~BREAK) | CONSTANT);
   SET_INTERRUPT(1);

   // load PC from non-maskable interrupt vector
   uint8_t pcl = Read(nmiVectorL);
   uint8_t pch = Read(nmiVectorH);
   pc = (pch << 8) + pcl;
   return;
}

bool mos6502::CheckInterrupts() {

   // NMI is edge triggered
   if (nmi_request && !nmi_inhibit) {
      nmi_request = false;
      nmi_inhibit = true;
      Svc_NMI();
      return true;
   }

   // check disabled bit
   if(!IF_INTERRUPT()) {
      // IRQ is level triggered
      if (irq_line == false && !nmi_inhibit) {
         Svc_IRQ();
         return true;
      }
   }

   return false;
}

void mos6502::Run(
      int32_t cyclesRemaining,
      uint64_t& cycleCount,
      CycleMethod cycleMethod)
{
   uint8_t opcode;
   Instr instr;

   while(cyclesRemaining > 0 && !illegalOpcode)
   {
      if (CheckInterrupts()) {
         cycleCount += 6; // TODO FIX verify this is correct
      }

      // fetch
      opcode = Read(pc++);

      // decode
      instr = InstrTable[opcode];

      // execute
      Exec(instr);

      cycleCount += instr.cycles;
      if (instr.penalty && crossed) {
         cycleCount++;
      }
      cyclesRemaining -=
         cycleMethod == CYCLE_COUNT        ? instr.cycles
         /* cycleMethod == INST_COUNT */   : 1;

      // run clock cycle callback
      if (Cycle)
         for(int i = 0; i < instr.cycles; i++)
            Cycle(this);
   }
}

void mos6502::RunEternally()
{
   uint8_t opcode;
   Instr instr;

   while(!illegalOpcode)
   {
      CheckInterrupts();

      // fetch
      opcode = Read(pc++);

      // decode
      instr = InstrTable[opcode];

      // execute
      Exec(instr);

      // run clock cycle callback
      if (Cycle)
         for(int i = 0; i < instr.cycles; i++)
            Cycle(this);
   }
}

void mos6502::Exec(Instr i)
{
   crossed = false;
   uint16_t src = (this->*i.addr)();
   (this->*i.code)(src);
}

uint16_t mos6502::GetPC()
{
   return pc;
}

uint8_t mos6502::GetS()
{
   return sp;
}

uint8_t mos6502::GetP()
{
   return status;
}

uint8_t mos6502::GetA()
{
   return A;
}

uint8_t mos6502::GetX()
{
   return X;
}

uint8_t mos6502::GetY()
{
   return Y;
}

void mos6502::SetPC(uint16_t n)
{
   pc = n;
}

void mos6502::SetS (uint8_t n)
{
   sp = n;
}

void mos6502::SetP (uint8_t n)
{
   status = n;
}

void mos6502::SetA (uint8_t n)
{
   A = n;
}

void mos6502::SetX (uint8_t n)
{
   X = n;
}

void mos6502::SetY (uint8_t n)
{
   Y = n;
}

void mos6502::SetResetS(uint8_t value)
{
   reset_sp = value;
}

void mos6502::SetResetA(uint8_t value)
{
   reset_A = value;
}

void mos6502::SetResetX(uint8_t value)
{
   reset_X = value;
}

void mos6502::SetResetY(uint8_t value)
{
   reset_Y = value;
}

void mos6502::SetResetP(uint8_t value)
{
   reset_status = value | CONSTANT | BREAK;
}

uint8_t mos6502::GetResetS()
{
   return reset_sp;
}

uint8_t mos6502::GetResetP()
{
   return reset_status;
}

uint8_t mos6502::GetResetA()
{
   return reset_A;
}

uint8_t mos6502::GetResetX()
{
   return reset_X;
}

uint8_t mos6502::GetResetY()
{
   return reset_Y;
}

void mos6502::Op_ILLEGAL(uint16_t src)
{
   illegalOpcode = true;
}

void mos6502::Op_ADC(uint16_t src)
{
   uint8_t m = Read(src);
   unsigned int tmp = m + A + (IF_CARRY() ? 1 : 0);

   // N and V computed *BEFORE* adjustment
   SET_NEGATIVE(tmp & 0x80);
   SET_OVERFLOW(!((A ^ m) & 0x80) && ((A ^ tmp) & 0x80));

   if (IF_DECIMAL())
   {
      // see http://www.6502.org/tutorials/decimal_mode.html
      int AL = ((A & 0xF) + (m & 0xF) + (IF_CARRY() ? 1 : 0));
      if (AL >= 0xA) {
         AL = ((AL + 6) & 0xF) + 0x10;
      }
      tmp = (m & 0xF0) + (A & 0xF0) + AL;
      if (tmp >= 0xA0) tmp += 0x60;
   }

   // Z and C computed *AFTER* adjustment
   SET_ZERO(!(tmp & 0xFF));
   SET_CARRY(tmp > 0xFF);

   A = tmp & 0xFF;
   return;
}



void mos6502::Op_AND(uint16_t src)
{
   uint8_t m = Read(src);
   uint8_t res = m & A;
   SET_NEGATIVE(res & 0x80);
   SET_ZERO(!res);
   A = res;
   return;
}


void mos6502::Op_ASL(uint16_t src)
{
   uint8_t m = Read(src);
   SET_CARRY(m & 0x80);
   m <<= 1;
   m &= 0xFF;
   SET_NEGATIVE(m & 0x80);
   SET_ZERO(!m);
   Write(src, m);
   return;
}

void mos6502::Op_ASL_ACC(uint16_t src)
{
   uint8_t m = A;
   SET_CARRY(m & 0x80);
   m <<= 1;
   m &= 0xFF;
   SET_NEGATIVE(m & 0x80);
   SET_ZERO(!m);
   A = m;
   return;
}

void mos6502::Op_BCC(uint16_t src)
{
   if (!IF_CARRY())
   {
      pc = src;
   }
   else {
      crossed = false; // branch not taken does not suffer penalty
   }
   return;
}

void mos6502::Op_BCS(uint16_t src)
{
   if (IF_CARRY())
   {
      pc = src;
   }
   else {
      crossed = false; // branch not taken does not suffer penalty
   }
   return;
}

void mos6502::Op_BEQ(uint16_t src)
{
   if (IF_ZERO())
   {
      pc = src;
   }
   else {
      crossed = false; // branch not taken does not suffer penalty
   }
   return;
}

void mos6502::Op_BIT(uint16_t src)
{
   uint8_t m = Read(src);
   uint8_t res = m & A;
   SET_NEGATIVE(res & 0x80);
   status = (status & 0x3F) | (uint8_t)(m & 0xC0) | CONSTANT | BREAK;
   SET_ZERO(!res);
   return;
}

void mos6502::Op_BMI(uint16_t src)
{
   if (IF_NEGATIVE())
   {
      pc = src;
   }
   else {
      crossed = false; // branch not taken does not suffer penalty
   }
   return;
}

void mos6502::Op_BNE(uint16_t src)
{
   if (!IF_ZERO())
   {
      pc = src;
   }
   else {
      crossed = false; // branch not taken does not suffer penalty
   }
   return;
}

void mos6502::Op_BPL(uint16_t src)
{
   if (!IF_NEGATIVE())
   {
      pc = src;
   }
   else {
      crossed = false; // branch not taken does not suffer penalty
   }
   return;
}

void mos6502::Op_BRK(uint16_t src)
{
   pc++;
   StackPush((pc >> 8) & 0xFF);
   StackPush(pc & 0xFF);
   StackPush(status | CONSTANT | BREAK);
   SET_INTERRUPT(1);
   pc = (Read(irqVectorH) << 8) + Read(irqVectorL);
   return;
}

void mos6502::Op_BVC(uint16_t src)
{
   if (!IF_OVERFLOW())
   {
      pc = src;
   }
   else {
      crossed = false; // branch not taken does not suffer penalty
   }
   return;
}

void mos6502::Op_BVS(uint16_t src)
{
   if (IF_OVERFLOW())
   {
      pc = src;
   }
   else {
      crossed = false; // branch not taken does not suffer penalty
   }
   return;
}

void mos6502::Op_CLC(uint16_t src)
{
   SET_CARRY(0);
   return;
}

void mos6502::Op_CLD(uint16_t src)
{
   SET_DECIMAL(0);
   return;
}

void mos6502::Op_CLI(uint16_t src)
{
   SET_INTERRUPT(0);
   return;
}

void mos6502::Op_CLV(uint16_t src)
{
   SET_OVERFLOW(0);
   return;
}

void mos6502::Op_CMP(uint16_t src)
{
   unsigned int tmp = A - Read(src);
   SET_CARRY(tmp < 0x100);
   SET_NEGATIVE(tmp & 0x80);
   SET_ZERO(!(tmp & 0xFF));
   return;
}

void mos6502::Op_CPX(uint16_t src)
{
   unsigned int tmp = X - Read(src);
   SET_CARRY(tmp < 0x100);
   SET_NEGATIVE(tmp & 0x80);
   SET_ZERO(!(tmp & 0xFF));
   return;
}

void mos6502::Op_CPY(uint16_t src)
{
   unsigned int tmp = Y - Read(src);
   SET_CARRY(tmp < 0x100);
   SET_NEGATIVE(tmp & 0x80);
   SET_ZERO(!(tmp & 0xFF));
   return;
}

void mos6502::Op_DEC(uint16_t src)
{
   uint8_t m = Read(src);
   m = (m - 1) & 0xFF;
   SET_NEGATIVE(m & 0x80);
   SET_ZERO(!m);
   Write(src, m);
   return;
}

void mos6502::Op_DEX(uint16_t src)
{
   uint8_t m = X;
   m = (m - 1) & 0xFF;
   SET_NEGATIVE(m & 0x80);
   SET_ZERO(!m);
   X = m;
   return;
}

void mos6502::Op_DEY(uint16_t src)
{
   uint8_t m = Y;
   m = (m - 1) & 0xFF;
   SET_NEGATIVE(m & 0x80);
   SET_ZERO(!m);
   Y = m;
   return;
}

void mos6502::Op_EOR(uint16_t src)
{
   uint8_t m = Read(src);
   m = A ^ m;
   SET_NEGATIVE(m & 0x80);
   SET_ZERO(!m);
   A = m;
}

void mos6502::Op_INC(uint16_t src)
{
   uint8_t m = Read(src);
   m = (m + 1) & 0xFF;
   SET_NEGATIVE(m & 0x80);
   SET_ZERO(!m);
   Write(src, m);
}

void mos6502::Op_INX(uint16_t src)
{
   uint8_t m = X;
   m = (m + 1) & 0xFF;
   SET_NEGATIVE(m & 0x80);
   SET_ZERO(!m);
   X = m;
}

void mos6502::Op_INY(uint16_t src)
{
   uint8_t m = Y;
   m = (m + 1) & 0xFF;
   SET_NEGATIVE(m & 0x80);
   SET_ZERO(!m);
   Y = m;
}

void mos6502::Op_JMP(uint16_t src)
{
   pc = src;
}

void mos6502::Op_JSR(uint16_t src)
{
   pc--;
   StackPush((pc >> 8) & 0xFF);
   StackPush(pc & 0xFF);
   pc = src;
}

void mos6502::Op_LDA(uint16_t src)
{
   uint8_t m = Read(src);
   SET_NEGATIVE(m & 0x80);
   SET_ZERO(!m);
   A = m;
}

void mos6502::Op_LDX(uint16_t src)
{
   uint8_t m = Read(src);
   SET_NEGATIVE(m & 0x80);
   SET_ZERO(!m);
   X = m;
}

void mos6502::Op_LDY(uint16_t src)
{
   uint8_t m = Read(src);
   SET_NEGATIVE(m & 0x80);
   SET_ZERO(!m);
   Y = m;
}

void mos6502::Op_LSR(uint16_t src)
{
   uint8_t m = Read(src);
   SET_CARRY(m & 0x01);
   m >>= 1;
   SET_NEGATIVE(0);
   SET_ZERO(!m);
   Write(src, m);
}

void mos6502::Op_LSR_ACC(uint16_t src)
{
   uint8_t m = A;
   SET_CARRY(m & 0x01);
   m >>= 1;
   SET_NEGATIVE(0);
   SET_ZERO(!m);
   A = m;
}

void mos6502::Op_NOP(uint16_t src)
{
   return;
}

void mos6502::Op_ORA(uint16_t src)
{
   uint8_t m = Read(src);
   m = A | m;
   SET_NEGATIVE(m & 0x80);
   SET_ZERO(!m);
   A = m;
}

void mos6502::Op_PHA(uint16_t src)
{
   StackPush(A);
   return;
}

void mos6502::Op_PHP(uint16_t src)
{
   StackPush(status | CONSTANT | BREAK);
   return;
}

void mos6502::Op_PLA(uint16_t src)
{
   A = StackPop();
   SET_NEGATIVE(A & 0x80);
   SET_ZERO(!A);
   return;
}

void mos6502::Op_PLP(uint16_t src)
{
   status = StackPop() | CONSTANT | BREAK;
   //SET_CONSTANT(1);
   return;
}

void mos6502::Op_ROL(uint16_t src)
{
   uint16_t m = Read(src);
   m <<= 1;
   if (IF_CARRY()) m |= 0x01;
   SET_CARRY(m > 0xFF);
   m &= 0xFF;
   SET_NEGATIVE(m & 0x80);
   SET_ZERO(!m);
   Write(src, m);
   return;
}

void mos6502::Op_ROL_ACC(uint16_t src)
{
   uint16_t m = A;
   m <<= 1;
   if (IF_CARRY()) m |= 0x01;
   SET_CARRY(m > 0xFF);
   m &= 0xFF;
   SET_NEGATIVE(m & 0x80);
   SET_ZERO(!m);
   A = m;
   return;
}

void mos6502::Op_ROR(uint16_t src)
{
   uint16_t m = Read(src);
   if (IF_CARRY()) m |= 0x100;
   SET_CARRY(m & 0x01);
   m >>= 1;
   m &= 0xFF;
   SET_NEGATIVE(m & 0x80);
   SET_ZERO(!m);
   Write(src, m);
   return;
}

void mos6502::Op_ROR_ACC(uint16_t src)
{
   uint16_t m = A;
   if (IF_CARRY()) m |= 0x100;
   SET_CARRY(m & 0x01);
   m >>= 1;
   m &= 0xFF;
   SET_NEGATIVE(m & 0x80);
   SET_ZERO(!m);
   A = m;
   return;
}

void mos6502::Op_RTI(uint16_t src)
{
   uint8_t lo, hi;

   status = StackPop() | CONSTANT | BREAK;

   lo = StackPop();
   hi = StackPop();

   pc = (hi << 8) | lo;

   nmi_inhibit = false; // always, more efficient that if()

   return;
}

void mos6502::Op_RTS(uint16_t src)
{
   uint8_t lo, hi;

   lo = StackPop();
   hi = StackPop();

   pc = ((hi << 8) | lo) + 1;
   return;
}

void mos6502::Op_SBC(uint16_t src)
{
   uint8_t m   = Read(src);
   int tmp     = A - m - (IF_CARRY() ? 0 : 1);

   // N and V computed *BEFORE* adjustment (binary semantics)
   SET_NEGATIVE(tmp & 0x80 );
   SET_OVERFLOW(((A ^ m) & (A ^ tmp) & 0x80) != 0);

   if (IF_DECIMAL())
   {
      // see http://www.6502.org/tutorials/decimal_mode.html
      int AL = (A & 0x0F) - (m & 0x0F) - (IF_CARRY() ? 0 : 1);
      if (AL < 0) {
         AL = ((AL - 6) & 0x0F) - 0x10;
      }
      tmp = (A & 0xF0) - (m & 0xF0) + AL;
      if (tmp < 0) tmp -= 0x60;
   }

   // Z and C computed *AFTER* adjustment
   SET_ZERO(!(tmp & 0xFF));
   SET_CARRY( tmp >= 0 );

   A = tmp & 0xFF;
   return;
}

void mos6502::Op_SEC(uint16_t src)
{
   SET_CARRY(1);
   return;
}

void mos6502::Op_SED(uint16_t src)
{
   SET_DECIMAL(1);
   return;
}

void mos6502::Op_SEI(uint16_t src)
{
   SET_INTERRUPT(1);
   return;
}

void mos6502::Op_STA(uint16_t src)
{
   Write(src, A);
   return;
}

void mos6502::Op_STX(uint16_t src)
{
   Write(src, X);
   return;
}

void mos6502::Op_STY(uint16_t src)
{
   Write(src, Y);
   return;
}

void mos6502::Op_TAX(uint16_t src)
{
   uint8_t m = A;
   SET_NEGATIVE(m & 0x80);
   SET_ZERO(!m);
   X = m;
   return;
}

void mos6502::Op_TAY(uint16_t src)
{
   uint8_t m = A;
   SET_NEGATIVE(m & 0x80);
   SET_ZERO(!m);
   Y = m;
   return;
}

void mos6502::Op_TSX(uint16_t src)
{
   uint8_t m = sp;
   SET_NEGATIVE(m & 0x80);
   SET_ZERO(!m);
   X = m;
   return;
}

void mos6502::Op_TXA(uint16_t src)
{
   uint8_t m = X;
   SET_NEGATIVE(m & 0x80);
   SET_ZERO(!m);
   A = m;
   return;
}

void mos6502::Op_TXS(uint16_t src)
{
   sp = X;
   return;
}

void mos6502::Op_TYA(uint16_t src)
{
   uint8_t m = Y;
   SET_NEGATIVE(m & 0x80);
   SET_ZERO(!m);
   A = m;
   return;
}


#include "mos6502.h"
	
mos6502::mos6502(BusRead r, BusWrite w)
{
	Write = (BusWrite)w;
	Read = (BusRead)r;
	Instr instr;
	
	// fill jump table with ILLEGALs
	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_ILLEGAL;
	for(int i = 0; i < 256; i++)
	{
		InstrTable[i] = instr;
	}
	
	// insert opcodes
	
	instr.addr = &mos6502::Addr_IMM;
	instr.code = &mos6502::Op_ADC;
	InstrTable[0x69] = instr;
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_ADC;
	InstrTable[0x6D] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_ADC;
	InstrTable[0x65] = instr;
	instr.addr = &mos6502::Addr_INX;
	instr.code = &mos6502::Op_ADC;
	InstrTable[0x61] = instr;
	instr.addr = &mos6502::Addr_INY;
	instr.code = &mos6502::Op_ADC;
	InstrTable[0x71] = instr;
	instr.addr = &mos6502::Addr_ZEX;
	instr.code = &mos6502::Op_ADC;
	InstrTable[0x75] = instr;	
	instr.addr = &mos6502::Addr_ABX;
	instr.code = &mos6502::Op_ADC;
	InstrTable[0x7D] = instr;
	instr.addr = &mos6502::Addr_ABY;
	instr.code = &mos6502::Op_ADC;
	InstrTable[0x79] = instr;
	
	instr.addr = &mos6502::Addr_IMM;
	instr.code = &mos6502::Op_AND;
	InstrTable[0x29] = instr;
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_AND;
	InstrTable[0x2D] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_AND;
	InstrTable[0x25] = instr;
	instr.addr = &mos6502::Addr_INX;
	instr.code = &mos6502::Op_AND;
	InstrTable[0x21] = instr;
	instr.addr = &mos6502::Addr_INY;
	instr.code = &mos6502::Op_AND;
	InstrTable[0x31] = instr;
	instr.addr = &mos6502::Addr_ZEX;
	instr.code = &mos6502::Op_AND;
	InstrTable[0x35] = instr;	
	instr.addr = &mos6502::Addr_ABX;
	instr.code = &mos6502::Op_AND;
	InstrTable[0x3D] = instr;
	instr.addr = &mos6502::Addr_ABY;
	instr.code = &mos6502::Op_AND;
	InstrTable[0x39] = instr;
	
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_ASL;
	InstrTable[0x0E] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_ASL;
	InstrTable[0x06] = instr;
	instr.addr = &mos6502::Addr_ACC;
	instr.code = &mos6502::Op_ASL_ACC;
	InstrTable[0x0A] = instr;	
	instr.addr = &mos6502::Addr_ZEX;
	instr.code = &mos6502::Op_ASL;
	InstrTable[0x16] = instr;
	instr.addr = &mos6502::Addr_ABX;
	instr.code = &mos6502::Op_ASL;
	InstrTable[0x1E] = instr;
	
	instr.addr = &mos6502::Addr_REL;
	instr.code = &mos6502::Op_BCC;
	InstrTable[0x90] = instr;
	
	instr.addr = &mos6502::Addr_REL;
	instr.code = &mos6502::Op_BCS;
	InstrTable[0xB0] = instr;
	
	instr.addr = &mos6502::Addr_REL;
	instr.code = &mos6502::Op_BEQ;
	InstrTable[0xF0] = instr;
	
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_BIT;
	InstrTable[0x2C] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_BIT;
	InstrTable[0x24] = instr;
	
	instr.addr = &mos6502::Addr_REL;
	instr.code = &mos6502::Op_BMI;
	InstrTable[0x30] = instr;
	
	instr.addr = &mos6502::Addr_REL;
	instr.code = &mos6502::Op_BNE;
	InstrTable[0xD0] = instr;
	
	instr.addr = &mos6502::Addr_REL;
	instr.code = &mos6502::Op_BPL;
	InstrTable[0x10] = instr;
	
	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_BRK;
	InstrTable[0x00] = instr;
	
	instr.addr = &mos6502::Addr_REL;
	instr.code = &mos6502::Op_BVC;
	InstrTable[0x50] = instr;
	
	instr.addr = &mos6502::Addr_REL;
	instr.code = &mos6502::Op_BVS;
	InstrTable[0x70] = instr;
	
	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_CLC;
	InstrTable[0x18] = instr;
	
	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_CLD;
	InstrTable[0xD8] = instr;
	
	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_CLI;
	InstrTable[0x58] = instr;
	
	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_CLV;
	InstrTable[0xB8] = instr;
	
	instr.addr = &mos6502::Addr_IMM;
	instr.code = &mos6502::Op_CMP;
	InstrTable[0xC9] = instr;
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_CMP;
	InstrTable[0xCD] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_CMP;
	InstrTable[0xC5] = instr;
	instr.addr = &mos6502::Addr_INX;
	instr.code = &mos6502::Op_CMP;
	InstrTable[0xC1] = instr;
	instr.addr = &mos6502::Addr_INY;
	instr.code = &mos6502::Op_CMP;
	InstrTable[0xD1] = instr;
	instr.addr = &mos6502::Addr_ZEX;
	instr.code = &mos6502::Op_CMP;
	InstrTable[0xD5] = instr;	
	instr.addr = &mos6502::Addr_ABX;
	instr.code = &mos6502::Op_CMP;
	InstrTable[0xDD] = instr;
	instr.addr = &mos6502::Addr_ABY;
	instr.code = &mos6502::Op_CMP;
	InstrTable[0xD9] = instr;
	
	instr.addr = &mos6502::Addr_IMM;
	instr.code = &mos6502::Op_CPX;
	InstrTable[0xE0] = instr;
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_CPX;
	InstrTable[0xEC] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_CPX;
	InstrTable[0xE4] = instr;
	
	instr.addr = &mos6502::Addr_IMM;
	instr.code = &mos6502::Op_CPY;
	InstrTable[0xC0] = instr;
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_CPY;
	InstrTable[0xCC] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_CPY;
	InstrTable[0xC4] = instr;
	
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_DEC;
	InstrTable[0xCE] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_DEC;
	InstrTable[0xC6] = instr;
	instr.addr = &mos6502::Addr_ZEX;
	instr.code = &mos6502::Op_DEC;
	InstrTable[0xD6] = instr;
	instr.addr = &mos6502::Addr_ABX;
	instr.code = &mos6502::Op_DEC;
	InstrTable[0xDE] = instr;
	
	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_DEX;
	InstrTable[0xCA] = instr;
	
	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_DEY;
	InstrTable[0x88] = instr;
	
	instr.addr = &mos6502::Addr_IMM;
	instr.code = &mos6502::Op_EOR;
	InstrTable[0x49] = instr;
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_EOR;
	InstrTable[0x4D] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_EOR;
	InstrTable[0x45] = instr;
	instr.addr = &mos6502::Addr_INX;
	instr.code = &mos6502::Op_EOR;
	InstrTable[0x41] = instr;
	instr.addr = &mos6502::Addr_INY;
	instr.code = &mos6502::Op_EOR;
	InstrTable[0x51] = instr;
	instr.addr = &mos6502::Addr_ZEX;
	instr.code = &mos6502::Op_EOR;
	InstrTable[0x55] = instr;	
	instr.addr = &mos6502::Addr_ABX;
	instr.code = &mos6502::Op_EOR;
	InstrTable[0x5D] = instr;
	instr.addr = &mos6502::Addr_ABY;
	instr.code = &mos6502::Op_EOR;
	InstrTable[0x59] = instr;
	
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_INC;
	InstrTable[0xEE] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_INC;
	InstrTable[0xE6] = instr;
	instr.addr = &mos6502::Addr_ZEX;
	instr.code = &mos6502::Op_INC;
	InstrTable[0xF6] = instr;
	instr.addr = &mos6502::Addr_ABX;
	instr.code = &mos6502::Op_INC;
	InstrTable[0xFE] = instr;
	
	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_INX;
	InstrTable[0xE8] = instr;
	
	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_INY;
	InstrTable[0xC8] = instr;
	
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_JMP;
	InstrTable[0x4C] = instr;
	instr.addr = &mos6502::Addr_ABI;
	instr.code = &mos6502::Op_JMP;
	InstrTable[0x6C] = instr;
	
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_JSR;
	InstrTable[0x20] = instr;
	
	instr.addr = &mos6502::Addr_IMM;
	instr.code = &mos6502::Op_LDA;
	InstrTable[0xA9] = instr;
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_LDA;
	InstrTable[0xAD] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_LDA;
	InstrTable[0xA5] = instr;
	instr.addr = &mos6502::Addr_INX;
	instr.code = &mos6502::Op_LDA;
	InstrTable[0xA1] = instr;
	instr.addr = &mos6502::Addr_INY;
	instr.code = &mos6502::Op_LDA;
	InstrTable[0xB1] = instr;
	instr.addr = &mos6502::Addr_ZEX;
	instr.code = &mos6502::Op_LDA;
	InstrTable[0xB5] = instr;	
	instr.addr = &mos6502::Addr_ABX;
	instr.code = &mos6502::Op_LDA;
	InstrTable[0xBD] = instr;
	instr.addr = &mos6502::Addr_ABY;
	instr.code = &mos6502::Op_LDA;
	InstrTable[0xB9] = instr;
	
	instr.addr = &mos6502::Addr_IMM;
	instr.code = &mos6502::Op_LDX;
	InstrTable[0xA2] = instr;
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_LDX;
	InstrTable[0xAE] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_LDX;
	InstrTable[0xA6] = instr;
	instr.addr = &mos6502::Addr_ABY;
	instr.code = &mos6502::Op_LDX;
	InstrTable[0xBE] = instr;
	instr.addr = &mos6502::Addr_ZEY;
	instr.code = &mos6502::Op_LDX;
	InstrTable[0xB6] = instr;
	
	instr.addr = &mos6502::Addr_IMM;
	instr.code = &mos6502::Op_LDY;
	InstrTable[0xA0] = instr;
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_LDY;
	InstrTable[0xAC] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_LDY;
	InstrTable[0xA4] = instr;
	instr.addr = &mos6502::Addr_ZEX;
	instr.code = &mos6502::Op_LDY;
	InstrTable[0xB4] = instr;
	instr.addr = &mos6502::Addr_ABX;
	instr.code = &mos6502::Op_LDY;
	InstrTable[0xBC] = instr;
	
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_LSR;
	InstrTable[0x4E] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_LSR;
	InstrTable[0x46] = instr;
	instr.addr = &mos6502::Addr_ACC;
	instr.code = &mos6502::Op_LSR_ACC;
	InstrTable[0x4A] = instr;
	instr.addr = &mos6502::Addr_ZEX;
	instr.code = &mos6502::Op_LSR;
	InstrTable[0x56] = instr;
	instr.addr = &mos6502::Addr_ABX;
	instr.code = &mos6502::Op_LSR;
	InstrTable[0x5E] = instr;
	
	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_NOP;
	InstrTable[0xEA] = instr;
	
	instr.addr = &mos6502::Addr_IMM;
	instr.code = &mos6502::Op_ORA;
	InstrTable[0x09] = instr;
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_ORA;
	InstrTable[0x0D] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_ORA;
	InstrTable[0x05] = instr;
	instr.addr = &mos6502::Addr_INX;
	instr.code = &mos6502::Op_ORA;
	InstrTable[0x01] = instr;
	instr.addr = &mos6502::Addr_INY;
	instr.code = &mos6502::Op_ORA;
	InstrTable[0x11] = instr;
	instr.addr = &mos6502::Addr_ZEX;
	instr.code = &mos6502::Op_ORA;
	InstrTable[0x15] = instr;	
	instr.addr = &mos6502::Addr_ABX;
	instr.code = &mos6502::Op_ORA;
	InstrTable[0x1D] = instr;
	instr.addr = &mos6502::Addr_ABY;
	instr.code = &mos6502::Op_ORA;
	InstrTable[0x19] = instr;
	
	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_PHA;
	InstrTable[0x48] = instr;
	
	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_PHP;
	InstrTable[0x08] = instr;
	
	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_PLA;
	InstrTable[0x68] = instr;
	
	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_PLP;
	InstrTable[0x28] = instr;
	
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_ROL;
	InstrTable[0x2E] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_ROL;
	InstrTable[0x26] = instr;
	instr.addr = &mos6502::Addr_ACC;
	instr.code = &mos6502::Op_ROL_ACC;
	InstrTable[0x2A] = instr;
	instr.addr = &mos6502::Addr_ZEX;
	instr.code = &mos6502::Op_ROL;
	InstrTable[0x36] = instr;
	instr.addr = &mos6502::Addr_ABX;
	instr.code = &mos6502::Op_ROL;
	InstrTable[0x3E] = instr;
	
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_ROR;
	InstrTable[0x6E] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_ROR;
	InstrTable[0x66] = instr;
	instr.addr = &mos6502::Addr_ACC;
	instr.code = &mos6502::Op_ROR_ACC;
	InstrTable[0x6A] = instr;
	instr.addr = &mos6502::Addr_ZEX;
	instr.code = &mos6502::Op_ROR;
	InstrTable[0x76] = instr;
	instr.addr = &mos6502::Addr_ABX;
	instr.code = &mos6502::Op_ROR;
	InstrTable[0x7E] = instr;
	
	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_RTI;
	InstrTable[0x40] = instr;
	
	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_RTS;
	InstrTable[0x60] = instr;
	
	instr.addr = &mos6502::Addr_IMM;
	instr.code = &mos6502::Op_SBC;
	InstrTable[0xE9] = instr;
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_SBC;
	InstrTable[0xED] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_SBC;
	InstrTable[0xE5] = instr;
	instr.addr = &mos6502::Addr_INX;
	instr.code = &mos6502::Op_SBC;
	InstrTable[0xE1] = instr;
	instr.addr = &mos6502::Addr_INY;
	instr.code = &mos6502::Op_SBC;
	InstrTable[0xF1] = instr;
	instr.addr = &mos6502::Addr_ZEX;
	instr.code = &mos6502::Op_SBC;
	InstrTable[0xF5] = instr;
	instr.addr = &mos6502::Addr_ABX;
	instr.code = &mos6502::Op_SBC;
	InstrTable[0xFD] = instr;
	instr.addr = &mos6502::Addr_ABY;
	instr.code = &mos6502::Op_SBC;
	InstrTable[0xF9] = instr;
	
	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_SEC;
	InstrTable[0x38] = instr;
	
	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_SED;
	InstrTable[0xF8] = instr;
	
	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_SEI;
	InstrTable[0x78] = instr;
	
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_STA;
	InstrTable[0x8D] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_STA;
	InstrTable[0x85] = instr;
	instr.addr = &mos6502::Addr_INX;
	instr.code = &mos6502::Op_STA;
	InstrTable[0x81] = instr;
	instr.addr = &mos6502::Addr_INY;
	instr.code = &mos6502::Op_STA;
	InstrTable[0x91] = instr;
	instr.addr = &mos6502::Addr_ZEX;
	instr.code = &mos6502::Op_STA;
	InstrTable[0x95] = instr;
	instr.addr = &mos6502::Addr_ABX;
	instr.code = &mos6502::Op_STA;
	InstrTable[0x9D] = instr;
	instr.addr = &mos6502::Addr_ABY;
	instr.code = &mos6502::Op_STA;
	InstrTable[0x99] = instr;
	
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_STX;
	InstrTable[0x8E] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_STX;
	InstrTable[0x86] = instr;
	instr.addr = &mos6502::Addr_ZEY;
	instr.code = &mos6502::Op_STX;
	InstrTable[0x96] = instr;
	
	instr.addr = &mos6502::Addr_ABS;
	instr.code = &mos6502::Op_STY;
	InstrTable[0x8C] = instr;
	instr.addr = &mos6502::Addr_ZER;
	instr.code = &mos6502::Op_STY;
	InstrTable[0x84] = instr;
	instr.addr = &mos6502::Addr_ZEX;
	instr.code = &mos6502::Op_STY;
	InstrTable[0x94] = instr;
	
	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_TAX;
	InstrTable[0xAA] = instr;
	
	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_TAY;
	InstrTable[0xA8] = instr;
	
	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_TSX;
	InstrTable[0xBA] = instr;
	
	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_TXA;
	InstrTable[0x8A] = instr;
	
	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_TXS;
	InstrTable[0x9A] = instr;
	
	instr.addr = &mos6502::Addr_IMP;
	instr.code = &mos6502::Op_TYA;
	InstrTable[0x98] = instr;
	
	Reset();
	
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
	effH = Read((abs & 0xFF00) + ((abs + 1) & 0x00FF) );
	
	addr = effL + 0x100 * effH;
	
	return addr;
}

uint16_t mos6502::Addr_ZEX()
{
	uint16_t addr = (Read(pc++) + X) % 256;
	return addr;
}

uint16_t mos6502::Addr_ZEY()
{
	uint16_t addr = (Read(pc++) + Y) % 256;
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
	return addr;
}


uint16_t mos6502::Addr_INX()
{
	uint16_t zeroL;
	uint16_t zeroH;
	uint16_t addr;
	
	zeroL = (Read(pc++) + X) % 256;
	zeroH = (zeroL + 1) % 256;
	addr = Read(zeroL) + (Read(zeroH) << 8);
	
	return addr;
}

uint16_t mos6502::Addr_INY()
{
	uint16_t zeroL;
	uint16_t zeroH;
	uint16_t addr;
	
	zeroL = Read(pc++);
	zeroH = (zeroL + 1) % 256;
	addr = Read(zeroL) + (Read(zeroH) << 8) + Y;
	
	return addr;
}

void mos6502::Reset()
{
	A = 0x00;
	Y = 0x00;
	X = 0x00;
	
	pc = (Read(rstVectorH) << 8) + Read(rstVectorL); // load PC from reset vector
		
    sp = 0xFD;
    
    status |= CONSTANT;
    
	cycles = 6; // according to the datasheet, the reset routine takes 6 clock cycles

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

void mos6502::IRQ()
{
	if(!IF_INTERRUPT())
	{
		SET_BREAK(0);
		StackPush((pc >> 8) & 0xFF);
		StackPush(pc & 0xFF);
		StackPush(status);
		SET_INTERRUPT(1);
		pc = (Read(irqVectorH) << 8) + Read(irqVectorL);
	}
	return;
}

void mos6502::NMI()
{
	SET_BREAK(0);
	StackPush((pc >> 8) & 0xFF);
	StackPush(pc & 0xFF);
	StackPush(status);
	SET_INTERRUPT(1);
	pc = (Read(nmiVectorH) << 8) + Read(nmiVectorL);
	return;
}

void mos6502::Run(uint32_t n)
{
	uint32_t start = cycles;
	uint8_t opcode;
	Instr instr;

	while(start + n > cycles && !illegalOpcode)
	{
		// fetch
		opcode = Read(pc++);
		
		// decode
		instr = InstrTable[opcode];
		
		// execute
		Exec(instr);
		
		cycles++;
	}
}

void mos6502::Exec(Instr i)
{
	uint16_t src = (this->*i.addr)();
	(this->*i.code)(src);
}


void mos6502::Op_ILLEGAL(uint16_t src)
{
	illegalOpcode = true;
}


void mos6502::Op_ADC(uint16_t src)
{
	uint8_t m = Read(src);
	unsigned int tmp = m + A + (IF_CARRY() ? 1 : 0);
	SET_ZERO(!(tmp & 0xFF));
    if (IF_DECIMAL())
    {
        if (((A & 0xF) + (m & 0xF) + (IF_CARRY() ? 1 : 0)) > 9) tmp += 6;
        SET_NEGATIVE(tmp & 0x80);
        SET_OVERFLOW(!((A ^ m) & 0x80) && ((A ^ tmp) & 0x80));
        if (tmp > 0x99)
        {
        	tmp += 96;
        }
        SET_CARRY(tmp > 0x99);
    }
	else
	{
		SET_NEGATIVE(tmp & 0x80);
		SET_OVERFLOW(!((A ^ m) & 0x80) && ((A ^ tmp) & 0x80));
		SET_CARRY(tmp > 0xFF);
    }
	
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
    return;
}


void mos6502::Op_BCS(uint16_t src)
{
    if (IF_CARRY())
    {
    	pc = src;
    }
    return;
}

void mos6502::Op_BEQ(uint16_t src)
{
    if (IF_ZERO())
    {
    	pc = src;
    }
    return;
}

void mos6502::Op_BIT(uint16_t src)
{
	uint8_t m = Read(src);
	uint8_t res = m & A;
	SET_NEGATIVE(res & 0x80);
	status = (status & 0x3F) | (uint8_t)(m & 0xC0);
	SET_ZERO(!res);
	return;
}

void mos6502::Op_BMI(uint16_t src)
{
    if (IF_NEGATIVE())
    {
    	pc = src;
    }
    return;
}

void mos6502::Op_BNE(uint16_t src)
{
    if (!IF_ZERO())
    {
    	pc = src;
    }
    return;
}

void mos6502::Op_BPL(uint16_t src)
{
    if (!IF_NEGATIVE())
    {
    	pc = src;
    }
    return;
}

void mos6502::Op_BRK(uint16_t src)
{
	pc++;
	StackPush((pc >> 8) & 0xFF);
	StackPush(pc & 0xFF);
	StackPush(status | BREAK);
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
    return;
}

void mos6502::Op_BVS(uint16_t src)
{
    if (IF_OVERFLOW())
    {
    	pc = src;
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
	m = (m - 1) % 256;
    SET_NEGATIVE(m & 0x80);
    SET_ZERO(!m);
    Write(src, m);
	return;
}

void mos6502::Op_DEX(uint16_t src)
{
	uint8_t m = X;
	m = (m - 1) % 256;
	SET_NEGATIVE(m & 0x80);
    SET_ZERO(!m);
    X = m;
    return;
}

void mos6502::Op_DEY(uint16_t src)
{
	uint8_t m = Y;
	m = (m - 1) % 256;
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
	m = (m + 1) % 256;
	SET_NEGATIVE(m & 0x80);
    SET_ZERO(!m);
    Write(src, m);
}

void mos6502::Op_INX(uint16_t src)
{
	uint8_t m = X;
	m = (m + 1) % 256;
	SET_NEGATIVE(m & 0x80);
    SET_ZERO(!m);
    X = m;
}

void mos6502::Op_INY(uint16_t src)
{
	uint8_t m = Y;
	m = (m + 1) % 256;
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
	StackPush(status | BREAK);
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
	status = StackPop();
	SET_CONSTANT(1);
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
	
	status = StackPop();
	
	lo = StackPop();
	hi = StackPop();
	
	pc = (hi << 8) | lo;
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
	uint8_t m = Read(src);
	unsigned int tmp = A - m - (IF_CARRY() ? 0 : 1);
	SET_NEGATIVE(tmp & 0x80);
	SET_ZERO(!(tmp & 0xFF));
    SET_OVERFLOW(((A ^ tmp) & 0x80) && ((A ^ m) & 0x80));
	
    if (IF_DECIMAL())
    {
    	if ( ((A & 0x0F) - (IF_CARRY() ? 0 : 1)) < (m & 0x0F)) tmp -= 6;
        if (tmp > 0x99)
        {
        	tmp -= 0x60;
        }
    }
    SET_CARRY(tmp < 0x100);
    A = (tmp & 0xFF);
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


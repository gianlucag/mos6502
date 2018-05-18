#include "mos6502.h"

namespace mos
{
    inline bool status_register::negative() const noexcept
        { return value & bits::negative; }
    inline bool status_register::overflow() const noexcept
        { return value & bits::overflow; }
    inline bool status_register::brk() const noexcept
        { return value & bits::brk; }
    inline bool status_register::decimal() const noexcept
        { return value & bits::decimal; }
    inline bool status_register::interrupt() const noexcept
        { return value & bits::interrupt; }
    inline bool status_register::zero() const noexcept
        { return value & bits::zero; }
    inline bool status_register::carry() const noexcept
        { return value & bits::carry; }

    inline void status_register::negative(bool set) noexcept
        { set ? value |= bits::negative : value &= ~bits::negative; }
    inline void status_register::overflow(bool set) noexcept
        { set ? value |= bits::overflow : value &= ~bits::overflow; }
    inline void status_register::brk(bool set) noexcept
        { set ? value |= bits::brk : value &= ~bits::brk; }
    inline void status_register::decimal(bool set) noexcept
        { set ? value |= bits::decimal : value &= ~bits::decimal; }
    inline void status_register::interrupt(bool set) noexcept
        { set ? value |= bits::interrupt : value &= ~bits::interrupt; }
    inline void status_register::zero(bool set) noexcept
        { set ? value |= bits::zero : value &= ~bits::zero; }
    inline void status_register::carry(bool set) noexcept
        { set ? value |= bits::carry : value &= ~bits::carry; }

    inline auto status_register::get() const noexcept -> std::uint8_t
        { return value; }
    inline void status_register::set(std::uint8_t newValue) noexcept
        { value = newValue | bits::constant; }
    inline void status_register::reset() noexcept
        { value = INITIAL_STATE; }

    inline status_register::operator std::uint8_t() const noexcept
        { return value; }

    template <class SystemBus, class Debugger>
    template <typename... Args>
    inline mos6502<SystemBus, Debugger>::mos6502(Args&& ...args) :
        myBus(std::forward<Args>(args)...)
    {
        // fill jump table with ILLEGALs
        for(int i = 0; i < 256; i++)
        {
            InstrTable[i] = { &mos6502::Addr_IMP, &mos6502::Op_ILLEGAL };
        }

        // insert opcodes

        InstrTable[0x69] = { &mos6502::Addr_IMM,     &mos6502::Op_ADC };
        InstrTable[0x6D] = { &mos6502::Addr_ABS,     &mos6502::Op_ADC };
        InstrTable[0x65] = { &mos6502::Addr_ZER,     &mos6502::Op_ADC };
        InstrTable[0x61] = { &mos6502::Addr_INX,     &mos6502::Op_ADC };
        InstrTable[0x71] = { &mos6502::Addr_INY,     &mos6502::Op_ADC };
        InstrTable[0x75] = { &mos6502::Addr_ZEX,     &mos6502::Op_ADC };
        InstrTable[0x7D] = { &mos6502::Addr_ABX,     &mos6502::Op_ADC };
        InstrTable[0x79] = { &mos6502::Addr_ABY,     &mos6502::Op_ADC };

        InstrTable[0x29] = { &mos6502::Addr_IMM,     &mos6502::Op_AND };
        InstrTable[0x2D] = { &mos6502::Addr_ABS,     &mos6502::Op_AND };
        InstrTable[0x25] = { &mos6502::Addr_ZER,     &mos6502::Op_AND };
        InstrTable[0x21] = { &mos6502::Addr_INX,     &mos6502::Op_AND };
        InstrTable[0x31] = { &mos6502::Addr_INY,     &mos6502::Op_AND };
        InstrTable[0x35] = { &mos6502::Addr_ZEX,     &mos6502::Op_AND };
        InstrTable[0x3D] = { &mos6502::Addr_ABX,     &mos6502::Op_AND };
        InstrTable[0x39] = { &mos6502::Addr_ABY,     &mos6502::Op_AND };

        InstrTable[0x0E] = { &mos6502::Addr_ABS,     &mos6502::Op_ASL };
        InstrTable[0x06] = { &mos6502::Addr_ZER,     &mos6502::Op_ASL };
        InstrTable[0x0A] = { &mos6502::Addr_ACC,     &mos6502::Op_ASL_ACC, 1 };
        InstrTable[0x16] = { &mos6502::Addr_ZEX,     &mos6502::Op_ASL };
        InstrTable[0x1E] = { &mos6502::Addr_ABX_pty, &mos6502::Op_ASL };

        InstrTable[0x90] = { &mos6502::Addr_REL,     &mos6502::Op_BCC };

        InstrTable[0xB0] = { &mos6502::Addr_REL,     &mos6502::Op_BCS };

        InstrTable[0xF0] = { &mos6502::Addr_REL,     &mos6502::Op_BEQ };

        InstrTable[0x2C] = { &mos6502::Addr_ABS,     &mos6502::Op_BIT };
        InstrTable[0x24] = { &mos6502::Addr_ZER,     &mos6502::Op_BIT };

        InstrTable[0x30] = { &mos6502::Addr_REL,     &mos6502::Op_BMI };

        InstrTable[0xD0] = { &mos6502::Addr_REL,     &mos6502::Op_BNE };

        InstrTable[0x10] = { &mos6502::Addr_REL,     &mos6502::Op_BPL };

        InstrTable[0x00] = { &mos6502::Addr_IMP,     &mos6502::Op_BRK,     1 };

        InstrTable[0x50] = { &mos6502::Addr_REL,     &mos6502::Op_BVC };

        InstrTable[0x70] = { &mos6502::Addr_REL,     &mos6502::Op_BVS };

        InstrTable[0x18] = { &mos6502::Addr_IMP,     &mos6502::Op_CLC,     1 };

        InstrTable[0xD8] = { &mos6502::Addr_IMP,     &mos6502::Op_CLD,     1 };

        InstrTable[0x58] = { &mos6502::Addr_IMP,     &mos6502::Op_CLI,     1 };

        InstrTable[0xB8] = { &mos6502::Addr_IMP,     &mos6502::Op_CLV,     1 };

        InstrTable[0xC9] = { &mos6502::Addr_IMM,     &mos6502::Op_CMP };
        InstrTable[0xCD] = { &mos6502::Addr_ABS,     &mos6502::Op_CMP };
        InstrTable[0xC5] = { &mos6502::Addr_ZER,     &mos6502::Op_CMP };
        InstrTable[0xC1] = { &mos6502::Addr_INX,     &mos6502::Op_CMP };
        InstrTable[0xD1] = { &mos6502::Addr_INY,     &mos6502::Op_CMP };
        InstrTable[0xD5] = { &mos6502::Addr_ZEX,     &mos6502::Op_CMP };
        InstrTable[0xDD] = { &mos6502::Addr_ABX,     &mos6502::Op_CMP };
        InstrTable[0xD9] = { &mos6502::Addr_ABY,     &mos6502::Op_CMP };

        InstrTable[0xE0] = { &mos6502::Addr_IMM,     &mos6502::Op_CPX };
        InstrTable[0xEC] = { &mos6502::Addr_ABS,     &mos6502::Op_CPX };
        InstrTable[0xE4] = { &mos6502::Addr_ZER,     &mos6502::Op_CPX };

        InstrTable[0xC0] = { &mos6502::Addr_IMM,     &mos6502::Op_CPY };
        InstrTable[0xCC] = { &mos6502::Addr_ABS,     &mos6502::Op_CPY };
        InstrTable[0xC4] = { &mos6502::Addr_ZER,     &mos6502::Op_CPY };

        InstrTable[0xCE] = { &mos6502::Addr_ABS,     &mos6502::Op_DEC };
        InstrTable[0xC6] = { &mos6502::Addr_ZER,     &mos6502::Op_DEC };
        InstrTable[0xD6] = { &mos6502::Addr_ZEX,     &mos6502::Op_DEC };
        InstrTable[0xDE] = { &mos6502::Addr_ABX_pty, &mos6502::Op_DEC };

        InstrTable[0xCA] = { &mos6502::Addr_IMP,     &mos6502::Op_DEX,     1 };

        InstrTable[0x88] = { &mos6502::Addr_IMP,     &mos6502::Op_DEY,     1 };

        InstrTable[0x49] = { &mos6502::Addr_IMM,     &mos6502::Op_EOR };
        InstrTable[0x4D] = { &mos6502::Addr_ABS,     &mos6502::Op_EOR };
        InstrTable[0x45] = { &mos6502::Addr_ZER,     &mos6502::Op_EOR };
        InstrTable[0x41] = { &mos6502::Addr_INX,     &mos6502::Op_EOR };
        InstrTable[0x51] = { &mos6502::Addr_INY,     &mos6502::Op_EOR };
        InstrTable[0x55] = { &mos6502::Addr_ZEX,     &mos6502::Op_EOR };
        InstrTable[0x5D] = { &mos6502::Addr_ABX,     &mos6502::Op_EOR };
        InstrTable[0x59] = { &mos6502::Addr_ABY,     &mos6502::Op_EOR };

        InstrTable[0xEE] = { &mos6502::Addr_ABS,     &mos6502::Op_INC };
        InstrTable[0xE6] = { &mos6502::Addr_ZER,     &mos6502::Op_INC };
        InstrTable[0xF6] = { &mos6502::Addr_ZEX,     &mos6502::Op_INC };
        InstrTable[0xFE] = { &mos6502::Addr_ABX_pty, &mos6502::Op_INC };

        InstrTable[0xE8] = { &mos6502::Addr_IMP,     &mos6502::Op_INX,     1 };

        InstrTable[0xC8] = { &mos6502::Addr_IMP,     &mos6502::Op_INY,     1 };

        InstrTable[0x4C] = { &mos6502::Addr_ABS,     &mos6502::Op_JMP };
        InstrTable[0x6C] = { &mos6502::Addr_ABI,     &mos6502::Op_JMP };

        InstrTable[0x20] = { &mos6502::Addr_ABS,     &mos6502::Op_JSR };

        InstrTable[0xA9] = { &mos6502::Addr_IMM,     &mos6502::Op_LDA };
        InstrTable[0xAD] = { &mos6502::Addr_ABS,     &mos6502::Op_LDA };
        InstrTable[0xA5] = { &mos6502::Addr_ZER,     &mos6502::Op_LDA };
        InstrTable[0xA1] = { &mos6502::Addr_INX,     &mos6502::Op_LDA };
        InstrTable[0xB1] = { &mos6502::Addr_INY,     &mos6502::Op_LDA };
        InstrTable[0xB5] = { &mos6502::Addr_ZEX,     &mos6502::Op_LDA };
        InstrTable[0xBD] = { &mos6502::Addr_ABX,     &mos6502::Op_LDA };
        InstrTable[0xB9] = { &mos6502::Addr_ABY,     &mos6502::Op_LDA };

        InstrTable[0xA2] = { &mos6502::Addr_IMM,     &mos6502::Op_LDX };
        InstrTable[0xAE] = { &mos6502::Addr_ABS,     &mos6502::Op_LDX };
        InstrTable[0xA6] = { &mos6502::Addr_ZER,     &mos6502::Op_LDX };
        InstrTable[0xBE] = { &mos6502::Addr_ABY,     &mos6502::Op_LDX };
        InstrTable[0xB6] = { &mos6502::Addr_ZEY,     &mos6502::Op_LDX };

        InstrTable[0xA0] = { &mos6502::Addr_IMM,     &mos6502::Op_LDY };
        InstrTable[0xAC] = { &mos6502::Addr_ABS,     &mos6502::Op_LDY };
        InstrTable[0xA4] = { &mos6502::Addr_ZER,     &mos6502::Op_LDY };
        InstrTable[0xB4] = { &mos6502::Addr_ZEX,     &mos6502::Op_LDY };
        InstrTable[0xBC] = { &mos6502::Addr_ABX,     &mos6502::Op_LDY };

        InstrTable[0x4E] = { &mos6502::Addr_ABS,     &mos6502::Op_LSR };
        InstrTable[0x46] = { &mos6502::Addr_ZER,     &mos6502::Op_LSR };
        InstrTable[0x4A] = { &mos6502::Addr_ACC,     &mos6502::Op_LSR_ACC, 1 };
        InstrTable[0x56] = { &mos6502::Addr_ZEX,     &mos6502::Op_LSR };
        InstrTable[0x5E] = { &mos6502::Addr_ABX_pty, &mos6502::Op_LSR };

        InstrTable[0xEA] = { &mos6502::Addr_IMP,     &mos6502::Op_NOP,     1 };

        InstrTable[0x09] = { &mos6502::Addr_IMM,     &mos6502::Op_ORA };
        InstrTable[0x0D] = { &mos6502::Addr_ABS,     &mos6502::Op_ORA };
        InstrTable[0x05] = { &mos6502::Addr_ZER,     &mos6502::Op_ORA };
        InstrTable[0x01] = { &mos6502::Addr_INX,     &mos6502::Op_ORA };
        InstrTable[0x11] = { &mos6502::Addr_INY,     &mos6502::Op_ORA };
        InstrTable[0x15] = { &mos6502::Addr_ZEX,     &mos6502::Op_ORA };
        InstrTable[0x1D] = { &mos6502::Addr_ABX,     &mos6502::Op_ORA };
        InstrTable[0x19] = { &mos6502::Addr_ABY,     &mos6502::Op_ORA };

        InstrTable[0x48] = { &mos6502::Addr_IMP,     &mos6502::Op_PHA,     1 };

        InstrTable[0x08] = { &mos6502::Addr_IMP,     &mos6502::Op_PHP,     1 };

        InstrTable[0x68] = { &mos6502::Addr_IMP,     &mos6502::Op_PLA,     2 };

        InstrTable[0x28] = { &mos6502::Addr_IMP,     &mos6502::Op_PLP,     2 };

        InstrTable[0x2E] = { &mos6502::Addr_ABS,     &mos6502::Op_ROL };
        InstrTable[0x26] = { &mos6502::Addr_ZER,     &mos6502::Op_ROL };
        InstrTable[0x2A] = { &mos6502::Addr_ACC,     &mos6502::Op_ROL_ACC, 1 };
        InstrTable[0x36] = { &mos6502::Addr_ZEX,     &mos6502::Op_ROL };
        InstrTable[0x3E] = { &mos6502::Addr_ABX_pty, &mos6502::Op_ROL };

        InstrTable[0x6E] = { &mos6502::Addr_ABS,     &mos6502::Op_ROR };
        InstrTable[0x66] = { &mos6502::Addr_ZER,     &mos6502::Op_ROR };
        InstrTable[0x6A] = { &mos6502::Addr_ACC,     &mos6502::Op_ROR_ACC, 1 };
        InstrTable[0x76] = { &mos6502::Addr_ZEX,     &mos6502::Op_ROR };
        InstrTable[0x7E] = { &mos6502::Addr_ABX_pty, &mos6502::Op_ROR };

        InstrTable[0x40] = { &mos6502::Addr_IMP,     &mos6502::Op_RTI,     2 };

        InstrTable[0x60] = { &mos6502::Addr_IMP,     &mos6502::Op_RTS };

        InstrTable[0xE9] = { &mos6502::Addr_IMM,     &mos6502::Op_SBC };
        InstrTable[0xED] = { &mos6502::Addr_ABS,     &mos6502::Op_SBC };
        InstrTable[0xE5] = { &mos6502::Addr_ZER,     &mos6502::Op_SBC };
        InstrTable[0xE1] = { &mos6502::Addr_INX,     &mos6502::Op_SBC };
        InstrTable[0xF1] = { &mos6502::Addr_INY,     &mos6502::Op_SBC };
        InstrTable[0xF5] = { &mos6502::Addr_ZEX,     &mos6502::Op_SBC };
        InstrTable[0xFD] = { &mos6502::Addr_ABX,     &mos6502::Op_SBC };
        InstrTable[0xF9] = { &mos6502::Addr_ABY,     &mos6502::Op_SBC };

        InstrTable[0x38] = { &mos6502::Addr_IMP,     &mos6502::Op_SEC,     1 };

        InstrTable[0xF8] = { &mos6502::Addr_IMP,     &mos6502::Op_SED,     1 };

        InstrTable[0x78] = { &mos6502::Addr_IMP,     &mos6502::Op_SEI,     1 };

        InstrTable[0x8D] = { &mos6502::Addr_ABS,     &mos6502::Op_STA };
        InstrTable[0x85] = { &mos6502::Addr_ZER,     &mos6502::Op_STA };
        InstrTable[0x81] = { &mos6502::Addr_INX,     &mos6502::Op_STA };
        InstrTable[0x91] = { &mos6502::Addr_INY_pty, &mos6502::Op_STA };
        InstrTable[0x95] = { &mos6502::Addr_ZEX,     &mos6502::Op_STA };
        InstrTable[0x9D] = { &mos6502::Addr_ABX_pty, &mos6502::Op_STA };
        InstrTable[0x99] = { &mos6502::Addr_ABY_pty, &mos6502::Op_STA };

        InstrTable[0x8E] = { &mos6502::Addr_ABS,     &mos6502::Op_STX };
        InstrTable[0x86] = { &mos6502::Addr_ZER,     &mos6502::Op_STX };
        InstrTable[0x96] = { &mos6502::Addr_ZEY,     &mos6502::Op_STX };

        InstrTable[0x8C] = { &mos6502::Addr_ABS,     &mos6502::Op_STY };
        InstrTable[0x84] = { &mos6502::Addr_ZER,     &mos6502::Op_STY };
        InstrTable[0x94] = { &mos6502::Addr_ZEX,     &mos6502::Op_STY };

        InstrTable[0xAA] = { &mos6502::Addr_IMP,     &mos6502::Op_TAX,     1 };

        InstrTable[0xA8] = { &mos6502::Addr_IMP,     &mos6502::Op_TAY,     1 };

        InstrTable[0xBA] = { &mos6502::Addr_IMP,     &mos6502::Op_TSX,     1 };

        InstrTable[0x8A] = { &mos6502::Addr_IMP,     &mos6502::Op_TXA,     1 };

        InstrTable[0x9A] = { &mos6502::Addr_IMP,     &mos6502::Op_TXS,     1 };

        InstrTable[0x98] = { &mos6502::Addr_IMP,     &mos6502::Op_TYA,     1 };

        return;
    }

    template <class SystemBus, class Debugger>
    inline uint16_t mos6502<SystemBus, Debugger>::Addr_ACC()
    {
        return 0; // not used
    }

    template <class SystemBus, class Debugger>
    inline uint16_t mos6502<SystemBus, Debugger>::Addr_IMM()
    {
        return PC++;
    }

    template <class SystemBus, class Debugger>
    inline uint16_t mos6502<SystemBus, Debugger>::Addr_ABS()
    {
        return fetch(PC++) + (fetch(PC++) << 8);
    }

    template <class SystemBus, class Debugger>
    inline uint16_t mos6502<SystemBus, Debugger>::Addr_ZER()
    {
        return fetch(PC++);
    }

    template <class SystemBus, class Debugger>
    inline uint16_t mos6502<SystemBus, Debugger>::Addr_IMP()
    {
        return 0; // not used
    }

    template <class SystemBus, class Debugger>
    inline uint16_t mos6502<SystemBus, Debugger>::Addr_REL()
    {
        std::int16_t offset;
        std::uint16_t addr;

        offset = static_cast<std::int8_t>(fetch(PC++));
        addr = PC + offset;
        return addr;
    }

    template <class SystemBus, class Debugger>
    inline uint16_t mos6502<SystemBus, Debugger>::Addr_ABI()
    {
        std::uint16_t addrL;
        std::uint16_t addrH;
        std::uint16_t effL;
        std::uint16_t effH;
        std::uint16_t abs;
        std::uint16_t addr;

        addrL = fetch(PC++);
        addrH = fetch(PC++);

        abs = (addrH << 8) | addrL;

        effL = fetch(abs);
        effH = fetch((abs & 0xFF00) + ((abs + 1) & 0x00FF) );

        addr = effL + 0x100 * effH;

        return addr;
    }

    template <class SystemBus, class Debugger>
    inline uint16_t mos6502<SystemBus, Debugger>::Addr_ZEX()
    {
        ++myCycles; // Zero-page indexed incurs an extra cycle.
        return (fetch(PC++) + X) & 0xFF;
    }

    template <class SystemBus, class Debugger>
    inline uint16_t mos6502<SystemBus, Debugger>::Addr_ZEY()
    {
        ++myCycles; // Zero-page indexed incurs an extra cycle.
        return (fetch(PC++) + Y) & 0xFF;
    }

    template <class SystemBus, class Debugger>
    inline uint16_t mos6502<SystemBus, Debugger>::Addr_ABX()
    {
        std::uint16_t addr;
        std::uint16_t addrL;
        std::uint16_t addrH;

        addrL = fetch(PC++);
        addrH = fetch(PC++) << 8;

        addr = addrL + addrH + X;
        if ((addr & 0xFF00) != (addrH & 0xFF00)) ++myCycles; // There is a penalty if crossing the
                                                             // page boundary.
        return addr;
    }

    template <class SystemBus, class Debugger>
    inline uint16_t mos6502<SystemBus, Debugger>::Addr_ABY()
    {
        std::uint16_t addr;
        std::uint16_t addrL;
        std::uint16_t addrH;

        addrL = fetch(PC++);
        addrH = fetch(PC++) << 8;

        addr = addrL + addrH + Y;
        if ((addr & 0xFF00) != (addrH & 0xFF00)) ++myCycles; // There is a penalty if crossing the
                                                             // page boundary.
        return addr;
    }


    template <class SystemBus, class Debugger>
    inline uint16_t mos6502<SystemBus, Debugger>::Addr_INX()
    {
        std::uint16_t zeroL;
        std::uint16_t zeroH;
        std::uint16_t addr;

        zeroL = (fetch(PC++) + X) & 0xFF;
        zeroH = (zeroL + 1) & 0xFF;
        addr = fetch(zeroL) + (fetch(zeroH) << 8);
        ++myCycles; // Indexed Indirect always incurs and extra cycle.

        return addr;
    }

    template <class SystemBus, class Debugger>
    inline uint16_t mos6502<SystemBus, Debugger>::Addr_INY()
    {
        std::uint16_t zeroL;
        std::uint16_t zeroH;
        std::uint16_t addr;
        std::uint16_t addrH;

        zeroL = fetch(PC++);
        zeroH = (zeroL + 1) & 0xFF;
        addrH = (fetch(zeroH) << 8);
        addr = fetch(zeroL) + addrH + Y;
        if ((addr & 0xFF00) != (addrH & 0xFF00)) ++myCycles; // There is a penalty if crossing the
                                                             // page boundary.

        return addr;
    }

    template <class SystemBus, class Debugger>
    inline uint16_t mos6502<SystemBus, Debugger>::Addr_ABX_pty()
    {
        ++myCycles;
        return fetch(PC++) + (fetch(PC++) << 8) + X;
    }

    template <class SystemBus, class Debugger>
    inline uint16_t mos6502<SystemBus, Debugger>::Addr_ABY_pty()
    {
        ++myCycles;
        return fetch(PC++) + (fetch(PC++) << 8) + Y;
    }

    template <class SystemBus, class Debugger>
    inline uint16_t mos6502<SystemBus, Debugger>::Addr_INY_pty()
    {
        std::uint16_t zeroL;
        std::uint16_t zeroH;
        std::uint16_t addr;

        zeroL = fetch(PC++);
        zeroH = (zeroL + 1) & 0xFF;
        addr = fetch(zeroL) + (fetch(zeroH) << 8) + Y;
        ++myCycles;

        return addr;
    }

    template <class SystemBus, class Debugger>
    inline auto mos6502<SystemBus, Debugger>::bus() noexcept -> system_bus<SystemBus>&
    {
        return static_cast<system_bus<SystemBus>&>(myBus);
    }

    template<class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::power_off() noexcept
    {
        isPendingPowerOff = true;
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::reset()
    {
        // Reset the counters.
        mySteps = 0;
        myCycles = 4;   // According to the datasheet, the reset routine takes 6 clock cycles. More
                        // will come.

        // Initialize the registers.
        A = 0x00;
        Y = 0x00;
        X = 0x00;
        SP = 0xFD;
        PC = (fetch(rstVectorH) << 8) + fetch(rstVectorL); // Load PC from reset vector.
        myStatus.reset();

        // Reset the fatal interrupts.
        hasEncounteredIllegalOpcode = false;
        isPendingPowerOff = false;
        return;
    }

    //template <class SystemBus, class Debugger>
    //inline void mos6502<SystemBus, Debugger>::write(uint16_t address, uint8_t data) noexcept
    //{
    //    myBus.write(address, data);
    //}

    //template <class SystemBus, class Debugger>
    //inline auto mos6502<SystemBus, Debugger>::read(uint16_t address) noexcept -> uint8_t
    //{
    //    return myBus.read(address);
    //}

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::run()
    {
        while (step());
    }

    template <class SystemBus, class Debugger>
    inline bool mos6502<SystemBus, Debugger>::step()
    {
        if (isPendingPowerOff)
        {
            // As soon as the power is lost, stop!
            return false;
        }
        else if (myBus.pending_nmi())
        {
            // TODO: How many cycles?
            service_nmi();
        }
        else if (myBus.pending_irq() && !myStatus.interrupt())
        {
            // TODO: How many cycles?
            service_irq();
        }
        /*always*/
        {
            // Fetch, decode, and execute.
            std::uint8_t opcode = fetch(PC++);
            exec(InstrTable[opcode]);
        }

        ++mySteps;

        // TODO: Should we break after the processor calls BRK? Maybe...
        // TODO: Break on Debugger::hit_breakpoint(pc);
        return !hasEncounteredIllegalOpcode;
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::push(std::uint8_t byte)
    {
        myBus.write(0x0100 + SP--, byte);
    }

    template<class SystemBus, class Debugger>
    inline auto mos6502<SystemBus, Debugger>::pop() -> std::uint8_t
    {
        return myBus.read(0x0100 + ++SP);
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::push_word(std::uint16_t word)
    {
        push(static_cast<std::uint8_t>(word >> 8));
        push(static_cast<std::uint8_t>(word));
    }

    template<class SystemBus, class Debugger>
    inline auto mos6502<SystemBus, Debugger>::pop_word() -> std::uint16_t
    {
        return pop() + static_cast<std::uint16_t>(pop() << 8);
    }

    template <class SystemBus, class Debugger>
    inline auto mos6502<SystemBus, Debugger>::fetch(std::uint16_t address) noexcept -> std::uint8_t
    {
        ++myCycles;
        return myBus.read(address);
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::store(std::uint16_t address, std::uint8_t data) noexcept
    {
        myBus.write(address, data);
        ++myCycles;
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::stack_push(std::uint8_t byte)
    {
        store(0x0100 + SP--, byte);
    }

    template <class SystemBus, class Debugger>
    inline auto mos6502<SystemBus, Debugger>::stack_pop() -> std::uint8_t
    {
        return fetch(0x0100 + ++SP);
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::exec(instruction_table_entry instruction)
    {
        uint16_t src = (this->*instruction.fetcher)();
        (this->*instruction.handler)(src);
        myCycles += instruction.cycle_adjustment;
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::service_nmi()
    {
        myStatus.brk(false);
        stack_push((PC >> 8) & 0xFF);
        stack_push(PC & 0xFF);
        stack_push(myStatus);
        myStatus.interrupt(true);
        PC = (fetch(nmiVectorH) << 8) + fetch(nmiVectorL);
        myCycles += 2; // We need to make sure this accounts for 7 cycles. 5 have already passed.
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::service_irq()
    {
        myStatus.brk(false);
        stack_push((PC >> 8) & 0xFF);
        stack_push(PC & 0xFF);
        stack_push(myStatus);
        myStatus.interrupt(true);
        PC = (fetch(irqVectorH) << 8) + fetch(irqVectorL);
        myCycles += 2; // We need to make sure this accounts for 7 cycles. 5 have already passed.
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_ILLEGAL(std::uint16_t src)
    {
        hasEncounteredIllegalOpcode = true;
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_ADC(std::uint16_t src)
    {
        std::uint8_t m = fetch(src);
        unsigned int tmp = m + A + (myStatus.carry() ? 1 : 0);
        myStatus.zero(!(tmp & 0xFF));
        if (myStatus.decimal())
        {
            if (((A & 0xF) + (m & 0xF) + (myStatus.carry() ? 1 : 0)) > 9) tmp += 6;
            myStatus.negative(tmp & 0x80);
            myStatus.overflow(!((A ^ m) & 0x80) && ((A ^ tmp) & 0x80));
            if (tmp > 0x99)
            {
                tmp += 96;
            }
            myStatus.carry(tmp > 0x99);
        }
        else
        {
            myStatus.negative(tmp & 0x80);
            myStatus.overflow(!((A ^ m) & 0x80) && ((A ^ tmp) & 0x80));
            myStatus.carry(tmp > 0xFF);
        }

        A = tmp & 0xFF;
    }



    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_AND(std::uint16_t src)
    {
        std::uint8_t m = fetch(src);
        std::uint8_t res = m & A;
        myStatus.negative(res & 0x80);
        myStatus.zero(!res);
        A = res;
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_ASL(std::uint16_t src)
    {
        std::uint8_t m = fetch(src);
        store(src, m);  // The 6502 stray write cycle.
        myStatus.carry(m & 0x80);
        m <<= 1;
        myStatus.negative(m & 0x80);
        myStatus.zero(!m);
        store(src, m);
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_ASL_ACC(std::uint16_t src)
    {
        std::uint8_t m = A;
        myStatus.carry(m & 0x80);
        m <<= 1;
        myStatus.negative(m & 0x80);
        myStatus.zero(!m);
        A = m;
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_BCC(std::uint16_t src)
    {
        if (!myStatus.carry())
        {
            if ((PC & 0xFF00) != (src & 0xFF00)) ++myCycles;
            fetch(PC); // stray fetch
            PC = src;
        }
    }


    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_BCS(std::uint16_t src)
    {
        if (myStatus.carry())
        {
            if ((PC & 0xFF00) != (src & 0xFF00)) ++myCycles;
            fetch(PC); // stray fetch
            PC = src;
        }
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_BEQ(std::uint16_t src)
    {
        if (myStatus.zero())
        {
            if ((PC & 0xFF00) != (src & 0xFF00)) ++myCycles;
            fetch(PC); // stray fetch
            PC = src;
        }
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_BIT(std::uint16_t src)
    {
        std::uint8_t m = fetch(src);
        std::uint8_t res = m & A;
        myStatus.negative(res & 0x80);
        myStatus.set((myStatus & 0x3F) | (uint8_t)(m & 0xC0));
        myStatus.zero(!res);
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_BMI(std::uint16_t src)
    {
        if (myStatus.negative())
        {
            if ((PC & 0xFF00) != (src & 0xFF00)) ++myCycles;
            fetch(PC); // stray fetch
            PC = src;
        }
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_BNE(std::uint16_t src)
    {
        if (!myStatus.zero())
        {
            if ((PC & 0xFF00) != (src & 0xFF00)) ++myCycles;
            fetch(PC); // stray fetch
            PC = src;
        }
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_BPL(std::uint16_t src)
    {
        if (!myStatus.negative())
        {
            if ((PC & 0xFF00) != (src & 0xFF00)) ++myCycles;
            fetch(PC); // stray fetch
            PC = src;
        }
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_BRK(std::uint16_t src)
    {
        PC++;
        stack_push((PC >> 8) & 0xFF);
        stack_push(PC & 0xFF);
        stack_push(myStatus | status_register::bits::brk);
        myStatus.interrupt(true);
        // TODO: If WD65C02, myStatus.decimal(false)
        PC = (fetch(irqVectorH) << 8) + fetch(irqVectorL);
        return;
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_BVC(std::uint16_t src)
    {
        if (!myStatus.overflow())
        {
            if ((PC & 0xFF00) != (src & 0xFF00)) ++myCycles;
            fetch(PC); // stray fetch
            PC = src;
        }
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_BVS(std::uint16_t src)
    {
        if (myStatus.overflow())
        {
            if ((PC & 0xFF00) != (src & 0xFF00)) ++myCycles;
            fetch(PC); // stray fetch
            PC = src;
        }
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_CLC(std::uint16_t src)
    {
        myStatus.carry(true);
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_CLD(std::uint16_t src)
    {
        myStatus.decimal(true);
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_CLI(std::uint16_t src)
    {
        myStatus.interrupt(true);
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_CLV(std::uint16_t src)
    {
        myStatus.overflow(true);
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_CMP(std::uint16_t src)
    {
        unsigned int tmp = A - fetch(src);
        myStatus.carry(tmp < 0x100);
        myStatus.negative(tmp & 0x80);
        myStatus.zero(!(tmp & 0xFF));
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_CPX(std::uint16_t src)
    {
        unsigned int tmp = X - fetch(src);
        myStatus.carry(tmp < 0x100);
        myStatus.negative(tmp & 0x80);
        myStatus.zero(!(tmp & 0xFF));
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_CPY(std::uint16_t src)
    {
        unsigned int tmp = Y - fetch(src);
        myStatus.carry(tmp < 0x100);
        myStatus.negative(tmp & 0x80);
        myStatus.zero(!(tmp & 0xFF));
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_DEC(std::uint16_t src)
    {
        std::uint8_t m = fetch(src);
        store(src, m);  // The 6502 stray write cycle.
        m = (m - 1) % 256;
        myStatus.negative(m & 0x80);
        myStatus.zero(!m);
        store(src, m);
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_DEX(std::uint16_t src)
    {
        std::uint8_t m = X;
        m = (m - 1) % 256;
        myStatus.negative(m & 0x80);
        myStatus.zero(!m);
        X = m;
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_DEY(std::uint16_t src)
    {
        std::uint8_t m = Y;
        m = (m - 1) % 256;
        myStatus.negative(m & 0x80);
        myStatus.zero(!m);
        Y = m;
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_EOR(std::uint16_t src)
    {
        std::uint8_t m = fetch(src);
        m = A ^ m;
        myStatus.negative(m & 0x80);
        myStatus.zero(!m);
        A = m;
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_INC(std::uint16_t src)
    {
        std::uint8_t m = fetch(src);
        store(src, m);  // The 6502 stray write cycle.
        m = (m + 1) % 256;
        myStatus.negative(m & 0x80);
        myStatus.zero(!m);
        store(src, m);
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_INX(std::uint16_t src)
    {
        std::uint8_t m = X;
        m = (m + 1) % 256;
        myStatus.negative(m & 0x80);
        myStatus.zero(!m);
        X = m;
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_INY(std::uint16_t src)
    {
        std::uint8_t m = Y;
        m = (m + 1) % 256;
        myStatus.negative(m & 0x80);
        myStatus.zero(!m);
        Y = m;
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_JMP(std::uint16_t src)
    {
        PC = src;
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_JSR(std::uint16_t src)
    {
        PC--;
        stack_push(static_cast<std::uint8_t>(PC >> 8));
        stack_push(static_cast<std::uint8_t>(PC));
        fetch(PC = src); // 6502 stray fetch
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_LDA(std::uint16_t src)
    {
        std::uint8_t m = fetch(src);
        myStatus.negative(m & 0x80);
        myStatus.zero(!m);
        A = m;
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_LDX(std::uint16_t src)
    {
        std::uint8_t m = fetch(src);
        myStatus.negative(m & 0x80);
        myStatus.zero(!m);
        X = m;
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_LDY(std::uint16_t src)
    {
        std::uint8_t m = fetch(src);
        myStatus.negative(m & 0x80);
        myStatus.zero(!m);
        Y = m;
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_LSR(std::uint16_t src)
    {
        std::uint8_t m = fetch(src);
        store(src, m);  // The 6502 stray write cycle.
        myStatus.carry(m & 0x01);
        m >>= 1;
        myStatus.negative(false);
        myStatus.zero(!m);
        store(src, m);
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_LSR_ACC(std::uint16_t src)
    {
        std::uint8_t m = A;
        myStatus.carry(m & 0x01);
        m >>= 1;
        myStatus.negative(false);
        myStatus.zero(!m);
        A = m;
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_NOP(std::uint16_t src)
    {
        return; // Literally, no operation.
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_ORA(std::uint16_t src)
    {
        std::uint8_t m = fetch(src);
        m = A | m;
        myStatus.negative(m & 0x80);
        myStatus.zero(!m);
        A = m;
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_PHA(std::uint16_t src)
    {
        stack_push(A);
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_PHP(std::uint16_t src)
    {
        stack_push(myStatus | status_register::bits::brk);
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_PLA(std::uint16_t src)
    {
        A = stack_pop();
        myStatus.negative(A & 0x80);
        myStatus.zero(!A);
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_PLP(std::uint16_t src)
    {
        myStatus.set(stack_pop());
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_ROL(std::uint16_t src)
    {
        std::uint16_t m = fetch(src);
        store(src, static_cast<std::uint8_t>(m));  // The 6502 stray write cycle.
        m <<= 1;
        if (myStatus.carry()) m |= 0x01;
        myStatus.carry(m > 0xFF);
        m &= 0xFF;
        myStatus.negative(m & 0x80);
        myStatus.zero(!m);
        store(src, static_cast<std::uint8_t>(m));
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_ROL_ACC(std::uint16_t src)
    {
        std::uint16_t m = A;
        m <<= 1;
        if (myStatus.carry()) m |= 0x01;
        myStatus.carry(m > 0xFF);
        m &= 0xFF;
        myStatus.negative(m & 0x80);
        myStatus.zero(!m);
        A = static_cast<std::uint8_t>(m);
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_ROR(std::uint16_t src)
    {
        std::uint16_t m = fetch(src);
        store(src, static_cast<std::uint8_t>(m));  // The 6502 stray write cycle.
        if (myStatus.carry()) m |= 0x100;
        myStatus.carry(m & 0x01);
        m >>= 1;
        m &= 0xFF;
        myStatus.negative(m & 0x80);
        myStatus.zero(!m);
        store(src, static_cast<std::uint8_t>(m));
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_ROR_ACC(std::uint16_t src)
    {
        std::uint16_t m = A;
        if (myStatus.carry()) m |= 0x100;
        myStatus.carry(m & 0x01);
        m >>= 1;
        m &= 0xFF;
        myStatus.negative(m & 0x80);
        myStatus.zero(!m);
        A = static_cast<std::uint8_t>(m);
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_RTI(std::uint16_t src)
    {
        std::uint8_t lo, hi;

        myStatus.set(stack_pop());

        lo = stack_pop();
        hi = stack_pop();

        PC = (hi << 8) | lo;
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_RTS(std::uint16_t src)
    {
        std::uint8_t lo, hi;

        lo = stack_pop();
        hi = stack_pop();

        PC = ((hi << 8) | lo) + 1;
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_SBC(std::uint16_t src)
    {
        std::uint8_t m = fetch(src);
        unsigned int tmp = A - m - (myStatus.carry() ? 0 : 1);
        myStatus.negative(tmp & 0x80);
        myStatus.zero(!(tmp & 0xFF));
        myStatus.overflow(((A ^ tmp) & 0x80) && ((A ^ m) & 0x80));

        if (myStatus.decimal())
        {
            if ( ((A & 0x0F) - (myStatus.carry() ? 0 : 1)) < (m & 0x0F)) tmp -= 6;
            if (tmp > 0x99)
            {
                tmp -= 0x60;
            }
        }
        myStatus.carry(tmp < 0x100);
        A = (tmp & 0xFF);
        return;
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_SEC(std::uint16_t src)
    {
        myStatus.carry(true);
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_SED(std::uint16_t src)
    {
        myStatus.decimal(true);
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_SEI(std::uint16_t src)
    {
        myStatus.interrupt(true);
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_STA(std::uint16_t src)
    {
        store(src, A);
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_STX(std::uint16_t src)
    {
        store(src, X);
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_STY(std::uint16_t src)
    {
        store(src, Y);
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_TAX(std::uint16_t src)
    {
        std::uint8_t m = A;
        myStatus.negative(m & 0x80);
        myStatus.zero(!m);
        X = m;
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_TAY(std::uint16_t src)
    {
        std::uint8_t m = A;
        myStatus.negative(m & 0x80);
        myStatus.zero(!m);
        Y = m;
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_TSX(std::uint16_t src)
    {
        std::uint8_t m = SP;
        myStatus.negative(m & 0x80);
        myStatus.zero(!m);
        X = m;
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_TXA(std::uint16_t src)
    {
        std::uint8_t m = X;
        myStatus.negative(m & 0x80);
        myStatus.zero(!m);
        A = m;
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_TXS(std::uint16_t src)
    {
        SP = X;
    }

    template <class SystemBus, class Debugger>
    inline void mos6502<SystemBus, Debugger>::Op_TYA(std::uint16_t src)
    {
        std::uint8_t m = Y;
        myStatus.negative(m & 0x80);
        myStatus.zero(!m);
        A = m;
    }
}

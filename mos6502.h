/*
===============================================================================
Name        : mos6502 
Author      : Gianluca Ghettini
Modified by : Matthew Holder
Version     : 1.0
Copyright   : 
Description : A MOS 6502 CPU emulator written in C++
===============================================================================
TODO:
    - WD6502 additions.
    - Possible check against real hardware for reset state.
    - Create structure the define default reset or other behaviors between
      CPU variants.
*/
#pragma once

// Required libraries.
#include <cstdint>
#include <utility>

// Concepts from this library.
#include "mos_system_bus.h"

namespace mos
{
    // TODO: Give Debugger concept its own module with this implementation.
    class null_debugger
    {
    };

    class status_register
    {
    public:
        struct bits
        {
            static const std::uint8_t negative  = 0x80;
            static const std::uint8_t overflow  = 0x40;
            static const std::uint8_t constant  = 0x20;
            static const std::uint8_t brk       = 0x10;
            static const std::uint8_t decimal   = 0x08;
            static const std::uint8_t interrupt = 0x04;
            static const std::uint8_t zero      = 0x02;
            static const std::uint8_t carry     = 0x01;
        };

        status_register() noexcept = default;

        inline bool negative() const noexcept;
        inline bool overflow() const noexcept;
        inline bool brk() const noexcept;
        inline bool decimal() const noexcept;
        inline bool interrupt() const noexcept;
        inline bool zero() const noexcept;
        inline bool carry() const noexcept;

        inline void negative(bool set) noexcept;
        inline void overflow(bool set) noexcept;
        inline void brk(bool set) noexcept;
        inline void decimal(bool set) noexcept;
        inline void interrupt(bool set) noexcept;
        inline void zero(bool set) noexcept;
        inline void carry(bool set) noexcept;

        inline auto get() const noexcept -> std::uint8_t;
        inline void set(std::uint8_t newValue) noexcept;
        inline void reset() noexcept;

        inline operator std::uint8_t() const noexcept;

    private:
        static const std::uint8_t INITIAL_STATE = bits::constant;

        std::uint8_t value;
    };

    template <class SystemBus, class Debugger = null_debugger>
    class mos6502
    {
    public:
        using my_type = mos6502<SystemBus, Debugger>;

        template <typename... Args>
        explicit mos6502(Args&& ...args);

        // Expose the system bus.
        auto bus() noexcept -> system_bus<SystemBus>&;

        // Expose the public CPU state.
        auto a() const noexcept -> std::uint8_t     { return A; }
        auto x() const noexcept -> std::uint8_t     { return X; }
        auto y() const noexcept -> std::uint8_t     { return Y; }
        auto sp() const noexcept -> std::uint8_t    { return SP; }
        auto pc() const noexcept -> std::uint16_t   { return PC; }
        // Allow debugger to alter the CPU state.
        void a(std::uint8_t value) noexcept         { A = value; }
        void x(std::uint8_t value) noexcept         { X = value; }
        void y(std::uint8_t value) noexcept         { Y = value; }
        void go(std::uint16_t address) noexcept     { PC = address; }
        // Expose the processor status and allow the debugger to modify it.
        auto status() noexcept -> status_register& { return myStatus; }

        // Expose the emulator counters.
        std::size_t cycles() const noexcept         { return myCycles; }
        std::size_t steps() const noexcept          { return mySteps; }

        // Machine operations.
        void power_off() noexcept;
        void reset();
        void run();
        bool step();

        // Debugging and testing stack operations.
        void push(std::uint8_t byte);
        auto pop() -> std::uint8_t;
        void push_word(std::uint16_t word);
        auto pop_word() -> std::uint16_t;

    private:

        struct instruction_table_entry
        {
            using fetcher_type = auto (my_type::*)()/* noexcept*/ -> std::uint16_t;
            using handler_type = void (my_type::*)(std::uint16_t)/* noexcept*/;

            fetcher_type fetcher;
            handler_type handler;
            int cycle_adjustment = 0;
        };

        // General purpose registers.
        uint8_t A; // Accumulator

        // Index registers.
        uint8_t X; // X-index
        uint8_t Y; // Y-index
    
        // Stack pointer.
        uint8_t SP;
    
        // Program counter.
        uint16_t PC;
    
        // Status register.
        status_register myStatus;

        // Executed instructions / steps.
        std::size_t mySteps = 0;
    
        // Consumed clock cycles.
        std::size_t myCycles = 0;

        // System bus.
        SystemBus myBus;

        // Fatal interrupts.
        bool hasEncounteredIllegalOpcode = false;
        bool isPendingPowerOff = false;

        // Instruction decoder.
        instruction_table_entry InstrTable[256];

        // addressing modes
        uint16_t Addr_ACC(); // ACCUMULATOR
        uint16_t Addr_IMM(); // IMMEDIATE
        uint16_t Addr_ABS(); // ABSOLUTE
        uint16_t Addr_ZER(); // ZERO PAGE
        uint16_t Addr_ZEX(); // INDEXED-X ZERO PAGE
        uint16_t Addr_ZEY(); // INDEXED-Y ZERO PAGE
        uint16_t Addr_ABX(); // INDEXED-X ABSOLUTE
        uint16_t Addr_ABY(); // INDEXED-Y ABSOLUTE
        uint16_t Addr_IMP(); // IMPLIED
        uint16_t Addr_REL(); // RELATIVE
        uint16_t Addr_INX(); // INDEXED-X INDIRECT
        uint16_t Addr_INY(); // INDEXED-Y INDIRECT
        uint16_t Addr_ABI(); // ABSOLUTE INDIRECT

        // Special case addressing modes
        uint16_t Addr_ABX_pty(); // INDEXED-X ABSOLUTE, always incur page boundary penalty.
        uint16_t Addr_ABY_pty(); // INDEXED-Y ABSOLUTE, always incur page boundary penalty.
        uint16_t Addr_INY_pty(); // INDEXED-Y INDIRECT, always incur page boundary penalty.

        // opcodes (grouped as per datasheet)
        void Op_ADC(std::uint16_t src);
        void Op_AND(std::uint16_t src);
        void Op_ASL(std::uint16_t src); void Op_ASL_ACC(std::uint16_t src);
        void Op_BCC(std::uint16_t src);
        void Op_BCS(std::uint16_t src);
    
        void Op_BEQ(std::uint16_t src);
        void Op_BIT(std::uint16_t src);
        void Op_BMI(std::uint16_t src);
        void Op_BNE(std::uint16_t src);
        void Op_BPL(std::uint16_t src);
    
        void Op_BRK(std::uint16_t src);
        void Op_BVC(std::uint16_t src);
        void Op_BVS(std::uint16_t src);
        void Op_CLC(std::uint16_t src);
        void Op_CLD(std::uint16_t src);
    
        void Op_CLI(std::uint16_t src);
        void Op_CLV(std::uint16_t src);
        void Op_CMP(std::uint16_t src);
        void Op_CPX(std::uint16_t src);
        void Op_CPY(std::uint16_t src);
    
        void Op_DEC(std::uint16_t src);
        void Op_DEX(std::uint16_t src);
        void Op_DEY(std::uint16_t src);
        void Op_EOR(std::uint16_t src);
        void Op_INC(std::uint16_t src);
    
        void Op_INX(std::uint16_t src);
        void Op_INY(std::uint16_t src);
        void Op_JMP(std::uint16_t src);
        void Op_JSR(std::uint16_t src);
        void Op_LDA(std::uint16_t src);
    
        void Op_LDX(std::uint16_t src);
        void Op_LDY(std::uint16_t src);
        void Op_LSR(std::uint16_t src); void Op_LSR_ACC(std::uint16_t src);
        void Op_NOP(std::uint16_t src);
        void Op_ORA(std::uint16_t src);
    
        void Op_PHA(std::uint16_t src);
        void Op_PHP(std::uint16_t src);
        void Op_PLA(std::uint16_t src);
        void Op_PLP(std::uint16_t src);
        void Op_ROL(std::uint16_t src); void Op_ROL_ACC(std::uint16_t src);
    
        void Op_ROR(std::uint16_t src); void Op_ROR_ACC(std::uint16_t src);
        void Op_RTI(std::uint16_t src);
        void Op_RTS(std::uint16_t src);
        void Op_SBC(std::uint16_t src);
        void Op_SEC(std::uint16_t src);
        void Op_SED(std::uint16_t src);
    
        void Op_SEI(std::uint16_t src);
        void Op_STA(std::uint16_t src);
        void Op_STX(std::uint16_t src);
        void Op_STY(std::uint16_t src);
        void Op_TAX(std::uint16_t src);
    
        void Op_TAY(std::uint16_t src);
        void Op_TSX(std::uint16_t src);
        void Op_TXA(std::uint16_t src);
        void Op_TXS(std::uint16_t src);
        void Op_TYA(std::uint16_t src);
    
        void Op_ILLEGAL(std::uint16_t src);
    
        // IRQ, reset, NMI vectors
        static const std::uint16_t irqVectorH = 0xFFFF;
        static const std::uint16_t irqVectorL = 0xFFFE;
        static const std::uint16_t rstVectorH = 0xFFFD;
        static const std::uint16_t rstVectorL = 0xFFFC;
        static const std::uint16_t nmiVectorH = 0xFFFB;
        static const std::uint16_t nmiVectorL = 0xFFFA;

        // Fetches and stores information from and to the bus.
        auto fetch(std::uint16_t address) noexcept -> std::uint8_t;
        void store(std::uint16_t address, std::uint8_t data) noexcept;

        // Stack operations
        void stack_push(std::uint8_t byte);
        auto stack_pop() -> std::uint8_t;

        void exec(instruction_table_entry i);
        void service_nmi();
        void service_irq();
    };
}

#include "mos6502.inl.h"

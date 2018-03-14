/*
===============================================================================
Name        : SystemBus
Author      : Matthew Holder
Version     : 0.1
Copyright   :
Description : SystemBus concept and implementations for `mos6502`.
===============================================================================
*/
#pragma once

// Required libraries.
#include <cstdint>
#include <array>
#include <iostream>
#include <memory>

namespace mos
{
    /*
    ## Universal 65xx system bus signals
    The SystemBus concept will provide the CPU with a means to address memory or memory mapped
    registers as well as a means for emulated hardware to pull the /IRQ or /NMI lines low.
    ```
    concept SystemBus
    {
        // Lifetime:

        // Must support default construct.
        SystemBus();

        // Must support no-throw destruct.
        ~SystemBus() noexcept;

        // Interrupts:

        // Must support a no-throw getter for the state of the /IRQ line. Returning `true` will
        // indicate the line is asserted.
        bool pending_irq() const noexcept;

        // Must support a no-throw getter for the state of the /NMI line. Returning `true` will
        // indicate the line is asserted.
        bool pending_nmi() const noexcept;

        // Memory mapped bus:

        // Must support a no-throw member to write data to an address on the bus. The `burn`
        // argument may be used to "burn" data into ROM.
        void write(std::uint16_t address, std::uint8_t data, bool burn = false) noexcept;

        // Must support a no-throw member to read data from an address on the bus.
        auto read(std::uint16_t address) noexcept -> std::uint8_t;
    };
    ```
    */

    // Wraps the SystemBus concept to add some additional functionality.
    template <class SystemBus>
    class system_bus : public SystemBus
    {
    public:
        // This group of members and operators are diabled so that this class may only be used to
        // wrap references to existing implementations via a static cast.
        system_bus() noexcept = delete;
        system_bus(system_bus&&) noexcept = delete;
        system_bus(const system_bus&) noexcept = delete;
        ~system_bus() noexcept = delete;
        auto operator =(system_bus&&) noexcept -> system_bus& = delete;
        auto operator =(const system_bus&) noexcept -> system_bus& = delete;
        static void* operator new(std::size_t) = delete;

        // Reads a 16-bit words, in little endian form, from a location on the system bus.
        auto read_word(std::uint16_t address) -> unsigned;

        // Writes a 16-bit word, in little endian form, to a location on the system bus.
        void write_word(std::uint16_t address, unsigned data);

        // Fill an area of memory.
        void fill(std::uint16_t address, std::uint16_t count, std::uint8_t value);

        // Sends the contents from a range to the system bus.
        template <class InputIterator,
            typename = typename std::iterator_traits<InputIterator>::iterator_category>
        void load_input(std::uint16_t base, InputIterator first, InputIterator last);

        // Sends the contents of an input stream, from its current position, to the system bus
        // starting at the `base` address.
        template <typename Elem, class Traits>
        void load_stream(std::uint16_t base, std::basic_istream<Elem, Traits> &source);
    };

    // Basic RAM implementation of the SystemBus concept.
    template <size_t CAPACITY, std::uint8_t FILL>
    class basic_memory
    {
    public:
        static_assert(CAPACITY > 0,
            "You must provide more capacity than zero");
        static_assert((CAPACITY & (CAPACITY - 1)) == 0,
            "Memory capacity can only be in powers of two");

        basic_memory() noexcept;
        ~basic_memory() noexcept = default;

        bool pending_irq() const noexcept;
        bool pending_nmi() const noexcept;

        void write(std::uint16_t address, std::uint8_t data, bool = false) noexcept;
        auto read(std::uint16_t address) noexcept -> std::uint8_t;

    private:
        static const std::uint16_t MASK = static_cast<std::uint16_t>(CAPACITY - 1);

        std::array<std::uint8_t, CAPACITY> ram;

        inline static auto real_address(std::uint16_t address) noexcept -> std::uint16_t;
    };
}

#include "mos_system_bus.inl.h"

#pragma once
#include "mos_system_bus.h"

namespace mos
{
    template <class SystemBus>
    inline auto system_bus<SystemBus>::read_word(std::uint16_t address) -> unsigned
    {
        return SystemBus::read(address) + (SystemBus::read(address + 1) << 8);
    }

    template <class SystemBus>
    inline void system_bus<SystemBus>::write_word(std::uint16_t address, unsigned data)
    {
        SystemBus::write(address++, static_cast<std::uint8_t>(data)); data >>= 8;
        SystemBus::write(address, static_cast<std::uint8_t>(data));
    }

    template <class SystemBus>
    inline void system_bus<SystemBus>::fill(std::uint16_t address, std::uint16_t count,
        std::uint8_t value)
    {
        // We offset count to find the end, and it may overflow.
        for (count += address; address != count; ++address)
        {
            SystemBus::write(address, value);
        }
    }

    template <class SystemBus>
    template <class InputIterator, typename>
    inline void system_bus<SystemBus>::load_input(std::uint16_t base, InputIterator first,
        InputIterator last)
    {
        static_assert(sizeof(decltype(*first)) == sizeof(std::uint8_t), "You should only load "
            "from input sources made of 8-bit elements");
        for (; first != last; ++first)
        {
            SystemBus::write(base++, *first);
        }
    }

    template <class SystemBus>
    template <typename Elem, class Traits>
    inline void system_bus<SystemBus>::load_stream(std::uint16_t base,
        std::basic_istream<Elem, Traits> &source)
    {
        using InputIterator = std::istreambuf_iterator<Elem, Traits>;

        auto *input = source.rdbuf();
        load_input(base, InputIterator(input), InputIterator());
    }

    template <size_t CAPACITY, std::uint8_t FILL>
    inline basic_memory<CAPACITY, FILL>::basic_memory() noexcept
    {
        ram.fill(FILL);
    }

    template <size_t CAPACITY, std::uint8_t FILL>
    inline bool basic_memory<CAPACITY, FILL>::pending_irq() const noexcept
    {
        // RAM chips don't usually assert the /IRQ line.
        return false;
    }

    template <size_t CAPACITY, std::uint8_t FILL>
    inline bool basic_memory<CAPACITY, FILL>::pending_nmi() const noexcept
    {
        // RAM chips don't usually assert the /NMI line.
        return false;
    }

    template <size_t CAPACITY, std::uint8_t FILL>
    inline void basic_memory<CAPACITY, FILL>::write(std::uint16_t address,
        std::uint8_t data, bool) noexcept
    {
        ram[real_address(address)] = data;
    }

    template <size_t CAPACITY, std::uint8_t FILL>
    inline auto basic_memory<CAPACITY, FILL>::read(std::uint16_t address) noexcept -> std::uint8_t
    {
        return ram[real_address(address)];
    }

    template<size_t CAPACITY, std::uint8_t FILL>
    inline auto basic_memory<CAPACITY, FILL>::real_address(std::uint16_t address) noexcept ->
        std::uint16_t
    {
        // So that if this bus or bus component is only wired to certain lines... it will
        // mirror reads and writes as real ICs would.
        return address & MASK;
    }
}

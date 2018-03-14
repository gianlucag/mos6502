# MOS6502 Simulator in C++

Based on Gianluca's [MOS6502 Emulator in C++](https://github.com/gianlucag/mos6502), I forked and
modified his emulator to create an implementation of a 6502 CPU simulator that I could use to test
algorithms and get an accurate cycle count.

## Features

The main features from Gianluca's are retained:
- 100% coverage of legal opcodes.
- Jump table for op-code execution.

New features I've added are:
- Use of a C++ concept called `SystemBus` that will provide bus reads, writes. _/IRQ_ asserts, and
  _/NMI_ asserts.
- Accurate cycle counting for all legal opcodes and interrupt asserts.
- Addition of an instruction or step counter.
- Inner loop now based arround stepping, which can also be manually invoked for debugging.

## Plans

Planned features that I want to implement:
- Definition of a `Debugger` concept for hanlding breakpoints and debugging symbols or listings,
  along with a basic implementation.
- If Gianluca implements them, then illegal opcodes.
- If Gianluca implements them, hardware glitches.

Some possibilities, but not decided:
- Implmentation of other variants of the 6502.

## Testing

According to Gianluca, his emulator was tested against the following test suite:

https://github.com/Klaus2m5/6502_65C02_functional_tests

I provided `mos6502-test.cpp` to tests the cycle accuracy called.

## API

### CPU

The `mos::mos6502` class is the 6502 CPU simulator. Using `mos::mos6502` requires at least one
template parameter to define the additional hardware attached to the simulated system. This type
must implements the `SystemBus` concept. The `mos::mos6502 class provide a nice public API.

- _`mos6502(args...)`_; a variable template constructor that will pass all of its arguments to the
  `SystemBus`.
- _`bus()`_; provides access to the system bus.
- _`a()`_, _`a(byte)`_, _`x()`_, _`x(byte)`_, _`y()`_, _`y(byte)`_; provides read/write access to
  the main registers of the CPU.
- _`sp()`_, _`pc()`_; provides read-only access to the stack-pointer and program-counter registers.
- _`go(word)`_; provides a means to jump the CPU program-counter to any location on the system bus.
- _`status()`_; provides read/write access to the CPU status flags. These are available using:
	- _`negative()`_, _`negative(bool)`_;  the _N_/_Negative_ flag.
	- _`overflow()`_, _`overflow(bool)`_;  the _V_/_oVerflow_ flag.
	- _`brk()`_, _`brk(bool)`_;  the _B_/_Break_ flag.
	- _`decimal()`_, _`decimal(bool)`_;  the _D_/_Decimal Enable_ flag.
	- _`interrupt()`_, _`interrupt(bool)`_;  the _I_/_Interrupt Disable_ flag.
	- _`zero()`_, _`zero(bool)`_;  the _Z_/_Zero_ flag.
	- _`carry()`_, _`carry(bool)`_;  the _C_/_Carry_ flag.
	- _`get()`_; gets the raw value of the register.
	- _`set(byte)`_; sets the raw value of the register.
	- _`reset()`_; resets the register to the usual initial value.
- _`cycles()`_, _`steps()`_; counters that indicate the number of cycles and steps the CPU has
  performed.
- _`power_off()`_; Powers the CPU off, stopping all running code.
- _`reset()`_; Resets the CPU to its initial state.
- _`run()`_; Runs the CPU till it is powered off or lands on an illegal instruction.
- _`step()`_; Executes the next instruction and stops.
- _`push(byte)`_,_`push_word(word)`_; pushes data to the stack without using cycles or steps.
- _`pop(byte)`_,_`pop_word(word)`_; pushes data to the stack without using cycles or steps.

### SystemBus

One C++ concept is defined by this library, the `SystemBus`. This concept is is used to provide the
CPU with access to memory, memory mapped hardware, and interrupt sources. A default implemenation
of this concept is provided called `mos::basic_memory`. A `SystemBus` must support the following
features.

- **`SystemBus()`**. _DefaultConstructible_, requires the type can be constructed by the default
  without any additional arguments.
- **`~SystemBus() noexcept`**. _Destructible_, requires the type can be destructed without throwing
  any exceptions.
- **`bool pending_irq() const noexcept`**. requires the type can signal if the _/IRQ_ line is
  asserted.
- **`bool pending_nmi() const noexcept`**. requires the type can signal if the _/NMI_ line is
  asserted.
- **`void write(std::uint16_t address, std::uint8_t data, bool burn = false) noexcept`**. requires
  the type can write to the system bus, and optionally provide a means to write to read-only
  memory. Please note the CPU will never pass trur for the `burn` argument.
- **`auto read(std::uint16_t address) noexcept -> std::uint8_t`**. requires the type can read from
  system bus.

A `SystemBus` may also support a constructor with number of arguments which are forwarded to it by
the `mos::mos6502`.

When the `SystemBus` is exposed from `mos::mos6502`, it is wrapped by `mos::system_bus` which
provides many additional useful features. These are:

- _`read_word(address)`_; reads a word from the bus, using little-endian format.
- _`write_work(address, word)`_; writes a word to the bus, using little-endian format.
- _`fill(address, count, byte)`_; fills an area of the bus with the same byte.
- _`load_input(address, first, last)`_; loads data from a pair of `InputIterators` onto the bus.
- _`load_stream(address, stream)`_; loads data from an `istream` onto the bus.

## Links ##

- Gianluca's [MOS6502 Emulator in C++](https://github.com/gianlucag/mos6502), the original source.
- [Wikipedia's MOS Technology 6502](http://en.wikipedia.org/wiki/MOS_Technology_6502), provides
  some history and basic information about the original CPU.
- [6502.org Document Archive](http://www.6502.org/documents/datasheets/mos/), provides lots of
  useful documents about the 6502.
- [Preliminary data-sheet on the 6500 Microprocessor Series](http://www.mdawson.net/vic20chrome/cpu/mos_6500_mpu_preliminary_may_1976.pdf)

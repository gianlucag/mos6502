#include "mos6502.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#if defined(__linux__)
extern "C" {
#include <termios.h>
}
#elif defined(_WIN32)
#include <windows.h>
#endif

#include <thread>
#include <chrono>
#include <queue>

// keyboard emulation, multithreading is needed.
// if multithreading is not used, a1basic.bin
// will abnormally stop when it meets goto
// statement.
static std::queue<char> kbdbuf;
static void keyboard()
{
	// make getchar() noecho & return without pressing Enter.
#if defined(__linux__)
	struct termios oldt, newt;
	tcgetattr(0, &oldt); // 0: stdin
	newt = oldt;
	newt.c_lflag &= ~ECHO; // disable echo
	newt.c_lflag &= ~ICANON; // buffer immediately, don't wait for \n
	tcsetattr(0, TCSANOW, &newt);	
#elif defined(_WIN32)
	DWORD mode;
	HANDLE con = GetStdHandle(STD_INPUT_HANDLE);
	GetConsoleMode(con, &mode);
	mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
	SetConsoleMode(con, mode);
#endif

	auto get = []() {
		char ch;
#if defined(__linux__)
		ch = getchar();
#elif defined(_WIN32)
		// can't use getchar() directly on Windows,
		// it requires you press Enter twice before
		// it returns.
		DWORD num;
		HANDLE con = GetStdHandle(STD_INPUT_HANDLE);
		ReadConsole(con, &ch, 1, &num, NULL);
#endif
		ch = toupper(ch);
		return ch == '\n' ? '\r' : ch;
	};

	while(true)
	{
		kbdbuf.push(get());
	}
}

#define PIPE_NAME "\\\\.\\pipe\\apple1"
// pipe server for load.c
void server()
{
	while (1)
	{
		HANDLE hd = CreateNamedPipe( 
			PIPE_NAME,   // pipe name 
			PIPE_ACCESS_DUPLEX,       // read/write access 
			PIPE_WAIT,                // blocking mode 
			PIPE_UNLIMITED_INSTANCES, // max. instances  
			1024,                  // output buffer size 
			1024,                  // input buffer size 
			0,                        // client time-out 
			NULL);   
		if (hd == INVALID_HANDLE_VALUE) 
		{
			exit(-1);
		}
		if (!ConnectNamedPipe(hd, NULL))
		{
			exit(-1);
		}

		while (1)
		{
			char ch; DWORD num; BOOL succ;
			succ = ReadFile(hd, &ch, 1, &num, NULL);
			if (!succ)
				break;
			kbdbuf.push(ch);
		}
		CloseHandle(hd);
	}
}

// 64kb memory
static uint8_t mem[65536];

// WOZ Monitor MMIO setups from:
// https://www.sbprojects.net/projects/apple1/wozmon.php

#define KBD 0xD010
#define KBDCR 0xD011
#define DSP 0xD012
#define DSPCR 0xD013

static uint8_t read(uint16_t addr)
{
	if (addr == KBD)
	{
		char value = kbdbuf.front();
		kbdbuf.pop();
		return value | 0x80; // always set b7. it's required
							 // by WOZ Monitor.
	}
	else if (addr == KBDCR)
	{
		// sleep only if the buffer is empty to speed up emulation
		if (kbdbuf.empty())
		{
			// sleep for 1 millisecond. or the dead loop in WOZ Monitor
			// eats up cpu.
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			return 0x00;
		}
		else
		{
			return 0x80; // set b7 of KBDCR when keyboard is ready.
		}
	}
	else if (addr == DSP)
	{
		return 0x00; // b7 of DSP indicates if the monitor has accep-
					 // ted the character. it's emulator always
					 // accepted immediately.
	}
	else
	{
		return mem[addr];
	}
}

static void write(uint16_t addr, uint8_t value)
{
	if (addr == KBD)
	{
		// meaningless
	}
	else if (addr == KBDCR)
	{
		// meaningless
	}
	else if (addr == DSP)
	{
		int v = toupper(value & 0x7F); // clear b7. it is ignored.
		putchar(v == '\r' ? '\n' : v); // WOZ Monitor uses \r as
									   // newline. convert it.
	}
	else if (addr >= 0xFF00)
	{
		// do nothing. this is where WOZ Monitor is located.
		// it is ROM, so unmodifiable.
	}
	else
	{
		mem[addr] = value;
	}
}

// WOZ Monitor
static const uint8_t rom[] = {
	0xD8, 0x58, 0xA0, 0x7F, 0x8C, 0x12, 0xD0, 0xA9,
	0xA7, 0x8D, 0x11, 0xD0, 0x8D, 0x13, 0xD0, 0xC9,
	0xDF, 0xF0, 0x13, 0xC9, 0x9B, 0xF0, 0x03, 0xC8,
	0x10, 0x0F, 0xA9, 0xDC, 0x20, 0xEF, 0xFF, 0xA9,
	0x8D, 0x20, 0xEF, 0xFF, 0xA0, 0x01, 0x88, 0x30,
	0xF6, 0xAD, 0x11, 0xD0, 0x10, 0xFB, 0xAD, 0x10,
	0xD0, 0x99, 0x00, 0x02, 0x20, 0xEF, 0xFF, 0xC9,
	0x8D, 0xD0, 0xD4, 0xA0, 0xFF, 0xA9, 0x00, 0xAA,
	0x0A, 0x85, 0x2B, 0xC8, 0xB9, 0x00, 0x02, 0xC9,
	0x8D, 0xF0, 0xD4, 0xC9, 0xAE, 0x90, 0xF4, 0xF0,
	0xF0, 0xC9, 0xBA, 0xF0, 0xEB, 0xC9, 0xD2, 0xF0,
	0x3B, 0x86, 0x28, 0x86, 0x29, 0x84, 0x2A, 0xB9,
	0x00, 0x02, 0x49, 0xB0, 0xC9, 0x0A, 0x90, 0x06,
	0x69, 0x88, 0xC9, 0xFA, 0x90, 0x11, 0x0A, 0x0A,
	0x0A, 0x0A, 0xA2, 0x04, 0x0A, 0x26, 0x28, 0x26,
	0x29, 0xCA, 0xD0, 0xF8, 0xC8, 0xD0, 0xE0, 0xC4,
	0x2A, 0xF0, 0x97, 0x24, 0x2B, 0x50, 0x10, 0xA5,
	0x28, 0x81, 0x26, 0xE6, 0x26, 0xD0, 0xB5, 0xE6,
	0x27, 0x4C, 0x44, 0xFF, 0x6C, 0x24, 0x00, 0x30,
	0x2B, 0xA2, 0x02, 0xB5, 0x27, 0x95, 0x25, 0x95,
	0x23, 0xCA, 0xD0, 0xF7, 0xD0, 0x14, 0xA9, 0x8D,
	0x20, 0xEF, 0xFF, 0xA5, 0x25, 0x20, 0xDC, 0xFF,
	0xA5, 0x24, 0x20, 0xDC, 0xFF, 0xA9, 0xBA, 0x20,
	0xEF, 0xFF, 0xA9, 0xA0, 0x20, 0xEF, 0xFF, 0xA1,
	0x24, 0x20, 0xDC, 0xFF, 0x86, 0x2B, 0xA5, 0x24,
	0xC5, 0x28, 0xA5, 0x25, 0xE5, 0x29, 0xB0, 0xC1,
	0xE6, 0x24, 0xD0, 0x02, 0xE6, 0x25, 0xA5, 0x24,
	0x29, 0x07, 0x10, 0xC8, 0x48, 0x4A, 0x4A, 0x4A,
	0x4A, 0x20, 0xE5, 0xFF, 0x68, 0x29, 0x0F, 0x09,
	0xB0, 0xC9, 0xBA, 0x90, 0x02, 0x69, 0x06, 0x2C,
	0x12, 0xD0, 0x30, 0xFB, 0x8D, 0x12, 0xD0, 0x60,
	0x00, 0x00, 0x00, 0x0F, 0x00, 0xFF, 0x00, 0x00,
};

int main()
{
	mos6502 mos(read, write);	

	// copy WOZ Monitor into ROM area.
	memcpy(&mem[0xff00], rom, 256);

	// start keyboard emulation
	std::thread(keyboard).detach();
	// start pipe server
	std::thread(server).detach();

	mos.Reset(); // load reset vector into PC
	mos.RunEternally();

	return 0;
}

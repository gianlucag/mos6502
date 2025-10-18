this is a generic debugger based on the mos6502 class.

when you run it, 4 windows appear.
the top left window shows processor registers.
the window below that shows memory.
the window below that allows commands.
the window on the right hand side shows various output.

get a list of commands by typing "help" or "?" and pressing <enter>.
most of these are self explanatory............

A=<num>                         set the A register
PC=<num>                        set the PC
SP=<num>                        set the STACK POINTER
SR=<num>                        set the STATUS
X=<num>                         set the X register
Y=<num>                         set the Y register
break <num>                     set a breakpoint
ihex <file>                     load ihex file
load <num> <file>               load binary file at addr
mem <num>                       show memory at addr
reset                           reset the cpu
run                             run program to breakpoint
step                            single step program
unbreak <num>                   clr a breakpoint
verbose <num>                   set verbosity (0-3)
wpclr <num>[-<num>]             clr write protect on address or range of addresses
wpset <num>[-<num>]             set write protect on address or range of addresses
quit                            quit the program
exit                            same as quit
help                            print help
?                               same as help

these commands are case insensitve.

any place you see <num>, the following formats are accepted:
$??        :: a hex number, like $55
0x??       :: a hex number, like 0x55
x??        :: a hex number, like x55
%????????  :: a binary number, like %01010101
b????????  :: a binary number, like b01010101
0b???????? :: a binary number, like 0b01010101
@???       :: an octal number, like @125
o???       :: an octal number, like o125
0???       :: an octal number, like 0125
anything else is considered decimal.

SR= is a special case:
the letters [nvbdizcNVBDIZC] are accepted.
SR=NV will set the N and V bits of the SR, leaving other bits alone.
SR=d will clear the D bit of the SR, leaving other bits alone.
SR=Zc will set the Z bit and clear the C bit of the SR, leaving other bits alone.

for <file>, tab completion should work as expected.


disassembly and lst file input is planned, but not yet implemented.
IRQ and NMI handling is also on the future roadmap.

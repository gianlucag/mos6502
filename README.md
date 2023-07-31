# apple1 emulator
An Apple I Emulator running in terminal.

# how to use
run the emulator in terminal. it takes no arguments.

there are some programs in tape folder. to use them, copy their contents
and paste it into emulator. for .txt files in tape folder, they are Apple 1 'commandline' commands. the last command of each file is the entry
address of the program. type R and then Enter to run the program.

for .bas files, you should first load tapes/BASIC.txt, and run command
E000R to enter the BASIC interpreter, and then copy and paste the .bas files, and finally type RUN to start the BASIC program. 

load.c is a tool that uses pipe to feed file as commandline input to the
emulator. but it only supports Windows for now.

# Makefile to run unit tests

# Fail early if required tools are missing
ifeq (, $(shell which git 2>/dev/null))
$(error "Error: git not found in PATH")
endif

ifeq (, $(shell which dosbox 2>/dev/null))
$(error "Error: dosbox not found in PATH")
endif

all: 6502_65C02_functional_tests/as65_142 main
	./tester.sh

6502_65C02_functional_tests:
	@echo "Fetching functional tests from GitHub..."
	git clone https://github.com/Klaus2m5/6502_65C02_functional_tests

6502_65C02_functional_tests/as65_142: 6502_65C02_functional_tests
	echo "Unpacking assembler..."
	mkdir -p 6502_65C02_functional_tests/as65_142
	( cd 6502_65C02_functional_tests/as65_142 && unzip ../as65_142.zip )

main: main.cpp ../mos6502.cpp ../mos6502.h
	g++ -o main ../mos6502.cpp main.cpp

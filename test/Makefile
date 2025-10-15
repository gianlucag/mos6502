# Makefile to run unit tests

# Fail early if required tools are missing
ifeq (, $(shell which git 2>/dev/null))
$(error "Error: git not found in PATH")
endif

ifeq (, $(shell which dosbox 2>/dev/null))
$(error "Error: dosbox not found in PATH")
endif

SHELL := /bin/bash
.SHELLFLAGS := -e -o pipefail -c

BASE := 6502_65C02_functional_tests

all: $(BASE) $(BASE)/as65_142 main tests tests
   @echo TEST COMPLETE: success

$(BASE):
	@echo "Fetching functional tests from GitHub..."
	git clone https://github.com/Klaus2m5/6502_65C02_functional_tests

$(BASE)/as65_142:
	echo "Unpacking assembler..."
	mkdir -p $(BASE)/as65_142
	( cd $(BASE)/as65_142 && unzip ../as65_142.zip )

main: main.cpp ../mos6502.cpp ../mos6502.h
	g++ -g -o main ../mos6502.cpp main.cpp

tests: 6502_functional_test 6502_decimal_test 6502_interrupt_test

ft.hex:
	cp $(BASE)/6502_functional_test.a65 ft.a65
	./as65.sh ft.a65

6502_functional_test: main ft.hex
	@echo "================ Running $@"
	./main ft.hex 0x400 0x3469 quiet # magic numbers come from comments and examination of *.lst

dt.hex:
	cp $(BASE)/6502_decimal_test.a65 dt.a65
	patch -p0 < decimal.patch
	./as65.sh dt.a65

6502_decimal_test: main dt.hex
	@echo "================ Running $@"
	./main dt.hex 0x200 ?0x000b quiet # magic numbers come from comments and examination of *.lst

it.hex:
	cp $(BASE)/6502_interrupt_test.a65 it.a65
	./as65.sh it.a65

6502_interrupt_test: main it.hex
	@echo "================ Running $@"
	./main it.hex 0x400 0x06f5 quiet # magic numbers come from comments and examination of *.lst

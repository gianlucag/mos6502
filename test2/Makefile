# Makefile to run unit tests

# Fail early if required tools are missing
ifeq (, $(shell which git 2>/dev/null))
$(error "Error: git not found in PATH")
endif

SHELL := /bin/bash
.SHELLFLAGS := -e -o pipefail -c

BASE := 65x02

all: $(BASE) main tests
   @echo TEST COMPLETE: success

clean:
	rm -rf $(BASE) ft.* dt.* it.*

$(BASE):
	@test ! -e "$@" || { echo "do NOT use 'make -B', use 'make clean ; make' instead"; exit 1; }
	@echo "Fetching functional tests from GitHub..."
	git clone https://github.com/SingleStepTests/65x02.git

main: main.cpp ../mos6502.cpp ../mos6502.h
	g++ -O3 -o main ../mos6502.cpp main.cpp

tests: main
	./main 65x02/6502/v1/00.json


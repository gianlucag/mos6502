#!/bin/bash

set -euo pipefail  # safer: also catches unset vars and failed pipes
IFS=$'\n\t'        # good habit when set -u

check_cmd() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "Error: '$1' is not installed or not in PATH." >&2
        exit 1
    fi
}

check_cmd git
check_cmd dosbox

if [ ! -d "6502_65C02_functional_tests" ]; then
    echo "Fetching functional tests from GitHub..."
    if ! git clone https://github.com/Klaus2m5/6502_65C02_functional_tests; then
        echo "Error: git clone failed." >&2
        exit 1
    fi
fi

if [ ! -d "6502_65C02_functional_tests/as65_142" ]; then
    echo "Unpacking assembler..."
    mkdir -p 6502_65C02_functional_tests/as65_142
    (
        cd 6502_65C02_functional_tests/as65_142
        unzip ../as65_142.zip
    )
fi

if [[ ! -x ./main ]]; then
    echo "Executable 'main' not found — building..."
    g++ -o main ../mos6502.cpp main.cpp || { echo "Error: g++ build failed"; exit 1; }
else
    echo "'main' already exists — skipping build."
fi

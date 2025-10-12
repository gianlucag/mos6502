#!/bin/bash
set -euo pipefail

src_file="$1"
src_dir=$(dirname "$(realpath "$src_file")")
base_name=$(basename "${src_file%.*}")
dest_dir="6502_65C02_functional_tests/as65_142"

# Sanity checks
[[ -f "$src_file" ]] || { echo "Error: $src_file not found"; exit 1; }
[[ -d "$dest_dir" ]] || { echo "Error: $dest_dir not found"; exit 1; }

echo "Copying $src_file to $dest_dir"
cp "$src_file" "$dest_dir/" || { echo "Copy failed"; exit 1; }

(
  cd "$(dirname "$dest_dir")"
  echo "Running assembler under DOSBox..."
  dosbox -c "mount c $(pwd)" \
         -c "c:" \
         -c "cd as65_142" \
         -c "AS65-DOS.EXE -x1 -o${base_name}.hex -l${base_name}.lst -s2 -c -i -t -u -z $(basename "$src_file")" \
         -c "echo." -c "pause"
) || { echo "Assembler failed"; exit 1; }

# --- CHANGED BLOCK: copy artifacts back (handle UPPERCASE and 8.3 truncation) ---
echo "Copying artifacts back to $src_dir with normalized names..."
shopt -s nullglob
found=0
base_up="${base_name^^}"
base_up8="${base_up:0:8}"

for f in "$dest_dir"/*; do
    fn=$(basename "$f")
    name="${fn%.*}"          # filename without extension
    ext="${fn##*.}"          # extension (may already be upper)
    name_up="${name^^}"

    # Match exact uppercase or 8.3-truncated uppercase
    if [[ "$name_up" == "$base_up" || "$name_up" == "$base_up8" ]]; then
        lc_ext="${ext,,}"
        cp "$f" "$src_dir/${base_name}.${lc_ext}" || { echo "Failed to copy $fn"; exit 1; }
        ((found++))
    fi
done
shopt -u nullglob

[[ $found -gt 0 ]] || { echo "No artifacts matching $base_name (or 8.3 form) found in $dest_dir" >&2; exit 1; }

echo "Success: copied $found artifact(s) to $src_dir"

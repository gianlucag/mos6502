#!/bin/bash
set -euo pipefail

src_file="$1"
src_dir=$(dirname "$(realpath "$src_file")")
base_name=$(basename "${src_file%.*}")
dest_dir="6502_65C02_functional_tests/as65_142"

# Sanity checks
[[ -f "$src_file" ]] || { echo "Error: $src_file not found"; exit 1; }
[[ -d "$dest_dir" ]] || { echo "Error: $dest_dir not found"; exit 1; }

# ---- Collision check (case-insensitive) ----
shopt -s nullglob
declare -a collisions=()
check_fn_lc="$(basename "$src_file")"
check_fn_lc="${check_fn_lc,,}"         # full filename (with extension), lowercased
check_base_lc="${base_name,,}"         # basename (no extension), lowercased

for f in "$dest_dir"/*; do
    [[ -f "$f" ]] || continue
    bn="$(basename "$f")"
    bn_lc="${bn,,}"
    name="${bn%.*}"
    name_lc="${name,,}"
    if [[ "$bn_lc" == "$check_fn_lc" || "$name_lc" == "$check_base_lc" ]]; then
        collisions+=("$bn")
    fi
done
shopt -u nullglob

if (( ${#collisions[@]} > 0 )); then
    echo "Error: collision(s) in '$dest_dir' (case-insensitive): ${collisions[*]}" >&2
    echo "Refusing to copy to avoid overwriting existing files." >&2
    exit 1
fi

echo "Copying $src_file to $dest_dir"
cp "$src_file" "$dest_dir/" || { echo "Copy failed"; exit 1; }

(
  cd "$(dirname "$dest_dir")"
  echo "Running assembler under DOSBox..."
  dosbox -c "mount c $(pwd)" \
         -c "c:" \
         -c "cd as65_142" \
         -c "AS65-DOS.EXE -x1 -o${base_name}.hex -l${base_name}.lst -s2 -c -i -t -u -z $(basename "$src_file")" \
         -c "exit" # change that "exit" to a "pause" if you need to debug
) || { echo "Assembler failed"; exit 1; }

# ---- Artifact move block ----
echo "Moving artifacts back to $src_dir with normalized names..."
shopt -s nullglob
found=0
src_base_lc="${base_name,,}"

for f in "$dest_dir"/*; do
    [[ -f "$f" ]] || continue
    fn="$(basename "$f")"
    name="${fn%.*}"           # basename without extension
    ext="${fn##*.}"           # extension
    name_lc="${name,,}"       # lowercased basename

    if [[ "$name_lc" == "$src_base_lc" ]]; then
        echo "Moving $f → $src_dir/${base_name}.${ext,,}"
        mv "$f" "$src_dir/${base_name}.${ext,,}" || { echo "Failed to move $fn"; exit 1; }
        ((found++)) || true    # prevent 'set -e' from exiting since found++ returns old value (non-zero on later iterations)
    fi
done
shopt -u nullglob

(( found > 0 )) || { echo "No artifacts for '$base_name' found in $dest_dir" >&2; exit 1; }

echo "Success: moved $found artifact(s) to $src_dir"

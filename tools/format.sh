#!/bin/bash
# A simple script to run clang-format over the source code

# Get the directory of this script, then go up one level to the module root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." &> /dev/null && pwd)"

echo "Formatting source files in $SCRIPT_DIR/src and $SCRIPT_DIR/include..."

# Find all .c and .h files in src/ and include/ and format them in-place
if [ -d "$SCRIPT_DIR/src" ] || [ -d "$SCRIPT_DIR/include" ]; then
    find "$SCRIPT_DIR/src" "$SCRIPT_DIR/include" -type f \( -name "*.c" -o -name "*.h" \) 2>/dev/null -exec clang-format --style="{IndentWidth: 4, ColumnLimit: 120, MaxEmptyLinesToKeep: 2}" -i {} +
fi

echo "Formatting complete!"

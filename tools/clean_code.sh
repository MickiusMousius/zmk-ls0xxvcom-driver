#!/bin/bash
# Wrapper to add blank lines and then format the code

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"

echo "Running add_blank_lines.py..."
python3 "$SCRIPT_DIR/add_blank_lines.py"

echo "Running format.sh..."
bash "$SCRIPT_DIR/format.sh"

echo "Code cleaning complete!"

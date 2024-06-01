#!/bin/bash

# Define the hardcoded path to manuf.py
MANUF_PY_PATH="venv/lib/python3.12/site-packages/manuf/manuf.py"

# Define the new URL for MANUF_URL
NEW_URL="https://www.wireshark.org/download/automated/data/manuf"

# Check if the file exists
if [ ! -f "$MANUF_PY_PATH" ]; then
    echo "File $MANUF_PY_PATH does not exist."
    exit 1
fi

# Replace the MANUF_URL variable with the new URL
sed -i.bak "s|MANUF_URL = \".*\"|MANUF_URL = \"$NEW_URL\"|" "$MANUF_PY_PATH"

echo "MANUF_URL in $MANUF_PY_PATH has been updated to $NEW_URL"


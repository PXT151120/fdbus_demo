#!/bin/bash

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Set the library path and run name_server
export LD_LIBRARY_PATH="$SCRIPT_DIR/_build_install/install/usr/lib:$LD_LIBRARY_PATH"
exec "$SCRIPT_DIR/_build_install/install/usr/bin/name_server" "$@"

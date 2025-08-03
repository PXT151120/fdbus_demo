#!/bin/bash

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Set the library path and run host_server
export LD_LIBRARY_PATH="$SCRIPT_DIR/install/usr/lib:$LD_LIBRARY_PATH"
exec "$SCRIPT_DIR/install/usr/bin/host_server" "$@"

#!/bin/bash
# Build script for TFT Dash Display
# Wraps 'zig build' for convenience

set -e

if [[ "$1" == "test" ]]; then
    echo "Building and running tests..."
    zig build test
else
    echo "Building..."
    zig build
fi

echo "Build complete. Binaries in zig-out/bin/"

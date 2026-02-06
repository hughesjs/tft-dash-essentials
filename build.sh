#!/bin/bash
# Build script for TFT Dash Display
# Automatically detects platform (macOS vs Linux)

set -e

if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS build
    echo "Building for macOS..."
    g++ -o testsdl testsdl.cpp -I/Library/Frameworks/SDL2.framework/Headers -F/Library/Frameworks -framework SDL2
else
    # Linux build (Raspberry Pi)
    echo "Building for Linux..."
    g++ -o testsdl testsdl.cpp -lSDL2 -lpthread
fi

echo "Build complete: testsdl"

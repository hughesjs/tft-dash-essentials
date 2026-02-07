#!/bin/bash
# Build script for TFT Dash Display
# Automatically detects platform (macOS vs Linux)

set -e

if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS build
    echo "Building for macOS..."
    g++ -o testsdl src/testsdl.cpp src/parser.c -I/Library/Frameworks/SDL2.framework/Headers -F/Library/Frameworks -framework SDL2
else
    # Linux build (Raspberry Pi)
    echo "Building for Linux..."
    g++ -o testsdl src/testsdl.cpp src/parser.c -lSDL2 -lpthread
fi

echo "Build complete: testsdl"

# Build tests if requested
if [[ "$1" == "test" ]]; then
    echo "Building tests..."
    gcc -o test_parser src/test_parser.c src/parser.c -std=c99 -lm
    echo "Build complete: test_parser"
fi

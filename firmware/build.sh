#!/bin/bash
#
# build.sh — Build TFT Dash firmware using arduino-cli
#
# Compiles the .ino sketch for the Gen4 board (ATMega32u4/Leonardo) and
# outputs the .hex file to build/firmware.hex.
#
# Prerequisites:
#   - arduino-cli installed and on PATH
#
# Usage:
#   ./build.sh              # Compile firmware
#   ./build.sh setup        # Install board support + libraries (first time)
#   ./build.sh clean        # Remove build output
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
SKETCH="$SCRIPT_DIR/firmware.ino"

# Gen4 board = ATMega32u4 = Arduino Leonardo
FQBN="arduino:avr:leonardo"

# --- Check arduino-cli is available ---
check_arduino_cli() {
    if ! command -v arduino-cli &>/dev/null; then
        echo "ERROR: arduino-cli not found on PATH"
        echo ""
        echo "Install it:"
        echo "  curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh"
        echo "  # or: pacman -S arduino-cli"
        exit 1
    fi
}

# --- Install board support and libraries ---
setup() {
    check_arduino_cli

    echo "Installing Arduino AVR board support..."
    arduino-cli core update-index
    arduino-cli core install arduino:avr

    echo ""
    echo "Installing libraries..."
    arduino-cli lib install "RTClib"

    # Eeprom24C32_64 isn't in the Library Manager — install from git
    if ! arduino-cli lib list 2>/dev/null | grep -q "Eeprom24C32_64"; then
        echo "Installing Eeprom24C32_64 from GitHub..."
        arduino-cli config set library.enable_unsafe_install true
        arduino-cli lib install --git-url https://github.com/jlesech/Eeprom24C32_64.git
    fi

    echo ""
    echo "Setup complete."
}

# --- Compile the sketch ---
compile() {
    check_arduino_cli

    mkdir -p "$BUILD_DIR"

    echo "Compiling firmware for $FQBN..."
    arduino-cli compile \
        --fqbn "$FQBN" \
        --output-dir "$BUILD_DIR" \
        "$SKETCH"

    # arduino-cli names the output after the sketch
    local hex="$BUILD_DIR/firmware.ino.hex"
    if [ ! -f "$hex" ]; then
        echo "ERROR: Compilation produced no hex file"
        exit 1
    fi

    echo "  OK: $hex ($(wc -c < "$hex") bytes)"
}

cd "$SCRIPT_DIR"

case "${1:-build}" in
    build)
        compile
        ;;

    setup)
        setup
        ;;

    clean)
        echo "Cleaning build output..."
        rm -rf "$BUILD_DIR"
        echo "Done."
        ;;

    *)
        echo "Usage: $0 [build|setup|clean]"
        exit 1
        ;;
esac

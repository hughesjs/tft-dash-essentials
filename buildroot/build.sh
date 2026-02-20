#!/bin/bash
#
# build.sh — Build the TFT Dash SD card image
#
# Cross-compiles the display app for ARM, then runs Buildroot to produce
# a flashable SD card image.
#
# Prerequisites:
#   - Buildroot cloned alongside this repo (or specify BUILDROOT_DIR)
#   - zig (for cross-compilation)
#   - Standard build tools (gcc, make, patch, unzip, rsync, etc.)
#
# Usage:
#   ./build.sh              # Cross-compile app + full Buildroot build
#   ./build.sh rebuild      # Cross-compile app + rebuild image only
#   ./build.sh clean        # Clean Buildroot output
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(dirname "$SCRIPT_DIR")"
DISPLAY_DIR="$REPO_DIR/tftdashdisplay"
BUILDROOT_DIR="${BUILDROOT_DIR:-$REPO_DIR/buildroot-src}"

# --- Find the Buildroot sysroot (varies by toolchain type) ---
find_sysroot() {
    # External toolchain: sysroot is inside host/<tuple>/sysroot
    local sysroot
    sysroot=$(find "$BUILDROOT_DIR/output/host" -maxdepth 2 -name sysroot -type d 2>/dev/null | head -1)
    if [ -z "$sysroot" ] && [ -d "$BUILDROOT_DIR/output/staging" ]; then
        sysroot="$BUILDROOT_DIR/output/staging"
    fi
    echo "$sysroot"
}

# --- Ensure Buildroot has ARM SDL2 in staging ---
ensure_staging_sdl2() {
    local sysroot
    sysroot=$(find_sysroot)

    if [ -n "$sysroot" ] && [ -f "$sysroot/usr/lib/libSDL2.so" ]; then
        return 0
    fi

    echo "Building ARM SDL2 via Buildroot (first time only, may take a while)..."
    cd "$BUILDROOT_DIR"
    make BR2_EXTERNAL="$SCRIPT_DIR" tftdash_defconfig
    make BR2_EXTERNAL="$SCRIPT_DIR" sdl2
    cd "$SCRIPT_DIR"

    sysroot=$(find_sysroot)
    if [ -z "$sysroot" ] || [ ! -f "$sysroot/usr/lib/libSDL2.so" ]; then
        echo "ERROR: Buildroot did not produce staging SDL2"
        exit 1
    fi
}

# --- Cross-compile the display binary ---
cross_compile() {
    check_buildroot
    ensure_staging_sdl2

    local sysroot
    sysroot=$(find_sysroot)

    echo "Cross-compiling display binary for Raspberry Pi (armhf)..."

    cd "$DISPLAY_DIR"
    # Match the glibc version from Buildroot's toolchain to avoid linker errors
    local glibc_ver
    glibc_ver=$(strings "$sysroot/lib/libc.so.6" 2>/dev/null \
        | grep -oP 'GLIBC_\K2\.\d+' | sort -t. -k2 -n | tail -1 || echo "2.38")

    zig build "-Dtarget=arm-linux-gnueabihf.${glibc_ver}" -Doptimize=ReleaseSafe \
        -Dsdl2-include-path="$sysroot/usr/include/SDL2" \
        -Dsdl2-lib-path="$sysroot/usr/lib"
    cd "$SCRIPT_DIR"

    local bin="$DISPLAY_DIR/zig-out/bin/testsdl"
    if [ ! -f "$bin" ]; then
        echo "ERROR: Build produced no binary at $bin"
        exit 1
    fi

    if ! file "$bin" | grep -q "ARM"; then
        echo "ERROR: $bin is not an ARM binary"
        exit 1
    fi

    echo "  OK: $(file "$bin")"
}

# --- Check Buildroot is available ---
check_buildroot() {
    if [ ! -d "$BUILDROOT_DIR" ]; then
        echo "Buildroot not found at $BUILDROOT_DIR"
        echo ""
        echo "Clone it first:"
        echo "  git clone https://github.com/buildroot/buildroot.git $BUILDROOT_DIR"
        echo ""
        echo "Or set BUILDROOT_DIR to point to your existing Buildroot checkout."
        exit 1
    fi
}

cd "$SCRIPT_DIR"

case "${1:-build}" in
    build)
        cross_compile

        echo ""
        echo "=== TFT Dash Image Builder ==="
        echo "Buildroot: $BUILDROOT_DIR"
        echo "External:  $SCRIPT_DIR"
        echo ""

        cd "$BUILDROOT_DIR"
        make BR2_EXTERNAL="$SCRIPT_DIR" tftdash_defconfig
        make BR2_EXTERNAL="$SCRIPT_DIR"

        echo ""
        echo "=== Done ==="
        echo "SD card image: $BUILDROOT_DIR/output/images/sdcard.img"
        echo ""
        echo "Flash with:"
        echo "  sudo dd if=$BUILDROOT_DIR/output/images/sdcard.img of=/dev/sdX bs=4M status=progress"
        ;;

    rebuild)
        cross_compile

        echo ""
        echo "Rebuilding image..."

        cd "$BUILDROOT_DIR"
        make BR2_EXTERNAL="$SCRIPT_DIR" tftdash-rebuild
        make BR2_EXTERNAL="$SCRIPT_DIR"

        echo ""
        echo "SD card image: $BUILDROOT_DIR/output/images/sdcard.img"
        ;;

    clean)
        check_buildroot
        echo "Cleaning build output..."
        cd "$BUILDROOT_DIR"
        make BR2_EXTERNAL="$SCRIPT_DIR" clean
        echo "Done."
        ;;

    *)
        echo "Usage: $0 [build|rebuild|clean]"
        exit 1
        ;;
esac

# CLAUDE.md - TFT Dash Display

## Overview

This is the display application for the TFT Dash motorcycle dashboard. It runs on a Raspberry Pi, receives sensor data from the firmware over USB serial, and renders the dashboard GUI using SDL3 at 1024x600 resolution. The entire codebase is pure C23.

## Build

```bash
# Build everything
zig build

# Build and run all tests (43 tests across 5 suites)
zig build test

# Build and run the dashboard
zig build run

# Cross-compile for Raspberry Pi (aarch64)
zig build -Dtarget=aarch64-linux-gnu -Doptimize=ReleaseSafe
```

Binaries are output to `zig-out/bin/dashboard`.

### Dependencies

- Zig 0.15+ (`pacman -S zig` on Arch, or download from ziglang.org)
- SDL3 is built from source and statically linked via Zig's dependency system (castholm/SDL) — no system SDL library needed
- pthreads (linked via build.zig)

## Source Files

```
display/
├── src/
│   ├── main.c              Main display application (~2200 lines)
│   ├── parser.h/c          Serial message parsing with data-driven lookup tables
│   ├── assets.h/c          Asset management with themed texture lookup
│   ├── sensor_feed.h/c     Bike sensor data feed (serial/pipe connection + background thread)
│   ├── menu.h/c            Menu system data tables (screen routing, cursors, thumbnails)
│   ├── tpms_feed.h/c       TPMS data feed (Standard + eBay hardware, binary protocol decoding)
│   ├── test_parser.c       Parser test suite (10 tests)
│   ├── test_assets.c       Asset manager test suite (7 tests)
│   ├── test_sensor_feed.c  Sensor feed integration tests (9 tests)
│   ├── test_menu.c         Menu data table tests (8 tests)
│   ├── test_tpms_feed.c    TPMS frame decoding tests (9 tests)
│   └── TPMSTest.cpp        Legacy standalone TPMS test utility
├── assets/themes/           BMP files organised by theme
├── build.zig                Zig build system configuration
├── build.zig.zon            Zig package manifest
├── CLAUDE.md                This file
└── REFACTORING.md           Refactoring progress log
```

## Architecture

### Modules

| Module | Responsibility | SDL dependency |
|--------|---------------|----------------|
| `parser` | Deserialise comma-delimited serial messages into structs | None |
| `assets` | Load themed BMP textures, two-key hash map lookup | Yes (SDL_Texture) |
| `sensor_feed` | Own serial/pipe connection, background thread, expose const state | None |
| `menu` | Data tables for menu screens (backgrounds, cursors, thumbnails) | None |
| `tpms_feed` | Own TPMS serial connection, binary protocol decoding, background thread | None |
| `main` | SDL init, render loop, drawing helpers | Yes |

### Main Application (`main.c`)

The render loop reads state through const pointers from the feed modules:

```c
static const dashboard_state *dash;
static const menu_state *menu;
static const nav_state *nav;
static const tpms_state *tpms;
```

These are refreshed each frame from `sensor_feed` and `tpms_feed`. The main file contains:
- Sprite atlas coordinate tables and glyph font descriptors
- Drawing helper functions (numbers, gauges, indicators, navigation overlay)
- `draw_menu()` — menu rendering driven by `menu.h` lookup tables
- `draw_dashboard()` — main dashboard rendering
- `main()` — SDL init, module creation, event/render loop

### Threading Model

Three threads, no explicit synchronisation:
1. **Main thread**: SDL event loop + rendering (reads const pointers)
2. **`sensor_feed` thread**: Reads firmware serial data, writes internal state
3. **`tpms_feed` thread**: Reads TPMS serial data, writes internal state

### Serial Protocol

The firmware sends comma-delimited messages framed by markers:
- **Live data**: `{,speed,rpm,coolant,battery,hour,minute,fuel,...,}`
- **Menu data**: `[,menustate,odo1,odo2,...,settings,...,]`

Parsing is handled by the `parser` module using data-driven field descriptor tables.

### Serial Ports

**Dashboard** (sensor_feed):
- Simulator: `/tmp/tft-dash-pipe`
- Linux: `/dev/ttyACM0`, `/dev/ttyACM1`
- macOS: `cu.usbserial-*`, `cu.usbmodem*`
- Baud rate: 115200

**TPMS** (tpms_feed):
- Linux: `/dev/ttyUSB0`, `/dev/ttyUSB1`
- macOS: `cu.usbserial-D3077502`, `cu.usbserial-210`
- Baud rate: 9600 (Standard) or 19200 (eBay)

## Theming

9 themes: default/white, green, red, blue, orange, yellow, night, bright (high-contrast), dark.

Each theme has its own directory under `assets/themes/`. All 9 themes are loaded into the asset store at startup. Textures are retrieved via `tex("Name.bmp")` for the current theme or `tex_from("theme", "Name.bmp")` for a specific theme.

## Key Constants

- Screen resolution: `1024x600` (`SCREEN_WIDTH`, `SCREEN_HEIGHT`)
- Serial baud rate: `115200`
- TPMS hardware types: `TPMS_MODEL_STANDARD (100)`, `TPMS_MODEL_EBAY (200)`
- Menu states: integer ranges mapped to `menu_screen` enum via `menu_screen_for_state()`

## Guidelines

- All source is C23 — no C++ features
- All rendering uses SDL3 textures created from BMP surfaces — no TTF fonts
- When adding new visual elements, follow the existing sprite-sheet pattern with themed variants
- Test serial data parsing changes carefully — the message format must match the firmware exactly
- New subsystems should follow the established pattern: opaque struct, const accessors, integration tests

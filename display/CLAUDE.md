# CLAUDE.md - TFT Dash Display

## Overview

This is the display application for the TFT Dash motorcycle dashboard. It runs on a Raspberry Pi, receives sensor data from the firmware over USB serial, and renders the dashboard GUI using SDL3 at 1024x600 resolution. The entire codebase is pure C23.

## Build

```bash
# Build everything
zig build

# Build and run all tests (62 tests across 6 suites)
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
│   ├── animation.h/c       Data-driven animation system (oneshot, ping-pong, loop, chase modes) with registry
│   ├── dashboard_anims.h/c Animation instances (startup, flash, info transition, RPM) and data tables
│   ├── draw.h              Shared rendering state and drawing primitives (in progress — being extracted from main.c)
│   ├── test_parser.c       Parser test suite (10 tests)
│   ├── test_assets.c       Asset manager test suite (7 tests)
│   ├── test_sensor_feed.c  Sensor feed integration tests (9 tests)
│   ├── test_menu.c         Menu data table tests (8 tests)
│   ├── test_tpms_feed.c    TPMS frame decoding tests (9 tests)
│   ├── test_animation.c    Animation test suite (17 tests)
│   └── TPMSTest.cpp        Legacy standalone TPMS test utility
├── assets/themes/           PNG files organised by theme
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
| `assets` | Load themed PNG textures, two-key hash map lookup | Yes (SDL_Texture) |
| `sensor_feed` | Own serial/pipe connection, background thread, expose const state | None |
| `menu` | Data tables for menu screens (backgrounds, cursors, thumbnails) | None |
| `tpms_feed` | Own TPMS serial connection, binary protocol decoding, background thread | None |
| `animation` | Data-driven animation engine (oneshot, ping-pong, loop, chase modes) with global registry | None |
| `dashboard_anims` | Concrete animation instances (startup, flash, info transition, RPM) and data tables (info screen textures, rev counter reveal thresholds) | None |
| `draw` | Shared rendering state and drawing primitives (in progress — being extracted from main.c) | Yes (SDL_Texture) |
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

Three threads, deliberately no mutex synchronisation:
1. **Main thread**: SDL event loop + rendering (reads const pointers) + animation ticking
2. **`sensor_feed` thread**: Reads firmware serial data, writes internal state
3. **`tpms_feed` thread**: Reads TPMS serial data, writes internal state

Each feed has exactly one writer thread and one reader (the render loop). All shared fields are naturally-aligned 32-bit int/float values, which are atomic on ARM. The worst case is a single render frame showing a mix of two consecutive serial updates — imperceptible at 60 fps. A mutex would add lock contention every frame for no visible benefit.

The animation system runs single-threaded on the main thread. `anim_tick_all()` is called once per frame to advance all registered animations based on elapsed time.

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

Each theme has its own directory under `assets/themes/`. All 9 themes are loaded into the asset store at startup. Textures are retrieved via `tex("Name.png")` for the current theme or `tex_from("theme", "Name.png")` for a specific theme.

## Key Constants

- Screen resolution: `1024x600` (`SCREEN_WIDTH`, `SCREEN_HEIGHT`)
- Serial baud rate: `115200`
- TPMS hardware types: `TPMS_MODEL_STANDARD (100)`, `TPMS_MODEL_EBAY (200)`
- Menu states: integer ranges mapped to `menu_screen` enum via `menu_screen_for_state()`

## Guidelines

- All source is C23 — no C++ features
- All rendering uses SDL3 textures created from PNG surfaces (loaded via stb_image) — no TTF fonts
- All assets are PNG — the BMP-to-PNG migration is complete
- The `sensor_feed` module tracks data staleness; the display shows a NO COMMS badge when data is stale
- When adding new visual elements, follow the existing sprite-sheet pattern with themed variants
- Test serial data parsing changes carefully — the message format must match the firmware exactly
- New subsystems should follow the established pattern: opaque struct, const accessors, integration tests

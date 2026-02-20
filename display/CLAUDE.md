# CLAUDE.md - TFT Dash Display

## Current Branch State

Branch `refactor/parser-extraction` has 12 commits ahead of `master`, not yet pushed. To push:

```bash
git push -u origin refactor/parser-extraction
```

Commits (oldest first):
1. Extract message parsing to testable C modules
2. Add support for tft-dash-simulator pipe
3. Add simulator usage documentation
4. Update build.sh to detect platform and build correctly
5. Reorganise BMPs into assets/themes structure
6. Move source files into src directory
7. Integrate new parser module into main app
8. Add Zig build system replacing manual g++ invocations
9. Complete asset reorganisation with thumbnails in theme directories
10. Tidied up a bit and added dummy CMakeLists
11. Refactor to snake_case naming convention throughout codebase
12. Add standalone asset manager module with themed texture lookup
13. Integrate asset store into testsdl.cpp, replacing 143 texture globals

All builds and tests pass (`zig build` and `zig build test`). See `REFACTORING.md` for remaining opportunities.

## Overview

This is the display application for the TFT Dash motorcycle dashboard. It runs on a Raspberry Pi, receives sensor data from the firmware over USB serial, and renders the dashboard GUI using SDL2 at 1024x600 resolution.

## Build

Uses Zig as the build system. The `build.zig` defines two targets: `testsdl` (main dashboard) and `test_parser` (parser test suite).

```bash
# Build everything
zig build

# Build and run parser tests
zig build test

# Build and run the dashboard
zig build run

# Convenience wrapper
./build.sh          # builds
./build.sh test     # builds and runs tests

# Cross-compile for Raspberry Pi (aarch64)
zig build -Dtarget=aarch64-linux-gnu
```

Binaries are output to `zig-out/bin/`.

### Dependencies

- Zig 0.15+ (`pacman -S zig` on Arch, or download from ziglang.org)
- SDL2 development libraries (`libsdl2-dev` on Raspberry Pi, `sdl2` on Arch)
- pthreads (linked via build.zig)

## Source Files

```
tftdashdisplay/
├── src/                    Source code
│   ├── testsdl.cpp        Main display application (~4800 lines)
│   ├── parser.h/c         Extracted parsing with lookup tables
│   ├── assets.h/c         Asset management with themed texture lookup
│   ├── test_parser.c      Parser test suite
│   ├── test_assets.c      Asset manager test suite
│   └── TPMSTest.cpp       TPMS test utility
├── assets/                 Graphics assets
│   └── themes/            BMP files organised by theme
├── build.zig              Zig build system configuration
├── build.zig.zon          Zig package manifest
├── build.sh               Convenience build wrapper
├── CLAUDE.md              Project documentation
├── REFACTORING.md         Refactoring progress log
└── SIMULATOR.md           Simulator integration guide
```

### Main Application
- **`src/testsdl.cpp`** (~6000 lines): The entire display application in a single file.
- **`src/TPMSTest.cpp`**: Standalone test utility for TPMS serial data.

### Refactored Modules
- **`src/parser.h/c`**: Extracted message parsing with data-driven lookup tables. Pure C, no SDL dependencies.
- **`src/assets.h/c`**: Asset management with two-key hash map `(theme, name)` → `SDL_Texture*`. Loads BMP files from theme directories. Integrated into `testsdl.cpp` replacing 143 globals with `tex()` / `tex_from()` lookups.
- **`src/test_parser.c`**: Comprehensive test suite for parsing logic (10 test cases, all passing).
- **`src/test_assets.c`**: Asset manager test suite (7 tests, all passing).

### Testing

```bash
# Build and run parser tests
zig build test
```

The parser has been extracted and tested independently of SDL/hardware. It uses a data-driven approach with field descriptor tables instead of if-ladders, providing O(1) field lookup and parsing each value exactly once.

## Architecture of `testsdl.cpp`

The file is structured in broad sections:

| Lines (approx.) | Section |
|---|---|
| 1-700 | Global state variables, `#define` constants, menu state definitions, asset store helpers |
| 700-1700 | Sprite/bitmap rendering helpers (numbers, gauges, indicators, nav icons) |
| 1700-2500 | `unpack_menu_message()` / `unpack_message()` - serial data deserialisation |
| 2500-2700 | `connect_interface()` / `poll_interface()` - serial port connection & read thread |
| 2700-2950 | TPMS interface - separate serial connection & polling thread |
| 3002+ | `draw_menu()` - on-screen menu system rendering |
| 3895+ | `draw_dashboard()` - main dashboard rendering |
| 4504+ | `main()` - SDL init, asset store init, thread spawning, event/render loop |

### Threading Model

Three pthreads, no explicit synchronisation:
1. **Main thread**: SDL event loop + rendering
2. **`pollInterface` thread**: Reads firmware serial data, updates globals
3. **`pollTPMSInterface`/`pollTPMSInterface2` thread**: Reads TPMS serial data

### Serial Protocol

The firmware sends comma-delimited messages framed by markers:
- **Live data**: `{,speed,rpm,coolant,battery,hour,minute,fuel,...,}`
- **Menu data**: `[,menustate,odo1,odo2,...,settings,...,]`

Parsing splits on commas after detecting `{` or `[` start markers.

### Serial Ports

- Linux: tries `/dev/ttyACM0`, `/dev/ttyACM1`
- macOS: tries `cu.usbserial-*`, `cu.usbmodem*`
- Baud rate: 115200

## Theming

Themes: default/white, green, red, blue, orange, yellow, night, bright (high-contrast), dark.

Each theme has its own directory under `assets/themes/` (e.g. `assets/themes/blue/Coolant.bmp`, `assets/themes/night/Revline.bmp`). The default theme is in `assets/themes/default/`.

Theme is selected via the on-screen menu and stored in EEPROM on the firmware side. Theme switching is instant — all 9 themes are loaded into the asset store at startup.

## BMP Assets

All graphical assets are BMP files in `assets/themes/<theme>/`. At startup, `asset_store_load_theme()` loads all 9 theme directories into a hash map. Textures are retrieved via `tex("Name.bmp")` for the current theme or `tex_from("theme", "Name.bmp")` for a specific theme.

## Key Constants

- Screen resolution: `1024x600` (`SCREEN_WIDTH`, `SCREEN_HEIGHT`)
- Serial baud rate: `115200`
- TPMS hardware types: `STANDARDTPMS (100)`, `EBAYTPMS (200)`
- Menu states use `#define` constants: major category (e.g. `SETUNITS = 4`) with sub-options as `category*100+offset` (e.g. `SETUNITSMILES = 401`)

## Navigation Constants

Navigation icon defines (`MUT`, `EXITL`, `EXITR`, `TNR`, `TNL`, `SLL`, `SLR`, etc.) map to indices in the navigation graphics sprite sheet.

## Guidelines

- This is a single-file C++ application. Keep it that way unless there is a strong reason to split.
- All rendering uses SDL2 textures created from BMP surfaces - no TTF fonts.
- When adding new visual elements, follow the existing sprite-sheet pattern with themed variants.
- Test serial data parsing changes carefully - the message format must match the firmware exactly.
- Global variables are shared across threads without mutexes. Be aware of potential race conditions when modifying shared state.

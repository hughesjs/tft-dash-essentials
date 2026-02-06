# CLAUDE.md - TFT Dash Display

## Overview

This is the display application for the TFT Dash motorcycle dashboard. It runs on a Raspberry Pi, receives sensor data from the firmware over USB serial, and renders the dashboard GUI using SDL2 at 1024x600 resolution.

## Build

```bash
# Raspberry Pi (primary target)
g++ -o testsdl testsdl.cpp -lSDL2 -lpthread

# macOS (for development)
g++ -o testsdl testsdl.cpp -I/Library/Frameworks/SDL2.framework/Headers -F/Library/Frameworks -framework SDL2
```

There is no Makefile. `build.sh` contains the macOS build command. Compilation is a single `g++` invocation.

### Dependencies

- SDL2 development libraries (`libsdl2-dev` on Raspberry Pi)
- pthreads (linked via `-lpthread`)

## Source Files

### Main Application
- **`testsdl.cpp`** (~6000 lines): The entire display application in a single file. No header files.
- **`TPMSTest.cpp`**: Standalone test utility for TPMS serial data.
- **`build.sh`**: macOS build script.

### Refactored Modules (New)
- **`parser.h/c`**: Extracted message parsing with data-driven lookup tables. Pure C, no SDL dependencies.
- **`test_parser.c`**: Comprehensive test suite for parsing logic (10 test cases, all passing).
- **`REFACTORING.md`**: Documentation of refactoring progress and architecture improvements.

### Testing

```bash
# Build and run parser tests
gcc -o test_parser test_parser.c parser.c -std=c99 -lm
./test_parser
```

The parser has been extracted and tested independently of SDL/hardware. It uses a data-driven approach with field descriptor tables instead of if-ladders, providing O(1) field lookup and parsing each value exactly once.

## Architecture of `testsdl.cpp`

The file is structured in broad sections:

| Lines (approx.) | Section |
|---|---|
| 1-700 | Global state variables, `#define` constants, menu state definitions |
| 700-3000 | Sprite/bitmap rendering helpers (numbers, gauges, indicators, nav icons) |
| 3007-3500 | `UnpackMenuMessage()` / `UnpackMessage()` - serial data deserialisation |
| 3524-3696 | `connectInterface()` / `pollInterface()` - serial port connection & read thread |
| 3698-3950 | TPMS interface - separate serial connection & polling thread |
| 4234+ | `Drawmenu()` - on-screen menu system rendering |
| 5137+ | `Drawdashboard()` - main dashboard rendering |
| 5746+ | `main()` - SDL init, thread spawning, event/render loop |

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

Each theme has a complete set of BMP sprite files with the theme name as prefix (e.g. `blue-Coolant.bmp`, `night-Revline.bmp`). Unprefixed BMPs are the default/white theme.

Theme is selected via the on-screen menu and stored in EEPROM on the firmware side.

## BMP Assets

All graphical assets are BMP files loaded as SDL surfaces at startup via `loadsurfaces()`. They **must** be in the same directory as the `testsdl` binary.

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

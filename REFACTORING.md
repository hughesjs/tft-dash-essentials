# TFT Dash Refactoring Progress

## Completed

### Parser extraction
- Extracted message parsing from `testsdl.cpp` into `parser.h/c`
- Data-driven field descriptor tables instead of if-ladders
- Tokenisation via `strsep` (C23 + `_GNU_SOURCE`)
- Comprehensive test suite in `test_parser.c` (10 tests, all passing)
- `zig build test` to run

### Asset manager extraction
- Standalone `assets.h/c` module for themed BMP loading
- Two-key hash map lookup: `(theme, asset_name)` → `SDL_Texture*`
- FNV-1a hashing, open addressing with linear probing, auto-resize at 70% load
- `asset_store_load_theme()` scans directories via `opendir`/`readdir`
- Test suite in `test_assets.c` (7 tests, all passing)
- `zig build test` runs both parser and asset tests

### Asset store integration into testsdl.cpp
- Replaced 143 global variables (72 `SDL_Surface*` + 71 `SDL_Texture*`) with 2 (`g_assets` + `g_current_theme`)
- Deleted `load_surface()`, `load_surfaces()`, and `init_textures()` (~1000 lines of boilerplate)
- All 9 themes loaded upfront at startup — no per-switch disk I/O
- Theme switching is now a string assignment instead of full surface reload + texture recreation
- `tex("Name.bmp")` helper for current-theme lookups, `tex_from("theme", "Name.bmp")` for fixed-theme
- Default-only assets (odometer setup, theme arrows) use `tex_from("default", ...)`
- Per-theme thumbnails use `tex_from("green", "greenthumb.bmp")` etc.
- ~1250 lines deleted, ~40 lines added

### snake_case naming convention
- All functions, globals, locals, struct types and members converted to snake_case
- Constants remain UPPER_CASE (already correct)

### Zig build system
- Replaced manual g++ invocations with `build.zig`
- Builds main app, parser tests, and handles asset copying
- Cross-compilation support for aarch64

## Display-side refactoring opportunities

Roughly in order of impact:

1. ~~**Texture loading boilerplate**~~ — **Done.** Replaced by asset store integration.

2. **Drawing function duplication** (~200 lines) — 7 near-identical `draw_*_string()` functions. Parameterise into one generic glyph renderer.

3. **Global state grouping** (~100 globals) — Group into structs: `bike_state`, `tpms_state`, `animation_state`, `display_strings`, etc.

4. ~~**Theme descriptor table**~~ — **Done.** `THEME_NAMES[]` array + `theme_name_from_id()` replaces all `if (theme == X)` ladders.

5. **Nav symbol lookup table** — Replace 20+ `strcmp()` chains with a data-driven table (same pattern as parser).

6. **Menu system extraction** — `draw_menu()` is 576 lines of nested ifs. Extract per-menu rendering functions.

7. **Warning badge priority** — Extract implicit nested-if priority logic into an explicit priority function.

## Firmware architectural change (future option)

### Current state

The firmware handles both signal acquisition AND application logic:
- Reads sensors, averages, converts to meaningful units
- Manages the entire menu state machine (which menu, which digit being edited, cycling values)
- Stores all settings in EEPROM (theme, units, sprockets, fan temp, TPMS, etc.)
- Controls the fan relay
- Tracks odometer/trips and persists to EEPROM
- Manages RTC

The display just renders whatever state the firmware sends. The protocol is one-directional (firmware to display).

### Proposal: move menu logic to the display

Make the firmware a sensor bridge. It reads hardware, outputs values, and accepts setting commands from the display.

**Stays on firmware (hardware-dependent):**
- Sensor reading, averaging, signal processing
- Fan relay control (safety-critical — must work if display crashes)
- Odometer/trip persistence in EEPROM (SD card too fragile for frequent writes on a motorcycle)
- RTC read/write
- Speed correction (must happen before odometer accumulation)

**Moves to display:**
- Menu state machine and navigation
- Theme/unit preferences (become Pi-side config, possibly a file)
- All menu digit editing state (odo digits, time digits, speed correction digits)

**New protocol:**

```
Firmware -> Display (sensor data only):
  {,speed,rpm,coolant,batt,fuel,neutral,oil,highbeam,left,right,
   gear,ambient_temp,hour,minute,odo,trip1,trip2,oil_press,oil_temp,
   fan_state,button_option,button_select,}

Display -> Firmware (commands):
  CMD:RESET_TRIP1
  CMD:RESET_TRIP2
  CMD:SET_SPD_CORRECT:-2.5
  CMD:SET_TIME:14:23
  CMD:SET_SPROCKET_FRONT:15
  CMD:SET_SPROCKET_REAR:44
  CMD:SET_FAN_TEMP:95
  CMD:SET_FAN_MODE:2
  CMD:SET_GEAR_INTERVAL:200
  CMD:SAVE_ODO
```

**Benefits:**
- Firmware becomes much simpler — no menu state machine, no digit editor state
- Serial protocol shrinks — no more sending 6 odo digits, 4 time digits, etc.
- Adding/changing menus only requires display changes
- Display-only settings (theme, units) don't round-trip through the firmware
- Button events are raw — display decides what they mean in context

**Costs:**
- Bidirectional serial (trivial — `Serial.read()` on Arduino, `write()` on Pi)
- Settings persistence needs thought — EEPROM for hardware settings, file for display settings
- If display is down, buttons do nothing (but they're useless without a screen anyway)

**Implementation path:**
1. Add `Serial.read()` command handler to firmware
2. Strip menu state machine from firmware
3. Implement menu logic on display side
4. Simplify serial protocol
5. Add display-side config file for theme/units/layout

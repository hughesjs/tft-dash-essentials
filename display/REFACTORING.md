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

### Drawing function deduplication
- Replaced 7 near-identical `draw_*_string()` functions (~270 lines) with one `draw_glyph_string()` renderer (~20 lines)
- Data-driven `glyph_font` struct captures all differences: texture name, sprite atlas coords, glyph character set, case-insensitive matching, inter-glyph spacing, whitespace handling, clipping, and per-glyph y-offsets
- 8 font descriptors defined as static const: `FONT_SMALL_GREY`, `FONT_SMALL_BLUE`, `FONT_MEDIUM`, `FONT_LARGE`, `FONT_NAV_LARGE`, `FONT_NAV_SMALL`, `FONT_NAV_DIGITS`
- All existing function names kept as one-line wrappers (86 call sites untouched)
- `draw_speed_digit` left as-is (renders to pre-positioned destination rects, not a flowing string)

### snake_case naming convention
- All functions, globals, locals, struct types and members converted to snake_case
- Constants remain UPPER_CASE (already correct)

### Zig build system
- Replaced manual g++ invocations with `build.zig`
- Builds main app, parser tests, and handles asset copying
- Cross-compilation support for aarch64

## Display-side refactoring opportunities

### Sensor feed extraction
- Extracted serial connection, byte framing, and message parsing into `sensor_feed.h/c`
- Opaque `sensor_feed` struct — consumers get `const` pointers, can't write state
- Replaced ~50 global variables with `dash->`, `menu->`, `nav->` struct field access
- Deleted ~1400 lines from testsdl.cpp: `connect_interface()`, `pollInterface()`, `unpack_message()`, `unpack_menu_message()`, `unpack_nav_message()`, `choice()`, `select()`, test scenario functions
- 9 integration tests via named pipes in `test_sensor_feed.c`
- testsdl.cpp reduced from ~4400 to ~3070 lines

### Next: further extractions
The same pattern can be applied to:

1. **Extract the menu system** — `draw_menu()` (~500 lines of nested ifs) takes explicit input/output.
2. **Extract TPMS** — small, self-contained, same pattern.
3. **What remains in testsdl.cpp** is `main()`, the render loop, and drawing helpers — a thin orchestrator.

### Smaller wins (do whenever convenient)

1. ~~**Texture loading boilerplate**~~ — **Done.** Replaced by asset store integration.

2. ~~**Theme descriptor table**~~ — **Done.** `THEME_NAMES[]` array + `theme_name_from_id()` replaces all `if (theme == X)` ladders.

3. ~~**Nav symbol lookup table**~~ — **Done.** Replaced 17 strcmp chains with `nav_symbol_entry` lookup table + `nav_icon` enum.

4. ~~**Warning badge priority**~~ — **Done.** Replaced nested if/else staircase with `resolve_warning()` lambda returning a `warning_badge` struct. Priority order is now a flat list of early-returns.

5. **Nav distance protocol** — The phone app sends yards AND miles as separate fields, and the display picks which to show based on `nav_yards <= 300`. This split of responsibility is daft — the phone app knows the distance, it should send one value and let the display format it. Requires a phone app + protocol change.

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

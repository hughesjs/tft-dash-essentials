# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

TFT Dash is a motorcycle dashboard replacement project. It consists of two main software components plus hardware designs:

- **Firmware** (`firmware/`): Arduino code running on an ATMega32u4 (Gen4) or ATMega2560 (Mega/Gen3) that reads bike sensor signals (speed, RPM, coolant temp, fuel level, indicators, etc.) and streams comma-delimited data over USB serial at 115200 bps.
- **Display** (`display/`): C++ application running on a Raspberry Pi that receives the serial data and renders the dashboard GUI using SDL3 at 1024x600 resolution.
- **Hardware** (`hardware/`): EAGLE schematic/board files for the Gen4 interface PCB, plus 3D-printable enclosure STLs.
- **Pi Image** (`pi-image/`): Configuration files from the Raspberry Pi 3 SD card — boot config, update script, fstab. The Pi runs a read-only RetroPie-based image with WiFi/BT disabled, DPI display at 1024x600. App lives at `/home/pi/tftdash/TFTDash/` in flat BMP layout.

## Build Commands

### Display Software
```bash
cd display

# Build (requires zig 0.15+ and SDL3 dev libraries)
zig build

# Build and run parser tests
zig build test

# Cross-compile for Raspberry Pi (aarch64)
zig build -Dtarget=aarch64-linux-gnu -Doptimize=ReleaseSafe
```

Uses Zig as the build system. See `display/build.zig` for configuration.

### Firmware
Open `firmware/tftdashfirmwareGEN4.ino` in the Arduino IDE. Configuration is controlled by compiler defines at the top of the file:

**Board selection** (uncomment one):
- `#define GEN4BOARD` - ATMega32u4 (Leonardo-compatible) Gen4 board
- `#define MEGABOARD` - ATMega2560 Gen3 board

**Bike model selection** (uncomment one):
- `#define BIKE_FZS1000` - Yamaha FZS1000 Fazer (original)
- `#define BIKE_FZS600` - Yamaha FZS600 Fazer

This controls gear ratios, primary drive ratio, wheel diameter, and RPM calibration constant. See `docs/signal-reverse-engineering.md` for full details of all bike-specific constants and signal formats.

Pre-compiled hex files are also provided in the same directory.

### SD Card Image (Buildroot)
```bash
cd buildroot-tftdash

# First build (20-30 mins, downloads toolchain + kernel)
./build.sh

# Rebuild after app/asset changes
./build.sh rebuild
```

Requires Buildroot cloned alongside the repo (`git clone https://github.com/buildroot/buildroot.git ../buildroot-src`) and the display binary pre-built for ARM (see above). Output is `../buildroot-src/output/images/sdcard.img`. See `buildroot-tftdash/README.md` for full details.

## Architecture

### Serial Protocol
The firmware outputs two types of comma-delimited messages continuously:

- **Live data** `{,speed,rpm,coolant,battery,hour,minute,fuel,...,}` - real-time sensor readings
- **Menu data** `[,menustate,odo1,odo2,...,settings,...,]` - menu state and configuration values

The display's `pollInterface()` thread reads serial data byte-by-byte, frames messages by looking for `{` or `[` start markers, then calls `UnpackMessage()` or `UnpackMenuMessage()` to parse fields by splitting on commas.

### Display Application (`testsdl.cpp`)
Single ~6000 line file with this structure:

- **Global state** (lines 1-700): All dashboard state as global variables - speed, RPM, temps, indicators, theme, menu state, TPMS sensor data
- **Sprite/bitmap rendering helpers** (lines 700-3000): Functions to render numbers, gauges, indicators, and navigation icons using BMP sprite sheets
- **`UnpackMenuMessage()` / `UnpackMessage()`** (lines 3007-3500): Deserialise the comma-delimited serial strings into global state variables
- **`connectInterface()` / `pollInterface()`** (lines 3524-3696): Serial port connection (tries `/dev/ttyACM0`, `/dev/ttyACM1` on Linux; `cu.usbserial-*`, `cu.usbmodem*` on macOS) and background read thread
- **TPMS interface** (lines 3698-3950): Separate serial connection and polling thread for tyre pressure monitoring sensors (supports two TPMS hardware models: `STANDARDTPMS` and `EBAYTPMS`)
- **`Drawmenu()`** (line 4234): Renders on-screen menu system for settings (trip reset, units, time, theme, sprocket ratio, coolant fan temp, TPMS, odometer)
- **`Drawdashboard()`** (line 5137): Main rendering - speed, RPM bar, coolant, fuel gauge, indicators, warnings, info panels, navigation overlay
- **`main()`** (line 5746): Initialises SDL, spawns serial polling threads, loads themed BMP surfaces, runs the main event/render loop

### Threading Model
The display uses pthreads with three threads:
1. **Main thread**: SDL event loop + rendering
2. **`pollInterface` thread**: Reads firmware serial data and updates global state
3. **`pollTPMSInterface`/`pollTPMSInterface2` thread**: Reads TPMS serial data

Global variables are shared between threads without explicit synchronisation.

### Theming
The display supports multiple colour themes (default/white, green, red, blue, orange, yellow, night, high-contrast). Each theme has a complete set of BMP sprite files prefixed with the theme name (e.g. `blue-Coolant.bmp`). Theme is selected via the on-screen menu and stored in EEPROM on the firmware side.

### BMP Assets
All graphical assets are BMP files that must be in the same directory as the `testsdl` binary. They are loaded as SDL surfaces and converted to textures at startup via `loadsurfaces()`.

## Key Constants
- Screen resolution: 1024x600
- Serial baud rate: 115200
- Menu states are defined as `#define` constants with a naming convention: major category (e.g. `SETUNITS = 4`) and sub-options as category*100+offset (e.g. `SETUNITSMILES = 401`)

## Planned: OTA Update System (WiFi AP Mode)

Design for wireless update delivery without USB sticks. See `docs/signal-reverse-engineering.md` for existing update package format.

### Architecture

The Pi temporarily becomes a WiFi access point. A phone app connects and pushes the update directly. BLE (or a physical button combo) triggers update mode.

```
Phone App                    Arduino (BLE)              Pi
─────────                    ─────────────              ──
1. Tap "Update"
2. Send BLE ──────────────> Relay ──────────────> Start hostapd AP
   "START_UPDATE"                                  "TFTDash" network
                                                   Start HTTP server
                                                   on 192.168.4.1:8080

3. Join "TFTDash" network
   (iOS: NEHotspotConfiguration
    with system dialog)

4. POST update zip ─────── WiFi direct to Pi ───> Receive, extract,
   to 192.168.4.1:8080                            apply update

5. Pi responds 200 OK ──── WiFi ────────────────> Tear down AP
                                                   Restart dash
6. Phone auto-rejoins
   previous network
```

### Why This Approach
- **BLE alone is too slow** — update package is ~10MB zipped, 330MB uncompressed. BLE tops out at 10-30 KB/s.
- **Pi as AP avoids phone hotspot dependency** — iOS has no public API to enable Personal Hotspot programmatically. Pi-hosted AP works identically on iOS and Android.
- **Fixed known IP** — Pi is always 192.168.4.1 as the AP gateway. No discovery needed.
- **BLE only carries control messages** — "start update mode" / "disconnect". Existing BLE relay through Arduino handles this already.

### Implementation Plan

#### Pi-Side Update Daemon
- Small service/script that listens for update trigger (BLE command via serial, or physical button combo like hold option+select for 5 seconds)
- On trigger: start `hostapd` + `dnsmasq` to create "TFTDash" AP on 192.168.4.1
- Run HTTP server on port 8080 accepting POST to `/update`
- On receiving zip: validate, extract to staging area, apply (copy binary + assets, optionally flash firmware via avrdude), tear down AP, restart dashboard
- Could be ~50 lines of bash wrapping a small HTTP receiver

#### Firmware Changes
- New BLE message type: `<UPD:START%>` triggers update mode
- Arduino relays to Pi via existing serial passthrough (no firmware changes needed beyond recognising the prefix)

#### Phone App
- "Update Dash" button bundles the update zip
- Sends BLE `START_UPDATE` command
- Uses `NEHotspotConfiguration` (iOS) to join "TFTDash" network — shows one system confirmation dialog
- POSTs zip to `http://192.168.4.1:8080/update`
- Shows progress bar
- Handles success/failure response

#### Pi Requirements
- `hostapd` and `dnsmasq` packages installed on Pi SD card image
- WiFi hardware (Pi 3+ has built-in)
- Pre-configured hostapd.conf for the "TFTDash" AP

### Existing Update Package (USB Stick)
The current update mechanism uses a USB stick with a `tftupdate/` directory. A boot script on the Pi's SD card detects it and copies files. Structure:
```
tftupdate/
├── bootimage.png              Fazer splash screen
├── firmware/
│   ├── firmware.hex           Mega board hex
│   ├── firmware4.hex          Gen4 board hex
│   └── avrdude.conf           avrdude config for flashing
└── new/
    ├── testsdl                ARM 32-bit binary (display app)
    ├── testsdl.cpp            Source (for reference)
    └── *.bmp (×855)           All theme BMPs, flat layout with prefixes
```
The exact boot script is on the Pi's SD card image (not in this repo yet — pull the SD card to retrieve it).

### TODO
- [ ] Pull Pi SD card and retrieve the existing update/boot script
- [ ] Write `make-update-package.sh` to match the exact format the Pi expects
- [ ] Calibrate RPM_CONSTANT for FZS600 (warm idle should read 1150-1250 RPM)
- [ ] Recalibrate fuel level lookup table for FZS600 tank (19.4L vs 21L, different shape)

# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

TFT Dash is a motorcycle dashboard replacement project with four main components:

- **Display** (`display/`): Pure C23 application running on a Raspberry Pi that receives serial data and renders the dashboard GUI using SDL3 at 1024x600 resolution. See `display/CLAUDE.md` for detailed module-level documentation.
- **Firmware** (`firmware/`): Arduino code running on an ATMega32u4 (Gen4 board) that reads bike sensor signals (speed, RPM, coolant temp, fuel level, indicators, etc.) and streams comma-delimited data over USB serial at 115200 bps.
- **Simulator** (`simulator/`): .NET 10 / Avalonia desktop GUI for development without hardware. Provides interactive controls for all dashboard state, preset scenarios, and writes to a FIFO pipe that the display reads as if it were a serial port.
- **Hardware** (`hardware/`): EAGLE schematic/board files for the Gen4 interface PCB, 3D-printable enclosure STLs, and component datasheets.

## Build Commands

A root `Makefile` provides unified build targets. Run `make help` for the full list.

```bash
make display       # Build display natively for dev/testing
make firmware      # Compile firmware .hex via arduino-cli
make simulator     # Build the .NET simulator
make test          # Run all tests (display + simulator)
make               # Full SD card image (firmware + display cross-compile + buildroot)
make rebuild       # Rebuild image after app/asset changes
make update-package # Build USB update package
make qemu          # Boot image in QEMU from slot A
make qemu-b        # Boot from slot B (A/B update testing)
make setup         # First-time Arduino board support + libraries
```

### Display Software
```bash
cd display && zig build           # Build natively (requires zig 0.15+)
cd display && zig build test      # Run all tests (62 tests across 6 suites)
cd display && zig build -Dtarget=aarch64-linux-gnu -Doptimize=ReleaseSafe  # Cross-compile for Pi
```

### Firmware
Open `firmware/firmware.ino` in the Arduino IDE, or compile via `firmware/build.sh build`.

**Bike model selection** (uncomment one at the top of the file):
- `#define BIKE_FZS1000` - Yamaha FZS1000 Fazer (original)
- `#define BIKE_FZS600` - Yamaha FZS600 Fazer

This controls gear ratios, primary drive ratio, wheel diameter, and RPM calibration constant. See `docs/signal-reverse-engineering.md` for full details of all bike-specific constants and signal formats.

### Simulator
```bash
# Build
dotnet build simulator/src/TftDashSimulator/TftDashSimulator.csproj

# Run GUI only (no pipe output)
dotnet run --project simulator/src/TftDashSimulator/TftDashSimulator.csproj

# Run with FIFO pipe output to /tmp/tft-dash-pipe (feeds the display)
dotnet run --project simulator/src/TftDashSimulator/TftDashSimulator.csproj -- --pipe

# Run tests
dotnet test simulator/tests/TftDashSimulator.Tests/
```

The simulator creates a FIFO at `/tmp/tft-dash-pipe` and writes protocol messages at 10 Hz. The display's `sensor_feed` module auto-detects this pipe and reads from it. A "Pause Comms" toggle button stops sending to test the stale data warning.

### SD Card Image (Buildroot)
```bash
cd buildroot-tftdash
./build.sh          # First build (20-30 mins, downloads toolchain + kernel)
./build.sh rebuild  # Rebuild after app/asset changes
```

Requires Buildroot cloned alongside the repo (`git clone https://github.com/buildroot/buildroot.git ../buildroot-src`). Output is `../buildroot-src/output/images/sdcard.img`. See `buildroot-tftdash/README.md` for full details.

## Architecture

### Serial Protocol
The firmware outputs two types of comma-delimited messages continuously:

- **Live data** `{,speed,rpm,coolant,battery,hour,minute,fuel,...,}` - real-time sensor readings
- **Menu data** `[,menustate,odo1,odo2,...,settings,...,]` - menu state and configuration values

The display's `sensor_feed` thread reads serial data byte-by-byte, frames messages by looking for `{` or `[` start markers, then calls `parse_live_message()` or `parse_menu_message()` to parse fields via data-driven descriptor tables.

### Display Application (`main.c` + modules)
Pure C23 codebase. The main file (~2200 lines) contains the render loop and drawing helpers. Subsystems are extracted into modules:

- **`parser.h/c`** — Serial message deserialisation with data-driven field descriptor tables
- **`assets.h/c`** — Themed PNG texture loading (via stb_image) with two-key hash map lookup
- **`sensor_feed.h/c`** — Owns serial/pipe connection, background thread for firmware data, and comms staleness tracking
- **`menu.h/c`** — Menu screen routing, cursor positions, and theme thumbnail lookup tables
- **`tpms_feed.h/c`** — TPMS serial connection and binary protocol decoding (Standard + eBay)
- **`animation.h/c`** — Data-driven animation engine (oneshot, ping-pong, loop, chase modes) with global registry
- **`dashboard_anims.h/c`** — Concrete animation instances (startup, flash, info transition, RPM) and data tables
- **`draw.h`** — Shared rendering state and drawing primitives (in progress — being extracted from main.c)

### Threading Model
Three pthreads:
1. **Main thread**: SDL event loop + rendering (reads const pointers) + animation ticking (`anim_tick_all()` each frame)
2. **`sensor_feed` thread**: Reads firmware serial data, writes internal state
3. **`tpms_feed` thread**: Reads TPMS serial data, writes internal state

### Theming
9 colour themes (default/white, green, red, blue, orange, yellow, night, bright, dark). Each theme has its own directory under `assets/themes/`. All themes loaded into the asset store at startup. Textures retrieved via `tex("Name.png")` or `tex_from("theme", "Name.png")`.

### PNG Assets
All graphical assets are PNG files in `assets/themes/<theme>/`. Loaded at startup via stb_image through `asset_store_load_theme()` into a hash map.

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

### A/B Rootfs Update System

The SD card uses a 4-partition layout for atomic updates:

```
p1  boot      32MB   FAT32   kernel, DTBs, firmware, config.txt, cmdline.txt
p2  rootfs_a  512MB  ext4    active Buildroot rootfs (read-only)
p3  rootfs_b  512MB  ext4    standby — dd target for updates
p4  data      16MB   ext4    persistent user data (rw)
```

**Update flow**: Insert USB stick with `tftupdate/rootfs.img` → `S10usbupdate` init script detects it at boot → `tft-update.sh` writes image to standby partition, flips `cmdline.txt` root=, reboots into new slot. If the new image is bad, the old slot remains intact.

**Key files**:
- `buildroot-tftdash/overlay/usr/bin/tft-update.sh` — core update script
- `buildroot-tftdash/overlay/etc/init.d/S10usbupdate` — boot-time USB scan
- `buildroot-tftdash/board/genimage.cfg` — 4-partition SD card layout

**USB update package format** (built via `make update-package`):
```
tftupdate/
├── rootfs.img            Full ext4 rootfs image (dd'd to standby partition)
├── rootfs.img.sha256     SHA-256 checksum for verification
└── firmware.hex          Optional Arduino firmware (flashed via avrdude)
```

## Documentation (`docs/`)

- **`SERIAL-PROTOCOL.md`** — Complete serial protocol specification: live data (37 fields), menu data, TPMS binary protocol, navigation message format
- **`signal-reverse-engineering.md`** — Electrical signal details: speed sensor, RPM counter, analogue signals (temp, fuel, battery), digital signals (neutral, oil, indicators), calibration constants, FZS600 variations
- **`ab-update-plan.md`** — A/B rootfs update system design
- **`ota-update-plan.md`** — Over-the-air WiFi update architecture (planned)
- **`gen5-arm-cortex-plan.md`** — Future ARM Cortex M4 board design notes

### TODO
- [ ] Calibrate RPM_CONSTANT for FZS600 (warm idle should read 1150-1250 RPM)
- [ ] Recalibrate fuel level lookup table for FZS600 tank (19.4L vs 21L, different shape)

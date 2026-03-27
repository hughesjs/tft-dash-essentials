# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

TFT Dash is a motorcycle dashboard replacement project. It consists of two main software components plus hardware designs:

- **Firmware** (`firmware/`): Arduino code running on an ATMega32u4 (Gen4 board) that reads bike sensor signals (speed, RPM, coolant temp, fuel level, indicators, etc.) and streams comma-delimited data over USB serial at 115200 bps.
- **Display** (`display/`): C++ application running on a Raspberry Pi that receives the serial data and renders the dashboard GUI using SDL3 at 1024x600 resolution.
- **Hardware** (`hardware/`): EAGLE schematic/board files for the Gen4 interface PCB, plus 3D-printable enclosure STLs.

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
Open `firmware/firmware.ino` in the Arduino IDE, or compile via `firmware/build.sh build`.

**Bike model selection** (uncomment one at the top of the file):
- `#define BIKE_FZS1000` - Yamaha FZS1000 Fazer (original)
- `#define BIKE_FZS600` - Yamaha FZS600 Fazer

This controls gear ratios, primary drive ratio, wheel diameter, and RPM calibration constant. See `docs/signal-reverse-engineering.md` for full details of all bike-specific constants and signal formats.

### SD Card Image (Buildroot)
```bash
cd buildroot-tftdash

# First build (20-30 mins, downloads toolchain + kernel)
./build.sh

# Rebuild after app/asset changes
./build.sh rebuild

# Build a USB update package for A/B updates
./build.sh update-package
```

Requires Buildroot cloned alongside the repo (`git clone https://github.com/buildroot/buildroot.git ../buildroot-src`) and the display binary pre-built for ARM (see above). Output is `../buildroot-src/output/images/sdcard.img`. See `buildroot-tftdash/README.md` for full details.

### A/B Update Testing
```bash
# Boot QEMU from slot A (default)
make qemu

# Boot QEMU from slot B
make qemu-b

# Inside QEMU, test update script:
dd if=/dev/zero of=/tmp/test.img bs=1M count=10
tft-update.sh /tmp/test.img
```

## Architecture

### Serial Protocol
The firmware outputs two types of comma-delimited messages continuously:

- **Live data** `{,speed,rpm,coolant,battery,hour,minute,fuel,...,}` - real-time sensor readings
- **Menu data** `[,menustate,odo1,odo2,...,settings,...,]` - menu state and configuration values

The display's `pollInterface()` thread reads serial data byte-by-byte, frames messages by looking for `{` or `[` start markers, then calls `UnpackMessage()` or `UnpackMenuMessage()` to parse fields by splitting on commas.

### Display Application (`main.c` + modules)
Pure C23 codebase. The main file (~2200 lines) contains the render loop and drawing helpers. Subsystems are extracted into modules:

- **`parser.h/c`** — Serial message deserialisation with data-driven field descriptor tables
- **`assets.h/c`** — Themed BMP texture loading with two-key hash map lookup
- **`sensor_feed.h/c`** — Owns serial/pipe connection and background thread for firmware data
- **`menu.h/c`** — Menu screen routing, cursor positions, and theme thumbnail lookup tables
- **`tpms_feed.h/c`** — TPMS serial connection and binary protocol decoding (Standard + eBay)

### Threading Model
Three pthreads:
1. **Main thread**: SDL event loop + rendering (reads const pointers)
2. **`sensor_feed` thread**: Reads firmware serial data, writes internal state
3. **`tpms_feed` thread**: Reads TPMS serial data, writes internal state

### Theming
9 colour themes (default/white, green, red, blue, orange, yellow, night, bright, dark). Each theme has its own directory under `assets/themes/`. All themes loaded into the asset store at startup. Textures retrieved via `tex("Name.bmp")` or `tex_from("theme", "Name.bmp")`.

### BMP Assets
All graphical assets are BMP files in `assets/themes/<theme>/`. Loaded at startup via `asset_store_load_theme()` into a hash map.

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

### TODO
- [ ] Calibrate RPM_CONSTANT for FZS600 (warm idle should read 1150-1250 RPM)
- [ ] Recalibrate fuel level lookup table for FZS600 tank (19.4L vs 21L, different shape)

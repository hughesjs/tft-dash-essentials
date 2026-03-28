# TFT Dash

A motorcycle dashboard replacement system. An Arduino-based interface board reads sensor signals from the bike — speed, RPM, coolant temperature, fuel level, battery voltage, indicators, and more — and streams data over USB serial to a Raspberry Pi, which renders a real-time graphical dashboard using SDL3.

## System Architecture

```
Phone App (BLE) ──> Arduino Firmware ──> USB Serial ──> Raspberry Pi Display
                    (ATMega32u4)          115200 bps     (SDL3, 1024x600)
                                                               |
                                                    TPMS Sensors (USB Serial)
```

The firmware outputs comma-delimited messages at 115200 bps. The display application deserialises these into dashboard state and renders at 60 fps. A separate TPMS receiver provides tyre pressure data via a second serial connection.

## Display Software Architecture

```
main.c (init, event loop, rendering)
  ├── draw.h          shared rendering state and drawing primitives (in progress)
  ├── dashboard_anims.h/c   animation instances and data tables
  │   └── animation.h/c     generic data-driven animation engine
  ├── sensor_feed.h/c       firmware serial data + comms staleness tracking
  ├── tpms_feed.h/c         tyre pressure serial data
  ├── parser.h/c             serial message deserialisation
  ├── assets.h/c             PNG texture management (stb_image)
  └── menu.h/c               menu screen data tables
```

Pure C23 codebase. Three threads: main (render + animation ticking), sensor feed (firmware serial), and TPMS feed (tyre pressure serial). 62 tests across 6 suites.

## Repository Structure

| Directory | Description |
|---|---|
| `display/` | Display application (C23/SDL3) for Raspberry Pi |
| `firmware/` | Arduino firmware for the sensor interface board (ATMega32u4) |
| `hardware/` | EAGLE schematic/board files for Gen4 PCB + 3D-printable enclosure STLs |
| `simulator/` | .NET/Avalonia desktop simulator for development without hardware |
| `buildroot-tftdash/` | Buildroot external tree for building the Pi SD card image |
| `docs/` | Protocol specs, signal reverse engineering notes, update system design |

## Quick Start

```bash
# Build the display application (requires Zig 0.15+)
make display

# Run all tests
make test

# Build the firmware (requires arduino-cli)
make firmware

# Build the simulator (requires .NET 10)
make simulator

# Full SD card image (firmware + display cross-compile + Buildroot)
make

# See all targets
make help
```

### Bike Model Selection

Uncomment one define at the top of `firmware/firmware.ino`:

- `#define BIKE_FZS1000` — Yamaha FZS1000 Fazer
- `#define BIKE_FZS600` — Yamaha FZS600 Fazer

This controls gear ratios, primary drive ratio, wheel diameter, and RPM calibration.

## Features

- Real-time speed, RPM, coolant temperature, battery voltage, and fuel level
- Gear position indicator (calculated from sprocket ratios)
- Turn indicator, high beam, neutral, and oil warning lights
- TPMS (tyre pressure monitoring) with two hardware variants
- Navigation overlay with turn-by-turn directions
- 9 colour themes with automatic day/night switching
- On-screen menu system for configuration
- NO COMMS warning when serial data goes stale
- Data-driven animation system (startup sequence, RPM effects, transitions)
- A/B rootfs update system for atomic SD card updates

## Documentation

See `docs/` for detailed documentation:

- **`SERIAL-PROTOCOL.md`** — Complete serial protocol specification
- **`signal-reverse-engineering.md`** — Electrical signal details and calibration constants
- **`ab-update-plan.md`** — A/B rootfs update system design
- **`ota-update-plan.md`** — Over-the-air WiFi update architecture (planned)

## Licence

2018-2025 TFTDashProject / Danny Draper.

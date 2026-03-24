# TFT Dash

A motorcycle dashboard replacement system comprising Arduino-based sensor firmware and a Raspberry Pi display application.

The firmware reads signals from the bike (speed, RPM, coolant temperature, fuel level, battery voltage, indicators, neutral, oil, high beam, and more) via a dedicated interface board, and streams the data over USB serial. The display application renders a real-time graphical dashboard using SDL3.

## Repository Structure

| Directory | Description |
|---|---|
| `display/` | Display software (C++/SDL3) for Raspberry Pi |
| `firmware/` | Arduino firmware for the sensor interface board (ATMega32u4) |
| `hardware/` | EAGLE schematic and board files for the Gen 4 (SMD, ATMega32u4) PCB |
| `hardware/3DModels/` | STL files for 3D printing the enclosure |
| `buildroot-tftdash/` | Buildroot external tree for building the Pi SD card image |
| `simulator/` | Simulator for development without hardware |
| `docs/` | Documentation (serial protocol, signal reverse engineering, etc.) |

## Prerequisites

### Display Software
- Raspberry Pi (or any Linux system for development)
- Zig 0.15+ (build system and cross-compilation)

### Firmware
- [Arduino IDE](https://www.arduino.cc/en/software) or `arduino-cli`
- Eeprom24C32_64 and RTClib Arduino libraries
- ATMega32u4 (Arduino Leonardo compatible) Gen4 board

### Hardware Design
- [Autodesk EAGLE](https://www.autodesk.com/products/eagle/overview) for `.brd` and `.sch` files

## Building

### Display Software

On Raspberry Pi / Linux:

```bash
cd display
zig build
```

On macOS:

```bash
cd display
zig build
```

The compiled binary must be run from the `display/` directory, as BMP assets need to be accessible from the working directory.

### Firmware

```bash
# Compile via arduino-cli
firmware/build.sh build

# Or open firmware/firmware.ino in the Arduino IDE and upload
```

Select the bike model at the top of `firmware.ino` by uncommenting the appropriate define:
- `#define BIKE_FZS1000` for Yamaha FZS1000 Fazer
- `#define BIKE_FZS600` for Yamaha FZS600 Fazer

## How It Works

The system operates as two components connected via USB serial at 115200 baud:

1. **Firmware** reads analogue and digital sensor inputs from the bike, processes them (averaging, calibration, unit conversion), and continuously outputs comma-delimited data strings over USB serial.

2. **Display** runs on the Raspberry Pi, receives serial data on a background thread, deserialises it into dashboard state, and renders the GUI on the main thread at 1024x600 resolution.

### Serial Data Format

The firmware outputs two message types:

- **Live data**: `{,speed,rpm,coolant,battery,hour,minute,fuel,neutral,oil,highbeam,left,right,...,}` - real-time sensor readings
- **Menu data**: `[,menustate,odo1,...,settings,...,]` - menu navigation state and configuration values

### Features

- Real-time speed, RPM, coolant temperature, battery voltage, and fuel level display
- Turn indicator, high beam, neutral, and oil warning lights
- Trip computer with two independent trip meters
- Odometer
- Gear position indicator (calculated from sprocket ratios)
- Coolant fan temperature control
- TPMS (Tyre Pressure Monitoring System) support with two hardware variants
- Navigation overlay with turn-by-turn directions
- Multiple colour themes (white, green, red, blue, orange, yellow, night, high contrast)
- Automatic day/night theme switching via ambient light sensor
- On-screen menu system for configuration (units, time, speed correction, sprocket sizes, etc.)
- KM/H and MPH unit support, Celsius and Fahrenheit, PSI and Bar

## Contact

- ddraper.cedesoft@gmail.com
- support@tftdashproject.co.uk
- http://www.tftdashproject.co.uk

## Licence

2018-2024 TFTDashProject / Danny Draper.

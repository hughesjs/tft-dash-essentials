# TFT Dash

A motorcycle dashboard replacement system comprising Arduino-based sensor firmware and a Raspberry Pi display application.

The firmware reads signals from the bike (speed, RPM, coolant temperature, fuel level, battery voltage, indicators, neutral, oil, high beam, and more) via a dedicated interface board, and streams the data over USB serial. The display application renders a real-time graphical dashboard using SDL2.

## Repository Structure

| Directory | Description |
|---|---|
| `tftdashdisplay/` | Display software (C++/SDL2) for Raspberry Pi |
| `tftdashfirmwareGEN4/` | Arduino firmware for the sensor interface board |
| `HardwareGen3/` | EAGLE schematic and board files for the Gen 3 (through-hole) PCB |
| `HardwareGen4/` | EAGLE schematic and board files for the Gen 4 (SMD, ATMega32u4) PCB |
| `3DModels/` | STL files for 3D printing the enclosure |

## Prerequisites

### Display Software
- Raspberry Pi (or any Linux system for development)
- SDL2 development libraries (`libsdl2-dev`)
- GNU C++ compiler (`g++`)

### Firmware
- [Arduino IDE](https://www.arduino.cc/en/software)
- Eeprom24C32_64 and RTClib Arduino libraries
- Gen 4 board: ATMega32u4 (Arduino Leonardo compatible)
- Gen 3 board: ATMega2560 (Arduino Mega compatible)

### Hardware Design
- [Autodesk EAGLE](https://www.autodesk.com/products/eagle/overview) for `.brd` and `.sch` files

## Building

### Display Software

On Raspberry Pi / Linux:

```bash
cd tftdashdisplay
g++ -o testsdl testsdl.cpp -lSDL2 -lpthread
```

On macOS:

```bash
cd tftdashdisplay
g++ -o testsdl testsdl.cpp -I/Library/Frameworks/SDL2.framework/Headers -F/Library/Frameworks -framework SDL2
```

The compiled binary must be run from the `tftdashdisplay/` directory, as all BMP sprite/graphic assets need to be in the same directory as the executable.

### Firmware

1. Open `tftdashfirmwareGEN4/tftdashfirmwareGEN4.ino` in the Arduino IDE.
2. Select the target board at the top of the file by uncommenting the appropriate define:
   - `#define GEN4BOARD` for Gen 4 (ATMega32u4 / Leonardo)
   - `#define MEGABOARD` for Gen 3 (ATMega2560 / Mega)
3. Upload to the board via the Arduino IDE.

Pre-compiled hex files are also available in the `tftdashfirmwareGEN4/` directory for direct flashing.

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

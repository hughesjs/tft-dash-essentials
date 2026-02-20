# Running TFT Dash with the Simulator

The display can now connect to the C# simulator for development and testing without hardware.

## Prerequisites

- .NET 8+ installed
- Simulator located at `../tft-dash-simulator/`

## Quick Start

### Terminal 1: Start the Simulator

```bash
cd ../tft-dash-simulator
dotnet run
```

The simulator will:
- Create a FIFO pipe at `/tmp/tft-dash-pipe`
- Wait for the display to connect
- Show a Terminal.Gui interface for controlling dashboard state
- Output messages at 10 Hz (100ms intervals)

### Terminal 2: Start the Display

```bash
cd display
zig build run
```

The display will:
- Detect the simulator pipe automatically
- Connect and start rendering
- Print "Simulator connected!" to stderr

## Connection Priority

The display checks for connections in this order:

1. **Simulator pipe** `/tmp/tft-dash-pipe` (highest priority)
2. macOS serial ports (`/dev/cu.usbserial-*`, `/dev/cu.usbmodem*`)
3. Linux serial ports (`/dev/ttyACM0`, `/dev/ttyACM1`)

This means the simulator is always used if available, making it safe for development without accidentally connecting to real hardware.

## Troubleshooting

### "Simulator connected!" but no data
- The simulator blocks until a reader connects
- Make sure both are running simultaneously
- Check that `/tmp/tft-dash-pipe` exists

### Display not connecting to simulator
- Ensure simulator started first (it creates the pipe)
- Check simulator console for "TFT Dash display connected."
- Try: `ls -l /tmp/tft-dash-pipe` to verify pipe exists

### Want to use real hardware instead
Stop the simulator - the display will automatically fall back to serial ports.

## Simulator Controls

See the simulator's Terminal.Gui interface for:
- Speed, RPM, temperature sliders
- Indicator toggles (neutral, oil, high beam, turn signals)
- Preset scenarios (idle, cruising, highway, etc.)
- Navigation message input

## Message Format

The simulator generates the exact same protocol as the firmware:

- **Live data**: `{,speed,rpm,coolant,battery,hour,minute,fuel,...,}`
- **Menu data**: `[,menustate,odo1,odo2,...,settings,...,]`

Messages are validated before sending to ensure protocol compliance.

## Development Workflow

1. **Modify parser** - Update `parser.c` and run `test_parser`
2. **Test with simulator** - Start simulator, run display, verify changes
3. **Real hardware test** - Stop simulator, test on actual bike

The simulator makes rapid iteration possible without hardware setup.

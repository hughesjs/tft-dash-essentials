# TFT Dash Serial Protocol

This document describes the serial communication protocols used by the TFT Dash system. There are three independent serial interfaces:

1. **Main interface** - Firmware to Display (USB serial)
2. **TPMS interface** - Tyre Pressure Monitoring System sensors to Display (separate USB serial)
3. **Navigation interface** - Phone to Firmware via BLE module on the Arduino (passed through to the Display embedded in the main interface output)

---

## 1. Main Interface (Firmware to Display)

**Connection:** USB serial over `/dev/ttyACM0` or `/dev/ttyACM1` (Linux) / `/dev/cu.usbmodem*` or `/dev/cu.usbserial-*` (macOS)

**Baud rate:** 115200

**Serial config:** 8N1, no flow control

**Direction:** Firmware sends continuously; display receives and parses.

The firmware outputs one of two message types on each loop iteration, depending on whether the user is in the main dashboard view or navigating the on-screen menu.

### 1.1 Live Data Message

Sent when the dashboard is in normal (non-menu) mode. Contains real-time sensor readings.

**Format:** `{,field1,field2,...,fieldN,}`

- Starts with `{`
- Fields are comma-delimited
- Ends with `}`
- Valid messages are between 90 and 400 characters in length

| Field index | Field | Type | Description |
|---|---|---|---|
| 0 | `{` | char | Start marker |
| 1 | speed | unsigned int | Current speed (in user's chosen unit: MPH or KM/H) |
| 2 | rpm | unsigned int | Engine RPM |
| 3 | coolant | int | Coolant temperature (averaged) |
| 4 | battery | float | Battery voltage (e.g. `12.5`) |
| 5 | hour | int | Current hour (0-23) from RTC |
| 6 | minute | int | Current minute (0-59) from RTC |
| 7 | fuel | int | Fuel float sensor value (raw integer) |
| 8 | neutral | int | Neutral light (1 = engaged, 0 = off) |
| 9 | oil | int | Oil level warning (1 = warning, 0 = ok) |
| 10 | highbeam | int | High beam indicator (1 = on, 0 = off) |
| 11 | left | int | Left indicator (1 = on, 0 = off) |
| 12 | right | int | Right indicator (1 = on, 0 = off) |
| 13 | menustate | int | Current menu state (`menulevel + choicestate`; 0 = main dashboard) |
| 14 | infomode | int | Info panel display mode |
| 15 | trip1 | float | Trip 1 distance (in user's chosen unit) |
| 16 | trip2 | float | Trip 2 distance (in user's chosen unit) |
| 17 | odo | float | Odometer reading (in user's chosen unit) |
| 18 | km | int | Unit mode (1 = kilometres, 0 = miles) |
| 19 | speedcorrection | float | Speed correction factor |
| 20 | theme | int | Current display theme ID |
| 21 | ambient | int | Ambient temperature (averaged) |
| 22 | gear | int | Current gear (calculated from sprocket ratios) |
| 23 | mpg | int | Miles per gallon |
| 24 | range | int | Estimated range remaining |
| 25 | maxspeed | unsigned int | Maximum speed recorded this trip (in user's chosen unit) |
| 26 | triptimehour | int | Trip time hours |
| 27 | triptimemin | int | Trip time minutes |
| 28 | oilpressavail | int | Oil pressure hardware available (1 = yes, 0 = no). Always 0 on Gen4 boards. |
| 29 | oilpress | int | Oil pressure sensor reading (averaged). Always 0 on Gen4 boards. |
| 30 | oiltemp | int | Oil temperature sensor reading (averaged). Always 0 on Gen4 boards. |
| 31 | fahrenheit | int | Temperature unit (1 = Fahrenheit, 0 = Celsius) |
| 32 | bar | int | Pressure unit (1 = Bar, 0 = PSI) |
| 33 | frontsensor | int | TPMS front sensor ID |
| 34 | rearsensor | int | TPMS rear sensor ID |
| 35 | frontpressurelow | int | Front tyre low pressure threshold |
| 36 | rearpressurelow | int | Rear tyre low pressure threshold |
| 37 | nav | string | Navigation message from phone via BLE (see section 3). Empty if no navigation active. |
| *end* | `}` | char | End marker |

**Example:**
```
{,78,5200,89,12.5,14,35,477,0,1,0,0,0,0,0,12.5,24.8,1234.5,0,1.0,3,22,4,45,120,78,1,35,0,0,0,0,0,1,2,30,28,,}
```

### 1.2 Menu Data Message

Sent when the user is navigating the on-screen menu system (i.e. `menulevel + choicestate != 0`). Contains current menu state and all configurable settings.

**Format:** `[,field1,field2,...,fieldN,]`

- Starts with `[`
- Fields are comma-delimited
- Ends with `]`
- Valid messages are between 80 and 100 characters in length

| Field index | Field | Type | Description |
|---|---|---|---|
| 0 | `[` | char | Start marker |
| 1 | menustate | int | Current menu state (`menulevel + choicestate`) |
| 2 | ododigit1 | int | Odometer setup digit 1 |
| 3 | ododigit2 | int | Odometer setup digit 2 |
| 4 | ododigit3 | int | Odometer setup digit 3 |
| 5 | ododigit4 | int | Odometer setup digit 4 |
| 6 | ododigit5 | int | Odometer setup digit 5 |
| 7 | ododigit6 | int | Odometer setup digit 6 |
| 8 | odo2digit1 | int | Odometer confirmation digit 1 |
| 9 | odo2digit2 | int | Odometer confirmation digit 2 |
| 10 | odo2digit3 | int | Odometer confirmation digit 3 |
| 11 | odo2digit4 | int | Odometer confirmation digit 4 |
| 12 | odo2digit5 | int | Odometer confirmation digit 5 |
| 13 | odo2digit6 | int | Odometer confirmation digit 6 |
| 14 | odoerror | int | Odometer error state |
| 15 | settimedigit0 | int | Set time digit 0 (tens of hours) |
| 16 | settimedigit1 | int | Set time digit 1 (units of hours) |
| 17 | settimedigit2 | int | Set time digit 2 (tens of minutes) |
| 18 | settimedigit3 | int | Set time digit 3 (units of minutes) |
| 19 | spcdigit0 | int | Speed correction sign/digit 0 (+ or -) |
| 20 | spcdigit1 | int | Speed correction digit 1 |
| 21 | spcdigit2 | int | Speed correction digit 2 |
| 22 | spcdigit3 | int | Speed correction digit 3 |
| 23 | frontsprocket | int | Front sprocket tooth count |
| 24 | rearsprocket | int | Rear sprocket tooth count |
| 25 | coolantfantemp | int | Coolant fan activation temperature |
| 26 | km | int | Using kilometres (1 = km, 0 = miles) |
| 27 | fh | int | Using Fahrenheit (1 = Fahrenheit, 0 = Celsius) |
| 28 | bar | int | Using Bar (1 = Bar, 0 = PSI) |
| 29 | frontsensor | int | TPMS front sensor ID |
| 30 | rearsensor | int | TPMS rear sensor ID |
| 31 | frontpressurelow | int | Front tyre low pressure threshold |
| 32 | rearpressurelow | int | Rear tyre low pressure threshold |
| 33 | controllayout | int | Control button layout |
| 34 | daytheme | int | Day theme ID |
| 35 | nighttheme | int | Night theme ID |
| 36 | currentlightlevel | int | Current ambient light sensor reading |
| 37 | lightswitchvalue | int | Light level threshold for day/night switching |
| 38 | fanneutraloption | int | Fan neutral mode option in coolant fan setup |
| 39 | gearratiointerval | int | Gear ratio update interval setting |
| *end* | `]` | char | End marker |

### 1.3 Menu State Constants

Menu states are encoded as `menulevel + choicestate`. The major menu category is a single digit, and sub-options are encoded as `category * 100 + offset`:

| Constant | Value | Description |
|---|---|---|
| `MAINDASHBOARD` | 0 | Normal dashboard view (live data sent) |
| `RESETTRIP1` | 1 | Reset Trip 1 |
| `RESETTRIP2` | 2 | Reset Trip 2 |
| `CONTROLSETUP` | 3 | Control layout setup |
| `CONTROLLAYOUT1` | 301 | Control layout option 1 |
| `CONTROLLAYOUT2` | 302 | Control layout option 2 |
| `SETUNITS` | 4 | Unit selection menu |
| `SETUNITSMILES` | 401 | Select miles |
| `SETUNITSKM` | 402 | Select kilometres |
| `SETUNITSCELCIUS` | 403 | Select Celsius |
| `SETUNITSFARENHEIT` | 404 | Select Fahrenheit |
| `SETUNITSPSI` | 405 | Select PSI |
| `SETUNITSBAR` | 406 | Select Bar |
| `SETTIME` | 5 | Set time menu |
| `SETTIMEDIGIT1-4` | 501-504 | Individual time digits |
| `AMBIENTLIGHTSETUP` | 6 | Ambient light setup |
| `DAYTHEMEPLUS/MINUS` | 601-602 | Day theme selection |
| `NIGHTTHEMEPLUS/MINUS` | 603-604 | Night theme selection |
| `LIGHTLEVELPLUS/MINUS` | 605-606 | Light level threshold |
| `SPEEDCORRECTIONMENU` | 7 | Speed correction |
| `SETDISPLAYTHEME` | 8 | Theme selection |
| `GEARINDICATORSETUP` | 9 | Gear indicator / sprocket setup |
| `COOLANTFANSETUP` | 10 | Coolant fan temperature setup |
| `TPMSSETUP` | 11 | TPMS sensor configuration |
| `ODOSETUP` | 12 | Odometer setup |

### 1.4 Display Theme IDs

| Value | Theme |
|---|---|
| 0 | Default (white) |
| 1 | Green |
| 2 | Red |
| 3 | Blue |
| 4 | Orange |
| 5 | Yellow |
| 6 | High contrast white |
| 7 | Night |
| 8 | High contrast dark |

### 1.5 Message Framing

The display reads the serial stream byte-by-byte on a background thread. Messages are framed as follows:

1. When a `{` or `[` character is received, the read buffer is reset and accumulation begins from that character.
2. Bytes are accumulated until the next start marker is received.
3. Before parsing, the message is validated:
   - Live data (`{`): length must be > 90 and < 400 characters
   - Menu data (`[`): length must be > 80 and < 100 characters
4. If validation passes, the message is copied to a global buffer and the appropriate unpack function is called.

On disconnection (0 or fewer bytes read), the display closes and reopens the serial port automatically.

---

## 2. TPMS Interface (Tyre Pressure Monitoring)

The TPMS system uses a **separate USB serial connection** between a wireless TPMS receiver and the Raspberry Pi. This is independent of the main firmware serial link - the TPMS receiver connects directly to the Pi, not via the Arduino. Two hardware variants are supported.

**Connection:** `/dev/ttyUSB0` or `/dev/ttyUSB1` (Linux) / `/dev/cu.usbserial-*` (macOS)

**Direction:** TPMS receiver sends binary data; display receives and parses on a dedicated thread.

### 2.1 Standard TPMS

**Baud rate:** 9600

**Serial config:** 8N1, no flow control

**Frame format:** Binary, 14 bytes per frame

| Byte | Value | Description |
|---|---|---|
| 0 | `0xAA` (170 / -86 signed) | Header byte 1 |
| 1 | `0xA1` (161 / -95 signed) | Header byte 2 |
| 2 | `0x41` (65) | Header byte 3 |
| 3 | 14 | Packet length |
| 4 | 99 | Mode/check byte |
| 5 | 1-4 | Sensor number (1-4) |
| 6-8 | — | Unknown / ID bytes |
| 9 | pressure high | Tyre pressure byte 1 (high bits) |
| 10 | pressure low | Tyre pressure byte 2 (low bits) |
| 11 | temperature | Temperature byte |
| 12 | state | Warning state byte |
| 13 | — | Unknown |

**Framing:** Messages start when byte `0xAA` is followed by `0xA1`. Valid frames are > 13 bytes.

**Pressure calculation:**
```
bar = 0.025 * (((byte9 & 3) * 256) + (byte10 & 255))
psi = bar * 14.5
```

**Temperature calculation:**
```
celsius = (byte11 & 255) - 50
```

**Warning state (byte 12):**

| Condition | Test |
|---|---|
| Fast leak | `(byte12 & 3) == 1` |
| High pressure | `(byte12 & 16) == 16` |
| Low pressure | `(byte12 & 8) == 8` |
| High temperature | `(byte12 & 4) == 4` |
| Low battery | `(byte12 & 128) == 128` |
| Normal | None of the above |

### 2.2 eBay TPMS (Budget Alternative)

**Baud rate:** 19200

**Serial config:** 8N1, no flow control

**Frame format:** Binary, 8 bytes per frame

| Byte | Value | Description |
|---|---|---|
| 0 | `0x55` (85) | Header byte 1 |
| 1 | `0xAA` (170 / -86 signed) | Header byte 2 |
| 2 | 8 | Packet length |
| 3 | sensor position | Sensor position code |
| 4 | pressure | Tyre pressure byte |
| 5 | temperature | Temperature byte |
| 6 | state | Warning state byte |
| 7 | — | Unknown |

**Framing:** Messages start when byte `0x55` (85) is followed by `0xAA` (-86 signed). Valid frames are > 5 bytes.

**Sensor position mapping:**

| Raw value | Mapped sensor | Position |
|---|---|---|
| 0 | 1 | Left front |
| 1 | 2 | Right front |
| 16 | 3 | Back left |
| 17 | 4 | Back right |

**Pressure calculation:**
```
psi = (byte4 & 255) * 3.44
bar = psi / 14.5
```

**Temperature calculation:**
```
celsius = (byte5 & 255) - 50
```

**Warning state (byte 6):**

| Condition | Test |
|---|---|
| No signal | `(byte6 & 32) != 0` |
| Air leak | `(byte6 & 8) != 0` |

### 2.3 Sensor Assignment

The TPMS front and rear sensor IDs are configurable through the on-screen menu system. The display matches incoming sensor numbers against `frontsensorid` and `rearsensorid` to assign readings to the front and rear tyre displays.

---

## 3. Navigation Messages (Phone via BLE through Arduino)

Navigation data originates from a phone app and is received by a Bluetooth Low Energy module connected to the **Arduino board** (not the Pi) via SoftwareSerial at 9600 baud.

The Arduino buffers incoming BLE bytes in its main loop. Once a complete navigation message is received (terminated by `%>`), the firmware stores it and includes it as field 37 of the next live data message sent over USB to the Raspberry Pi. The Pi never communicates with the BLE module directly.

### 3.1 BLE to Firmware Framing

The phone sends navigation messages terminated by the two-character sequence `%>`. The firmware accumulates bytes until it sees this terminator, at which point the complete message is stored in the `szNav` buffer. If no navigation message has been received, field 37 of the live data output is empty.

### 3.2 Navigation Message Format

Within the live data message, the navigation string uses `%` as its own field delimiter (distinct from the comma delimiter used in the surrounding protocol).

**Format:** `%symbol%road%towards%exit%yards%miles%drivingleft%destdistance%`

| Field index | Field | Type | Description |
|---|---|---|---|
| 1 | symbol | string | Navigation manoeuvre symbol code (see table below) |
| 2 | road | string | Road name to turn onto |
| 3 | towards | string | Place name being headed towards |
| 4 | exit | string | Motorway exit number (if applicable) |
| 5 | yards | int | Distance to next manoeuvre in yards |
| 6 | miles | float | Distance to next manoeuvre in miles |
| 7 | drivingleft | int | Driving side (1 = left-hand traffic / UK, 0 = right-hand traffic) |
| 8 | destdistance | float | Total distance to destination in miles |

### 3.3 Navigation Symbol Codes

| Code | Description |
|---|---|
| `MUT` | Make a U-turn (icon flipped based on driving side) |
| `MEX` | Motorway exit (icon flipped based on driving side) |
| `TNR` | Turn right |
| `TNL` | Turn left |
| `SLL` | Slight left |
| `SLR` | Slight right |
| `RB1` | Roundabout, 1st exit (icon flipped based on driving side) |
| `RB2` | Roundabout, 2nd exit (icon flipped based on driving side) |
| `RB3` | Roundabout, 3rd exit (icon flipped based on driving side) |
| `RB4` | Roundabout, 4th exit (icon flipped based on driving side) |
| `RB5` | Roundabout, 5th exit (icon flipped based on driving side) |
| `LNR` | Keep right / lane right |
| `LNL` | Keep left / lane left |
| `CON` | Continue straight |
| `ARV` | Arrived at destination |
| `PRK` | Parking |
| `FRY` | Ferry |

**Note:** Symbols marked "flipped based on driving side" render a mirrored icon depending on the `drivingleft` field, to correctly show the manoeuvre for left-hand or right-hand traffic countries.

### 3.4 Unit Conversion on Display

The display automatically converts navigation distances based on the user's unit preference:
- **Yards to metres:** `metres = yards / 1.094`
- **Miles to kilometres:** `km = miles * 1.609`

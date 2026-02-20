# OTA Update System — WiFi AP Mode

Design for wireless update delivery without USB sticks.

## Architecture

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

## Why This Approach

### BLE alone is too slow
The update package is ~10MB zipped, 330MB uncompressed. BLE tops out at 10-30 KB/s. Even the compressed package would take 5-15 minutes. The full BMP set uncompressed would take hours. The ATMega32u4 only has 2.5KB of RAM — it can't buffer or relay large transfers.

### Pi as AP avoids phone hotspot dependency
iOS has no public API to enable Personal Hotspot programmatically (`NEHotspotConfiguration` only joins existing networks, it can't create one). A Pi-hosted AP works identically on iOS and Android with no manual steps.

### Fixed known IP
The Pi is always 192.168.4.1 as the AP gateway. No mDNS discovery, no DHCP lease parsing, no platform-specific gateway addresses.

### BLE only carries control messages
"Start update mode" and "disconnect" — tiny messages that the existing BLE relay through the Arduino handles already. No new BLE protocol needed.

## Implementation Plan

### 1. Pi-Side Update Daemon

A small service/script that:

1. **Listens for update trigger** — either:
   - BLE command relayed via serial (watches for `<UPD:START%>` in the serial stream)
   - Physical button combo (hold option + select for 5 seconds) for garage use without a phone
2. **Starts WiFi AP** — `hostapd` + `dnsmasq` to create "TFTDash" network on 192.168.4.1
3. **Runs HTTP server** on port 8080 accepting POST to `/update`
4. **Receives and applies update** — validate zip, extract to staging area, copy binary + assets into place, optionally flash firmware hex via avrdude
5. **Tears down AP** and restarts the dashboard

Could be ~50 lines of bash wrapping a small HTTP receiver (e.g. Python one-liner or a tiny C program).

### 2. Firmware Changes

- New BLE message type: `<UPD:START%>` to trigger update mode
- Arduino relays to Pi via existing serial passthrough — the firmware already passes all BLE messages through, so this may need no firmware changes at all beyond the phone app sending the right string

### 3. Phone App

- "Update Dash" button that bundles the current update zip
- Sends BLE `START_UPDATE` command to the Arduino
- Uses `NEHotspotConfiguration` (iOS) / standard WiFi API (Android) to join "TFTDash" network — iOS shows one system confirmation dialog
- POSTs zip to `http://192.168.4.1:8080/update`
- Shows progress bar during upload
- Handles success/failure response from Pi
- On completion, phone auto-rejoins its previous network

### 4. Pi Requirements

- `hostapd` and `dnsmasq` packages installed on the Pi SD card image
- WiFi hardware (Pi 3 has built-in, but it is **currently disabled** via `dtoverlay=pi3-disable-wifi` in `/boot/config.txt` — this needs removed)
- Bluetooth is also disabled (`dtoverlay=pi3-disable-bt`) — can stay disabled since BLE goes through the Arduino, not the Pi
- Pre-configured `hostapd.conf` for the "TFTDash" AP
- The update daemon installed as a systemd service or integrated into `updatetft.sh`
- Root filesystem is read-only — daemon must remount rw before applying updates (existing script already does this)

## Existing Update Package (USB Stick)

The current mechanism uses a USB stick with a `tftupdate/` directory. A boot script on the Pi's SD card detects it and copies files across.

### Package Structure
```
tftupdate/
├── bootimage.png              Fazer splash screen (1024x600)
├── firmware/
│   ├── firmware.hex           Mega board hex
│   ├── firmware4.hex          Gen4 board hex
│   └── avrdude.conf           avrdude config for flashing Arduino
└── new/
    ├── testsdl                ARM 32-bit (armhf) binary — the display app
    ├── testsdl.cpp            Source code (for reference/backup)
    └── *.bmp (×855)           All theme BMPs in flat layout with prefixes
```

### BMP Naming Convention (Old Flat Layout)
- Default theme: `Coolant.bmp`, `Speednumbers.bmp`, etc. (no prefix)
- Themed: `blue-Coolant.bmp`, `green-Speednumbers.bmp`, etc.
- Theme prefixes: blue, bright, dark, green, night, orange, red, yellow

### BMP Naming Convention (New Refactored Layout)
The refactored display code expects theme directories:
```
assets/themes/<theme>/Name.bmp
```
e.g. `assets/themes/blue/Coolant.bmp`, `assets/themes/default/Coolant.bmp`

The packaging script needs to produce whichever format the Pi's image expects. The current Pi image uses the old flat layout. If the Pi image is updated with the refactored display code, the new directory layout should be used instead.

### Boot Script
The exact boot/update script is on the Pi's SD card image — not yet in this repo. Needs to be retrieved by pulling the SD card.

## TODO

- [ ] Pull Pi SD card and retrieve the existing update/boot script into this repo
- [ ] Write `make-update-package.sh` to produce the correct package format
- [ ] Implement Pi-side update daemon (hostapd AP + HTTP receiver)
- [ ] Add `<UPD:START%>` BLE message handling (may be zero firmware work)
- [ ] Phone app: BLE trigger + WiFi join + HTTP upload
- [ ] Calibrate `RPM_CONSTANT` for FZS600 (warm idle should read 1150–1250 RPM)
- [ ] Recalibrate fuel level lookup table for FZS600 tank (19.4L vs 21L, different shape)

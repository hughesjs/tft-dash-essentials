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

## Current Update Mechanism (USB A/B)

The A/B rootfs update system handles USB-based updates — see `docs/ab-update-plan.md`. OTA via WiFi AP would be an additional delivery path using the same rootfs image format.

## TODO

- [ ] Implement Pi-side update daemon (hostapd AP + HTTP receiver)
- [ ] Add `<UPD:START%>` BLE message handling (may be zero firmware work)
- [ ] Phone app: BLE trigger + WiFi join + HTTP upload

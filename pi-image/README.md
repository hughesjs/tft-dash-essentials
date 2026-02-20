# Pi Image Configuration

Files from the Raspberry Pi 3 SD card. The Pi runs a read-only RetroPie-based image with the TFT Dash display application.

## Files

| File | Installed to | Purpose |
|---|---|---|
| `updatetft.sh` | `/home/pi/updatetft.sh` | Boot script — starts dashboard, checks for USB updates |
| `rc.local` | `/etc/rc.local` | Launches `updatetft.sh` at boot |
| `config.txt` | `/boot/config.txt` | Pi boot config — DPI display at 1024x600, BT/WiFi disabled |
| `cmdline.txt` | `/boot/cmdline.txt` | Kernel command line — read-only root, no swap, quiet boot |
| `fstab` | `/etc/fstab` | Filesystem mounts — root and boot both mounted read-only |
| `splashscreen.list` | `/etc/splashscreen.list` | Path to the boot splash image |

## Key Details

- **Display**: 1024x600 DPI (directly connected, not HDMI)
- **Root filesystem**: Read-only (`ro` in fstab and cmdline). Remounted rw only during updates.
- **WiFi/Bluetooth**: Disabled in config.txt (`pi3-disable-bt`, `pi3-disable-wifi`)
- **App directory**: `/home/pi/tftdash/TFTDash/` — flat layout with binary + BMPs
- **Splash screen**: `/opt/retropie/supplementary/splashscreen/retropie-default.png`
- **Firmware flashing**: Via `avrdude` over `/dev/ttyACM*`

## Update Mechanism

The update script (`updatetft.sh`) runs at every boot via `rc.local`. It:

1. Starts the dashboard immediately
2. Waits 30 seconds for USB devices to mount
3. Scans for `tftupdate.zip` on any USB stick
4. If found: kills the dashboard, remounts rw, extracts and applies the update
5. The update script can update itself if `tftupdate/updatetft.sh` exists in the zip

## Updating These Files

Any of these files can be included in the update zip to be deployed automatically:

- `tftupdate/updatetft.sh` → updates the boot script
- `tftupdate/rc.local` → updates rc.local
- `tftupdate/config.txt` → updates boot config

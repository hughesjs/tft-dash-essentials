# A/B Rootfs Update Plan

**Status: Implemented** — see `buildroot-tftdash/` for the actual code.

## Overview

The TFT Dash Buildroot image uses an A/B rootfs partition scheme for atomic updates. Updates are delivered via USB stick — the entire Buildroot rootfs image is `dd`'d onto the standby partition, the boot target is flipped in `cmdline.txt`, and the device reboots.

This preserves the core benefit of Buildroot: the image produced by `build.sh` is the single source of truth for what runs on the dash. No patching individual files, no drift.

## SD Card Partition Layout

```
mmcblk0p1  boot      32MB    FAT32   (kernel, DTBs, config.txt, cmdline.txt)
mmcblk0p2  rootfs_a  512MB   ext4    (active — complete Buildroot rootfs, read-only)
mmcblk0p3  rootfs_b  512MB   ext4    (standby — dd target for updates)
mmcblk0p4  data      16MB    ext4    (persistent rw partition — update log, future use)
```

- **boot**: Kernel, device tree, Pi firmware blobs, and boot config. Rarely changes.
- **rootfs_a / rootfs_b**: Identical in size. One is active (mounted as `/`), the other is the update target. Both are full Buildroot images containing the app binary, BMP assets, libraries, everything.
- **data**: Small persistent partition mounted at `/data`. Currently used for update logging. User settings (odometer, trip, theme, etc.) are stored on the Arduino's I2C EEPROM, not on the Pi.

Total: ~1.1GB. Fits easily on any SD card (typical is 8-32GB). Both rootfs slots get the same image on initial flash, so either is bootable from the start.

## Design Decisions

- **cmdline.txt swap** (no U-Boot) — simple, appropriate for a physically-accessible motorbike dash. The update script rewrites `root=` in `cmdline.txt` to point at the standby partition.
- **No udev** — the system uses devtmpfs. USB detection uses a boot-time scan (`S10usbupdate` init script) instead of udev rules.
- **No root entry in fstab** — the kernel mounts root via the `root=` cmdline parameter. This avoids hardcoding which slot is active.
- **Boot partition stays 32MB** — current contents total ~27MB.
- **Rootfs partitions: 512MB each** — matches the existing Buildroot rootfs size.

## Update Flow

1. User builds a new image (`make image`)
2. User creates an update package (`make update-package`)
3. User copies `tftupdate/` directory to a FAT32 USB stick
4. User plugs USB into the Pi and reboots (or powers on)
5. `S10usbupdate` init script (runs at boot before the dashboard) scans USB devices
6. If `tftupdate/rootfs.img` is found, it hands off to `tft-update.sh`
7. `tft-update.sh`: verifies SHA-256 checksum, `dd`s image to standby partition, flips `cmdline.txt`, optionally flashes Arduino firmware, reboots
8. Device boots into the new rootfs slot

## Key Files

| File | Purpose |
|------|---------|
| `buildroot-tftdash/board/genimage.cfg` | 4-partition SD card layout |
| `buildroot-tftdash/overlay/etc/fstab` | A/B-aware mounts (no root entry, /data on p4) |
| `buildroot-tftdash/overlay/usr/bin/tft-update.sh` | Core update script |
| `buildroot-tftdash/overlay/etc/init.d/S10usbupdate` | Boot-time USB scan |
| `buildroot-tftdash/board/post-image.sh` | Generates cmdline.txt (defaults to slot A) |

## USB Update Package Format

Built via `make update-package` (or `buildroot-tftdash/build.sh update-package`):

```
tftupdate/
├── rootfs.img            Full ext4 rootfs image (dd'd to standby partition)
├── rootfs.img.sha256     SHA-256 checksum
└── firmware.hex          Optional Arduino firmware (flashed via avrdude)
```

## QEMU Testing

```bash
make qemu      # Boot from slot A (root=/dev/vda2)
make qemu-b    # Boot from slot B (root=/dev/vda3)
```

The update script is QEMU-aware: it detects `vda*` root devices and skips `cmdline.txt` modification (QEMU uses `-append` for the kernel cmdline). The `dd` to the standby partition still works, so the update logic can be exercised.

## What Cannot Be Tested in QEMU

- cmdline.txt swap (QEMU ignores on-disk cmdline.txt)
- USB stick detection (no USB passthrough)
- Actual reboot into other slot
- Display rendering

These require real Pi hardware.

## Open Questions

- **Visual feedback**: The dash should ideally show "Updating..." on screen during the `dd`. Could be a simple framebuffer splash image written before the update starts.
- **Boot partition updates**: If the kernel or DTB needs updating, the update script would also need to write to the boot partition. For now, assume the boot partition is stable and only the rootfs changes.
- **Rollback**: Currently no automatic rollback — if the new image is broken, the old slot is still intact but requires manual intervention (swap cmdline.txt back, or reflash). U-Boot with bootcount could be added later if needed.

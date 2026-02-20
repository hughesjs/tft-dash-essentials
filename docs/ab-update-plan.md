# A/B Rootfs Update Plan

## Overview

Implement a self-update mechanism for the Buildroot-based TFT Dash system using an A/B rootfs partition scheme. Updates are delivered via USB stick and applied atomically — the entire Buildroot image is `dd`'d onto the standby partition, the boot target is flipped, and the device reboots.

This preserves the core benefit of Buildroot: the image produced by `build.sh` is the single source of truth for what runs on the dash. No patching individual files, no drift.

## SD Card Partition Layout

```
mmcblk0p1  boot      ~50MB   FAT32   (kernel, DTBs, config.txt, cmdline.txt)
mmcblk0p2  rootfs_a  ~256MB  ext4    (active — complete Buildroot rootfs, read-only)
mmcblk0p3  rootfs_b  ~256MB  ext4    (standby — dd target for updates)
mmcblk0p4  data      ~16MB   ext4    (persistent user data — odometer, settings, trip)
```

- **boot**: Kernel, device tree, Pi firmware blobs, and boot config. Rarely changes.
- **rootfs_a / rootfs_b**: Identical in size. One is active (mounted as `/`), the other is the update target. Both are Buildroot images containing the full system — app binary, BMP assets, libraries, everything.
- **data**: Small persistent partition mounted at e.g. `/data`. Holds odometer, trip counters, user settings. Format is stable and survives updates. This is the only writable partition during normal operation.

## Boot Configuration

The Pi doesn't have U-Boot by default, so we use one of:

### Option A: cmdline.txt swap (simplest)

- `config.txt` on the boot partition can select between two `cmdline.txt` files
- The update script rewrites `cmdline.txt` to point `root=` at the other partition
- No rollback protection — if the new image is broken, manual intervention required

### Option B: U-Boot with bootcount (robust)

- Replace the Pi's direct kernel boot with U-Boot
- U-Boot maintains a `bootcount` variable and an `upgrade_available` flag
- On successful boot, the init system resets the bootcount
- If bootcount exceeds a threshold (e.g. 3), U-Boot falls back to the previous partition
- Buildroot has U-Boot support built in (`BR2_TARGET_UBOOT`)

### Recommendation

Start with **Option A** for simplicity. Move to U-Boot later if rollback protection is needed. On a motorbike dash, a failed update just means re-flashing the SD card — it's not a remote device you can't physically access.

## Update Trigger: USB Insert via udev

A udev rule detects USB stick insertion and triggers the update script.

### udev Rule

```
# /etc/udev/rules.d/99-tftupdate.rules
ACTION=="add", KERNEL=="sd[a-z]1", SUBSYSTEM=="block", RUN+="/usr/bin/tft-update.sh %k"
```

Place this in the Buildroot rootfs overlay at `buildroot/overlay/etc/udev/rules.d/`.

### Update Script (`tft-update.sh`)

```bash
#!/bin/sh

DEVICE="/dev/$1"
MOUNT_POINT="/tmp/usb"
UPDATE_FILE="tftupdate/rootfs.img"
CHECKSUM_FILE="tftupdate/rootfs.img.sha256"
CURRENT_ROOT=$(findmnt -n -o SOURCE /)

# Determine standby partition
if [ "$CURRENT_ROOT" = "/dev/mmcblk0p2" ]; then
    TARGET="/dev/mmcblk0p3"
    NEW_PARTNUM=3
elif [ "$CURRENT_ROOT" = "/dev/mmcblk0p3" ]; then
    TARGET="/dev/mmcblk0p2"
    NEW_PARTNUM=2
else
    echo "Unknown root partition: $CURRENT_ROOT" | logger -t tftupdate
    exit 1
fi

# Mount USB
mkdir -p "$MOUNT_POINT"
mount -o ro "$DEVICE" "$MOUNT_POINT" || exit 1

# Check for update files
if [ ! -f "$MOUNT_POINT/$UPDATE_FILE" ] || [ ! -f "$MOUNT_POINT/$CHECKSUM_FILE" ]; then
    umount "$MOUNT_POINT"
    exit 0  # No update on this USB — not an error
fi

# Verify checksum
cd "$MOUNT_POINT/tftupdate"
if ! sha256sum -c rootfs.img.sha256; then
    echo "Checksum verification failed" | logger -t tftupdate
    umount "$MOUNT_POINT"
    exit 1
fi
cd /

# Flash standby partition
dd if="$MOUNT_POINT/$UPDATE_FILE" of="$TARGET" bs=4M conv=fsync status=progress
sync

# Update cmdline.txt to boot from new partition
mount -o remount,rw /boot
sed -i "s|root=/dev/mmcblk0p[23]|root=/dev/mmcblk0p${NEW_PARTNUM}|" /boot/cmdline.txt
mount -o remount,ro /boot

# Clean up and reboot
umount "$MOUNT_POINT"
echo "Update applied to $TARGET, rebooting..." | logger -t tftupdate
reboot
```

## Build Pipeline Changes

### Buildroot Configuration

- Modify the Buildroot defconfig / `genimage.cfg` to produce the new 4-partition SD card layout
- The rootfs image (`rootfs.ext4`) is what gets placed on the USB stick for updates
- Adjust `build.sh` to also output the standalone rootfs image alongside the full `sdcard.img`

### Update Package Creation

Add a script to produce the USB update package:

```bash
#!/bin/sh
# make-update-package.sh
# Run after build.sh — packages the rootfs image for USB delivery

BUILDROOT_OUTPUT="../buildroot/output/images"
ROOTFS_IMG="$BUILDROOT_OUTPUT/rootfs.ext4"
OUTPUT_DIR="tftupdate"

mkdir -p "$OUTPUT_DIR"
cp "$ROOTFS_IMG" "$OUTPUT_DIR/rootfs.img"
cd "$OUTPUT_DIR"
sha256sum rootfs.img > rootfs.img.sha256
cd ..

echo "Update package ready in $OUTPUT_DIR/"
echo "Copy this directory to the root of a FAT32 USB stick."
```

### Persistent Data Partition

- The app needs to be modified to read/write settings, odometer, and trip data from `/data` instead of relying on EEPROM or files in the app directory
- First boot should initialise `/data` with defaults if empty
- The data partition format should be documented and kept stable across updates

## Implementation Steps

1. **Modify `genimage.cfg`** to produce the 4-partition layout (boot, rootfs_a, rootfs_b, data)
2. **Add the udev rule** to the Buildroot overlay
3. **Write `tft-update.sh`** and add it to the overlay at `overlay/usr/bin/`
4. **Write `make-update-package.sh`** in the repo root
5. **Update the app** to use `/data` for persistent storage (odo, settings, trip)
6. **Update `build.sh`** to also copy out the standalone rootfs image
7. **Test the full cycle**: build image, flash SD, build a second image, put on USB, insert USB, verify update applies and device boots the new image
8. **Update `cmdline.txt`** in the boot partition to default to `root=/dev/mmcblk0p2`

## Open Questions

- **Firmware flashing**: Should the update also flash Arduino firmware via avrdude? If so, include the hex file on the USB alongside the rootfs image and have the script flash it before rebooting. This is separate from the rootfs — it's just a serial flash command.
- **Boot partition updates**: If the kernel or DTB needs updating, the update script would also need to write to the boot partition. For now, assume the boot partition is stable and only the rootfs changes.
- **Visual feedback**: The dash should ideally show "Updating..." on screen during the `dd`. This could be a simple framebuffer splash image written before the update starts.
- **Data partition migration**: If a future update changes the format of persistent data, we'll need a migration mechanism. Keep it simple — version the data format and have the app handle upgrades on first boot.

#!/bin/sh
#
# tft-update.sh — A/B rootfs update for TFT Dash
#
# Writes a rootfs image to the standby partition, flips cmdline.txt
# to boot from it, and reboots.
#
# Usage:
#   tft-update.sh /dev/sda1          # USB partition containing tftupdate/
#   tft-update.sh /path/to/rootfs.img # Direct image file (for testing)
#
set -e

LOG="/data/update.log"
MOUNT_DIR="/tmp/tft-update-usb"

log() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') $*" | tee -a "$LOG"
}

die() {
    log "ERROR: $*"
    cleanup
    exit 1
}

cleanup() {
    umount "$MOUNT_DIR" 2>/dev/null || true
    rmdir "$MOUNT_DIR" 2>/dev/null || true
}

# --- Determine current and standby root partitions ---

CURRENT_ROOT=""
for arg in $(cat /proc/cmdline); do
    case "$arg" in
        root=*) CURRENT_ROOT="${arg#root=}" ;;
    esac
done

[ -n "$CURRENT_ROOT" ] || die "Could not determine current root from /proc/cmdline"

case "$CURRENT_ROOT" in
    /dev/mmcblk0p2) STANDBY="/dev/mmcblk0p3" ;;
    /dev/mmcblk0p3) STANDBY="/dev/mmcblk0p2" ;;
    /dev/vda2)      STANDBY="/dev/vda3" ;;
    /dev/vda3)      STANDBY="/dev/vda2" ;;
    *) die "Unexpected root device: $CURRENT_ROOT" ;;
esac

log "Current root: $CURRENT_ROOT — Standby: $STANDBY"

# --- Locate the update image ---

INPUT="$1"
[ -n "$INPUT" ] || die "Usage: tft-update.sh <device|image-file>"

IMAGE_FILE=""
FIRMWARE_HEX=""

if [ -b "$INPUT" ]; then
    # Block device — mount and look for update files
    mkdir -p "$MOUNT_DIR"
    mount -o ro "$INPUT" "$MOUNT_DIR" || die "Failed to mount $INPUT"

    if [ ! -f "$MOUNT_DIR/tftupdate/rootfs.img" ]; then
        cleanup
        die "No tftupdate/rootfs.img found on $INPUT"
    fi

    IMAGE_FILE="$MOUNT_DIR/tftupdate/rootfs.img"

    # Verify checksum if present
    if [ -f "$MOUNT_DIR/tftupdate/rootfs.img.sha256" ]; then
        log "Verifying SHA-256 checksum..."
        EXPECTED=$(cut -d' ' -f1 "$MOUNT_DIR/tftupdate/rootfs.img.sha256")
        ACTUAL=$(sha256sum "$IMAGE_FILE" | cut -d' ' -f1)
        [ "$EXPECTED" = "$ACTUAL" ] || die "Checksum mismatch: expected $EXPECTED, got $ACTUAL"
        log "Checksum OK"
    else
        log "Warning: no checksum file found, skipping verification"
    fi

    # Check for firmware update
    if [ -f "$MOUNT_DIR/tftupdate/firmware.hex" ]; then
        FIRMWARE_HEX="$MOUNT_DIR/tftupdate/firmware.hex"
    fi
elif [ -f "$INPUT" ]; then
    # Direct file path (for testing)
    IMAGE_FILE="$INPUT"
else
    die "$INPUT is neither a block device nor a file"
fi

# --- Stop the dashboard ---

log "Stopping dashboard..."
killall testsdl 2>/dev/null || true
sleep 1

# --- Write image to standby partition ---

log "Writing $IMAGE_FILE to $STANDBY ..."
dd if="$IMAGE_FILE" of="$STANDBY" bs=4M conv=fsync 2>>"$LOG"
sync
log "Write complete"

# --- Flash firmware if provided ---

if [ -n "$FIRMWARE_HEX" ]; then
    log "Flashing firmware from $FIRMWARE_HEX ..."
    if command -v avrdude >/dev/null 2>&1; then
        avrdude -p atmega32u4 -c avr109 -P /dev/ttyACM0 -U flash:w:"$FIRMWARE_HEX":i 2>>"$LOG" \
            && log "Firmware flash complete" \
            || log "Warning: firmware flash failed (non-fatal)"
    else
        log "Warning: avrdude not found, skipping firmware flash"
    fi
fi

# --- Flip cmdline.txt to boot from standby ---

# QEMU uses -append for kernel cmdline, so cmdline.txt changes have no effect.
# Skip the flip but still log what would happen.
case "$CURRENT_ROOT" in
    /dev/vda*)
        log "QEMU detected — skipping cmdline.txt modification"
        log "Would flip root= from $CURRENT_ROOT to $STANDBY"
        ;;
    *)
        log "Updating cmdline.txt to boot from $STANDBY"
        mount -o remount,rw /boot
        sed -i "s|root=$CURRENT_ROOT|root=$STANDBY|" /boot/cmdline.txt
        sync
        mount -o remount,ro /boot
        log "cmdline.txt updated"
        ;;
esac

# --- Clean up and reboot ---

cleanup
log "Update complete — rebooting into $STANDBY"
sync
reboot

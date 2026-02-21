#!/bin/bash
# Post-image script — generates the SD card image
set -e

BOARD_DIR="$(dirname "$0")"
GENIMAGE_CFG="${BOARD_DIR}/genimage.cfg"
GENIMAGE_TMP="${BUILD_DIR}/genimage.tmp"

# Copy config.txt and cmdline.txt to the firmware output dir
cp "${BR2_EXTERNAL_TFTDASH_PATH}/overlay/boot/config.txt" \
	"${BINARIES_DIR}/rpi-firmware/config.txt"

# A/B rootfs scheme: root= points to the active slot.
#   Slot A = /dev/mmcblk0p2 (default on fresh flash)
#   Slot B = /dev/mmcblk0p3
# tft-update.sh flips root= between p2 and p3 after writing an update
# to the standby partition.
cat > "${BINARIES_DIR}/rpi-firmware/cmdline.txt" <<'EOF'
dwc_otg.lpm_enable=0 console=tty1 root=/dev/mmcblk0p2 rootfstype=ext4 elevator=deadline fsck.repair=yes rootwait loglevel=3 consoleblank=0 logo.nologo vt.global_cursor_default=0 quiet fastboot noswap ro
EOF

rm -rf "${GENIMAGE_TMP}"

genimage \
	--rootpath "${TARGET_DIR}" \
	--tmppath "${GENIMAGE_TMP}" \
	--inputpath "${BINARIES_DIR}" \
	--outputpath "${BINARIES_DIR}" \
	--config "${GENIMAGE_CFG}"

exit 0

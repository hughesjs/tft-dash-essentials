#!/bin/bash
#
# updatetft.sh — TFT Dash boot script
#
# Called from /etc/rc.local at boot. Starts the dashboard, then checks for
# an update zip on any mounted USB stick. If found, applies the update.
#
# The update zip (tftupdate.zip) should be at the root of the USB stick.
# See make-update-package.sh for how to build one.
#

TFTDASH_DIR="/home/pi/tftdash/TFTDash"
STAGING_DIR="/home/pi/tftdashupdate"
SPLASH_PATH="/opt/retropie/supplementary/splashscreen/retropie-default.png"

# --- Start the dashboard immediately ---
cd "$TFTDASH_DIR"
./testsdl &

# Wait for USB devices to mount
sleep 30

# --- Scan for update zip on USB sticks ---
UPDATEFILE=""
UPDATEFOLDER=""

for i in $(readlink -f /dev/disk/by-id/usb* 2>/dev/null); do
	USBFOLDER=$(mount | grep "$i\b" | awk '{print $3}')
	if [ ! -z "${USBFOLDER}" ]; then
		if [ -f "$USBFOLDER/tftupdate.zip" ]; then
			UPDATEFILE="$USBFOLDER/tftupdate.zip"
			UPDATEFOLDER="$USBFOLDER"
		fi
	fi
done

if [ -z "${UPDATEFILE}" ]; then
	echo "No update USB found. Running TFT Dash normally."
	exit 0
fi

# --- Update found ---
clear
echo ""
echo "**********************************************************"
echo "            Found TFT Dash Update via USB..."
echo "**********************************************************"
echo ""
echo "Stopping dashboard and applying update..."
echo ""

pkill testsdl
sleep 5

# Remount root read-write for the update
sudo mount -o remount,rw /
sudo mount -o remount,rw /boot 2>/dev/null

# --- Extract update ---
echo "Extracting update..."

if [ -d "$STAGING_DIR" ]; then
	sudo rm -rf "$STAGING_DIR"
fi

sudo mkdir -p "$STAGING_DIR"
sudo cp "$UPDATEFILE" "$STAGING_DIR/"
sudo unzip -o "$STAGING_DIR/tftupdate.zip" -d "$STAGING_DIR"

TFTUPDATE="$STAGING_DIR/tftupdate"

# --- Self-update: update this script if a new version is in the zip ---
if [ -f "$TFTUPDATE/updatetft.sh" ]; then
	echo "Updating update script..."
	sudo cp -f "$TFTUPDATE/updatetft.sh" /home/pi/updatetft.sh
	sudo chmod +x /home/pi/updatetft.sh
fi

# --- Update rc.local if provided ---
if [ -f "$TFTUPDATE/rc.local" ]; then
	echo "Updating rc.local..."
	sudo cp -f "$TFTUPDATE/rc.local" /etc/rc.local
	sudo chmod +x /etc/rc.local
fi

# --- Update boot config if provided ---
if [ -f "$TFTUPDATE/config.txt" ]; then
	echo "Updating boot config..."
	sudo cp -f "$TFTUPDATE/config.txt" /boot/config.txt
fi

# --- Display software ---
echo ""
echo "Updating display software..."

if [ -f "$TFTUPDATE/new/testsdl" ]; then
	echo "  Copying new binary..."
	sudo cp -f "$TFTUPDATE/new/testsdl" "$TFTDASH_DIR/testsdl"
	sudo chmod +x "$TFTDASH_DIR/testsdl"
fi

# Copy BMP assets (flat layout: all files except binary and source)
BMP_COUNT=0
for f in "$TFTUPDATE/new/"*.bmp; do
	[ -f "$f" ] || continue
	sudo cp -f "$f" "$TFTDASH_DIR/"
	BMP_COUNT=$((BMP_COUNT + 1))
done
if [ $BMP_COUNT -gt 0 ]; then
	echo "  Copied $BMP_COUNT BMP files"
fi

# Copy theme directory layout if present (new asset structure)
if [ -d "$TFTUPDATE/new/assets" ]; then
	echo "  Copying theme asset directories..."
	sudo cp -rf "$TFTUPDATE/new/assets" "$TFTDASH_DIR/"
fi

# On-Pi compilation from source (overrides pre-built binary)
if [ -f "$TFTUPDATE/new/testsdl.cpp" ]; then
	echo "  Source file found — compiling on Pi..."
	if sudo g++ -o "$STAGING_DIR/testsdl_compiled" "$TFTUPDATE/new/testsdl.cpp" -lSDL2 -lpthread; then
		sudo cp -f "$STAGING_DIR/testsdl_compiled" "$TFTDASH_DIR/testsdl"
		sudo chmod +x "$TFTDASH_DIR/testsdl"
		echo "  Compiled binary installed (overrides pre-built)"
	else
		echo "  WARNING: Compilation failed — keeping pre-built binary"
	fi
fi

# --- Boot image ---
if [ -f "$TFTUPDATE/bootimage.png" ]; then
	echo "Updating boot image..."
	sudo cp -f "$TFTUPDATE/bootimage.png" "$SPLASH_PATH"
fi

# --- Firmware ---
echo ""
echo "*********************************************************************************"
echo "Updating firmware — DO NOT TURN OFF YOUR IGNITION DURING THIS PROCESS"
echo "*********************************************************************************"
echo ""

ARDUINOPORT=""
for p in /dev/ttyACM*; do
	[ -e "$p" ] && ARDUINOPORT="$p"
done

if [ ! -z "${ARDUINOPORT}" ] && [ -f "$TFTUPDATE/firmware/avrdude.conf" ]; then
	echo "Firmware port: $ARDUINOPORT"

	# Flash Mega board firmware
	if [ -f "$TFTUPDATE/firmware/firmware.hex" ]; then
		echo "Flashing ATMega2560 firmware..."
		avrdude -C"$TFTUPDATE/firmware/avrdude.conf" -v \
			-patmega2560 -cwiring \
			-P"$ARDUINOPORT" -b115200 -D \
			-Uflash:w:"$TFTUPDATE/firmware/firmware.hex":i
	fi

	# Flash Gen4 board firmware
	if [ -f "$TFTUPDATE/firmware/firmware4.hex" ]; then
		echo "Flashing ATMega32u4 firmware..."
		avrdude -C"$TFTUPDATE/firmware/avrdude.conf" -v \
			-patmega32u4 -cavr109 \
			-P"$ARDUINOPORT" -b57600 \
			-Uflash:w:"$TFTUPDATE/firmware/firmware4.hex":i
	fi
else
	echo "Firmware port not found or avrdude.conf missing — skipping firmware update"
fi

# --- Cleanup ---
echo ""
echo "Cleaning up..."
sudo rm -rf "$STAGING_DIR"

sudo mount -o remount,ro / 2>/dev/null
sudo mount -o remount,ro /boot 2>/dev/null

echo ""
echo "*****************************************************************"
echo "UPDATE COMPLETE. PLEASE REMOVE USB DRIVE AND SWITCH OFF IGNITION."
echo "*****************************************************************"
echo ""

# Restart the dashboard
echo "Restarting dashboard..."
cd "$TFTDASH_DIR"
./testsdl &

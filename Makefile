.PHONY: all image rebuild firmware display simulator test qemu qemu-b update-package clean clean-firmware clean-image clean-simulator setup help

BUILDROOT_DIR ?= buildroot-src
BUILDROOT_IMAGES = $(BUILDROOT_DIR)/output/images

all: image

# Build the full SD card image (firmware + display cross-compile + buildroot)
image: firmware
	buildroot-tftdash/build.sh build

# Rebuild image after app/asset changes (faster than full build)
rebuild: firmware
	buildroot-tftdash/build.sh rebuild

# Compile firmware .hex via arduino-cli
firmware:
	firmware/build.sh build

# Build display natively for local dev/testing
display:
	cd display && zig build

# Build simulator
simulator:
	dotnet build simulator/src/TftDashSimulator/TftDashSimulator.csproj

# Run all tests
test:
	cd display && zig build test
	dotnet test simulator/tests/TftDashSimulator.Tests/

# Boot the SD card image in QEMU from slot A (root=/dev/vda2).
# No display — QEMU cannot properly emulate the Pi's GPU.
qemu:
	cp $(BUILDROOT_IMAGES)/sdcard.img $(BUILDROOT_IMAGES)/sdcard-qemu.img
	qemu-system-aarch64 -machine virt -cpu cortex-a72 -smp 4 -m 1024 \
		-kernel $(BUILDROOT_IMAGES)/Image \
		-append "rw console=ttyAMA0,115200 root=/dev/vda2 rootfstype=ext4" \
		-drive file=$(BUILDROOT_IMAGES)/sdcard-qemu.img,format=raw,if=virtio \
		-display none -serial stdio

# Boot from slot B (root=/dev/vda3) for A/B update testing.
qemu-b:
	cp $(BUILDROOT_IMAGES)/sdcard.img $(BUILDROOT_IMAGES)/sdcard-qemu.img
	qemu-system-aarch64 -machine virt -cpu cortex-a72 -smp 4 -m 1024 \
		-kernel $(BUILDROOT_IMAGES)/Image \
		-append "rw console=ttyAMA0,115200 root=/dev/vda3 rootfstype=ext4" \
		-drive file=$(BUILDROOT_IMAGES)/sdcard-qemu.img,format=raw,if=virtio \
		-display none -serial stdio

# Build a USB update package (rootfs image + checksum)
update-package:
	buildroot-tftdash/build.sh update-package

# First-time setup: install Arduino board support and libraries
setup:
	firmware/build.sh setup

clean: clean-firmware clean-image clean-simulator
	cd display && rm -rf zig-out .zig-cache

clean-firmware:
	firmware/build.sh clean

clean-image:
	buildroot-tftdash/build.sh clean

clean-simulator:
	dotnet clean simulator/src/TftDashSimulator/TftDashSimulator.csproj

help:
	@echo "make            Build full SD card image (firmware + display + buildroot)"
	@echo "make rebuild    Rebuild image after app/asset changes"
	@echo "make firmware   Compile firmware .hex only"
	@echo "make display    Build display natively for local dev/testing"
	@echo "make simulator  Build the simulator"
	@echo "make test       Run all tests (display + simulator)"
	@echo "make qemu       Boot the SD card image in QEMU from slot A (serial console)"
	@echo "make qemu-b     Boot from slot B for A/B update testing"
	@echo "make update-package  Build a USB update package (rootfs + checksum)"
	@echo "make setup      First-time setup (Arduino board support + libraries)"
	@echo "make clean      Clean all build outputs"

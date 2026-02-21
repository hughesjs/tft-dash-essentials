.PHONY: all image rebuild firmware display simulator test qemu qemu-gui clean clean-firmware clean-image clean-simulator setup help

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

# Common QEMU flags for the virt machine. Uses virtio devices to avoid the
# raspi3b machine's incomplete hardware emulation (no GPU, broken sdhost, no
# pinctrl). The kernel + rootfs are the same aarch64 build that runs on real
# Pi hardware. virtio-gpu-gl-pci enables virgl 3D so mesa's virgl gallium
# driver can render through KMSDRM.
QEMU_COMMON = qemu-system-aarch64 -machine virt -cpu cortex-a72 -smp 4 -m 1024 \
	-kernel $(BUILDROOT_IMAGES)/Image \
	-append "rw console=ttyAMA0,115200 root=/dev/vda2 rootfstype=ext4" \
	-drive file=$(BUILDROOT_IMAGES)/sdcard-qemu.img,format=raw,if=virtio \
	-device virtio-gpu-gl-pci,edid=on,xres=1024,yres=600

# Boot in QEMU with serial console (headless — GPU renders offscreen)
qemu:
	cp $(BUILDROOT_IMAGES)/sdcard.img $(BUILDROOT_IMAGES)/sdcard-qemu.img
	$(QEMU_COMMON) -display egl-headless -serial stdio

# Boot in QEMU with a GUI window showing the display output
qemu-gui:
	cp $(BUILDROOT_IMAGES)/sdcard.img $(BUILDROOT_IMAGES)/sdcard-qemu.img
	$(QEMU_COMMON) -display gtk,gl=on -serial stdio

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
	@echo "make qemu       Boot the SD card image in QEMU (headless, serial console)"
	@echo "make qemu-gui   Boot the SD card image in QEMU with GUI window"
	@echo "make setup      First-time setup (Arduino board support + libraries)"
	@echo "make clean      Clean all build outputs"

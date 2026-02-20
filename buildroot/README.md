# Buildroot Image

Builds a minimal SD card image for the Raspberry Pi 3 that boots directly into the TFT Dash display application. Replaces the old hand-configured Raspbian/RetroPie image.

## What's in the image

- Linux kernel (Pi 3 optimised)
- Busybox (minimal userspace)
- SDL2 (KMS/DRM backend)
- avrdude (for firmware flashing via USB updates)
- TFT Dash binary + BMP assets
- Read-only root filesystem

No systemd, no desktop, no network services — just the kernel and the dash app.

## Prerequisites

1. **Buildroot** cloned as a sibling directory:
   ```bash
   git clone https://github.com/buildroot/buildroot.git ../buildroot-src
   ```

2. **Display binary** pre-built for ARM:
   ```bash
   cd ../tftdashdisplay
   zig build -Dtarget=arm-linux-gnueabihf -Doptimize=ReleaseSafe
   ```

3. Standard build tools (`gcc`, `make`, `patch`, `unzip`, `rsync`, etc.)

## Building

```bash
# First build (downloads toolchain + kernel, takes 20-30 mins)
./build.sh

# After changing app binary or assets
./build.sh rebuild

# Full clean
./build.sh clean
```

The output is `buildroot-src/output/images/sdcard.img`.

## Flashing

```bash
sudo dd if=../buildroot-src/output/images/sdcard.img of=/dev/sdX bs=4M status=progress
```

Replace `/dev/sdX` with your SD card device.

## Structure

```
buildroot/
├── build.sh                  Build wrapper script
├── configs/
│   └── tftdash_defconfig     Buildroot configuration
├── package/tftdash/
│   ├── Config.in             Package menu entry
│   └── tftdash.mk            Package recipe (installs pre-built binary + assets)
├── board/
│   ├── genimage.cfg          SD card partition layout
│   └── post-image.sh         Generates the final SD card image
├── overlay/                  Files overlaid onto the root filesystem
│   ├── boot/
│   │   └── config.txt        Pi boot config (DPI display, fast boot)
│   └── etc/
│       ├── fstab             Read-only root + tmpfs mounts
│       └── init.d/
│           └── S99tftdash    Init script (starts the dashboard)
├── external.desc             BR2_EXTERNAL descriptor
├── external.mk               External package includes
└── Config.in                 External package menu
```

## How it works

This uses Buildroot's [BR2_EXTERNAL](https://buildroot.org/downloads/manual/manual.html#outside-br-custom) mechanism. Our custom configs, packages, and overlays live in this directory while Buildroot itself is a separate checkout. The `build.sh` script ties them together.

The Zig cross-compiler builds the `testsdl` binary separately. The Buildroot package recipe (`tftdash.mk`) just copies the pre-built binary and BMP assets into the image — it doesn't compile anything.

## Boot sequence

1. GPU loads `bootcode.bin` → `start.elf` → reads `config.txt`
2. Kernel boots with `quiet fastboot noswap ro` — minimal console output
3. Busybox init runs `/etc/init.d/S99tftdash start`
4. Dashboard appears

Target: kernel to app in ~3-5 seconds.

## Customising

- **Boot config**: Edit `overlay/boot/config.txt`
- **Add packages**: Edit `configs/tftdash_defconfig`, add `BR2_PACKAGE_FOO=y`
- **Change init**: Edit `overlay/etc/init.d/S99tftdash`
- **Full reconfigure**: `cd ../buildroot-src && make BR2_EXTERNAL=../buildroot menuconfig`

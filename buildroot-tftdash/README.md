# Buildroot Image

Builds a minimal SD card image for the Raspberry Pi 3 that boots directly into the TFT Dash display application.

## What's in the image

- Linux kernel (Pi 3 optimised)
- Busybox (minimal userspace)
- SDL3 (KMS/DRM backend, statically linked)
- avrdude (for firmware flashing via USB updates)
- TFT Dash binary + BMP assets
- Read-only root filesystem
- A/B rootfs update support (two rootfs partitions + update scripts)

No systemd, no desktop, no network services — just the kernel and the dash app.

## SD Card Partition Layout

The image uses an A/B rootfs scheme for atomic updates:

```
p1  boot      32MB   FAT32   kernel, DTBs, firmware, config.txt, cmdline.txt
p2  rootfs_a  512MB  ext4    active Buildroot rootfs (read-only)
p3  rootfs_b  512MB  ext4    standby — dd target for updates
p4  data      16MB   ext4    persistent data (rw) — update log, future use
```

Both rootfs slots contain the same image on initial flash. The active slot is determined by the `root=` parameter in `cmdline.txt` on the boot partition. See `docs/ab-update-plan.md` for full details.

## Prerequisites

1. **Buildroot** cloned as a sibling directory:
   ```bash
   git clone https://github.com/buildroot/buildroot.git ../buildroot-src
   ```

2. **Display binary** pre-built for ARM:
   ```bash
   cd ../display
   zig build -Dtarget=aarch64-linux-gnu -Doptimize=ReleaseSafe
   ```

3. Standard build tools (`gcc`, `make`, `patch`, `unzip`, `rsync`, etc.)

## Building

```bash
# First build (downloads toolchain + kernel, takes 20-30 mins)
./build.sh

# After changing app binary or assets
./build.sh rebuild

# Build a USB update package
./build.sh update-package

# Full clean
./build.sh clean
```

The output is `../buildroot-src/output/images/sdcard.img`.

## Flashing

```bash
sudo dd if=../buildroot-src/output/images/sdcard.img of=/dev/sdX bs=4M status=progress
```

Replace `/dev/sdX` with your SD card device.

## USB Updates

After building a new image, create an update package:

```bash
./build.sh update-package
```

This produces an `update-package/tftupdate/` directory containing:
- `rootfs.img` — full ext4 rootfs image
- `rootfs.img.sha256` — SHA-256 checksum
- `firmware.hex` — Arduino firmware (if available)

Copy the `tftupdate/` directory to a FAT32 USB stick. Plug it into the Pi and reboot — the `S10usbupdate` init script detects the update at boot, writes the image to the standby rootfs partition, flips `cmdline.txt`, and reboots into the new slot.

## Structure

```
buildroot-tftdash/
├── build.sh                  Build wrapper script
├── configs/
│   └── tftdash_defconfig     Buildroot configuration
├── package/tftdash/
│   ├── Config.in             Package menu entry
│   └── tftdash.mk            Package recipe (installs pre-built binary + assets)
├── board/
│   ├── genimage.cfg          SD card partition layout (4-partition A/B scheme)
│   └── post-image.sh         Generates cmdline.txt and final SD card image
├── overlay/                  Files overlaid onto the root filesystem
│   ├── boot/
│   │   └── config.txt        Pi boot config (DPI display, fast boot)
│   ├── data/
│   │   └── .keep             Mountpoint for persistent data partition
│   ├── etc/
│   │   ├── fstab             A/B-aware mounts (no root entry, /data on p4)
│   │   └── init.d/
│   │       ├── S10usbupdate  Boot-time USB update scan
│   │       └── S99tftdash    Init script (starts the dashboard)
│   └── usr/bin/
│       └── tft-update.sh     Core A/B update script
├── external.desc             BR2_EXTERNAL descriptor
├── external.mk               External package includes
└── Config.in                 External package menu
```

## How it works

This uses Buildroot's [BR2_EXTERNAL](https://buildroot.org/downloads/manual/manual.html#outside-br-custom) mechanism. Our custom configs, packages, and overlays live in this directory while Buildroot itself is a separate checkout. The `build.sh` script ties them together.

The Zig cross-compiler builds the `dashboard` binary separately. The Buildroot package recipe (`tftdash.mk`) just copies the pre-built binary and BMP assets into the image — it doesn't compile anything.

## Boot sequence

1. GPU loads `bootcode.bin` → `start.elf` → reads `config.txt`
2. Kernel boots with `quiet fastboot noswap ro` — minimal console output
3. Busybox init runs `/etc/init.d/S10usbupdate start` — scans for USB update stick
4. If update found: writes to standby partition, reboots into new slot
5. If no update: `/etc/init.d/S99tftdash start` — dashboard appears

Target: kernel to app in ~3-5 seconds (add ~1 second when no USB present due to enumeration wait).

## Customising

- **Boot config**: Edit `overlay/boot/config.txt`
- **Add packages**: Edit `configs/tftdash_defconfig`, add `BR2_PACKAGE_FOO=y`
- **Change init**: Edit `overlay/etc/init.d/S99tftdash`
- **Update script**: Edit `overlay/usr/bin/tft-update.sh`
- **Full reconfigure**: `cd ../buildroot-src && make BR2_EXTERNAL=../buildroot-tftdash menuconfig`

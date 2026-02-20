################################################################################
#
# tftdash — motorcycle dashboard display application
#
# This package installs a pre-built ARM binary and BMP assets.
# The binary should be cross-compiled beforehand with:
#   cd display && zig build -Dtarget=arm-linux-gnueabihf -Doptimize=ReleaseSafe
#
################################################################################

TFTDASH_VERSION = 1.0
TFTDASH_SITE_METHOD = local
TFTDASH_SITE = $(realpath $(BR2_EXTERNAL_TFTDASH_PATH)/../display)
TFTDASH_DEPENDENCIES = sdl2

TFTDASH_INSTALL_DIR = $(TARGET_DIR)/home/pi/tftdash

define TFTDASH_INSTALL_TARGET_CMDS
	# Install pre-built binary
	$(INSTALL) -D -m 0755 $(TFTDASH_SITE)/zig-out/bin/testsdl \
		$(TFTDASH_INSTALL_DIR)/testsdl

	# Install theme assets (new directory layout)
	mkdir -p $(TFTDASH_INSTALL_DIR)/assets/themes
	for theme_dir in $(TFTDASH_SITE)/assets/themes/*/; do \
		theme=$$(basename $$theme_dir); \
		mkdir -p $(TFTDASH_INSTALL_DIR)/assets/themes/$$theme; \
		cp $$theme_dir/*.bmp $(TFTDASH_INSTALL_DIR)/assets/themes/$$theme/ 2>/dev/null || true; \
	done

	# Install boot image
	if [ -f $(TFTDASH_SITE)/assets/bootimage.png ]; then \
		$(INSTALL) -D -m 0644 $(TFTDASH_SITE)/assets/bootimage.png \
			$(TFTDASH_INSTALL_DIR)/bootimage.png; \
	fi
endef

$(eval $(generic-package))

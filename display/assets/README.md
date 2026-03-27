# TFT Dash Assets

This directory contains all graphical assets for the TFT Dash display application.

## Structure

```
assets/
└── themes/
    ├── default/       (91 files) - White/default theme
    ├── blue/          (81 files) - Blue theme
    ├── bright/        (84 files) - High-contrast bright theme
    ├── dark/          (81 files) - Dark theme
    ├── green/         (81 files) - Green theme
    ├── night/         (81 files) - Night theme
    ├── orange/        (81 files) - Orange theme
    ├── red/           (81 files) - Red theme
    ├── yellow/        (81 files) - Yellow theme
```

Each theme directory also contains its own `{theme_name}thumb.bmp` preview thumbnail.

**Total: 751 BMP files**

## Theme Files

Each theme directory contains bitmap assets for:

- **Rev gauge segments** (`R0.bmp` through `R12-13.bmp`)
- **Speed numbers** (`Speednumbers.bmp`)
- **Small numbers** for temperature, trip, odometer (`Smallnumbers.bmp`)
- **Indicators** (`Indicateleft.bmp`, `Indicateright.bmp`, `Highbeamlight.bmp`, `Neutrallight.bmp`, `Oillight.bmp`)
- **Warning badges** (`Lowtyrebadge.bmp`, `Lowoilbadge.bmp`, `Overheatbadge.bmp`, `Fuelwarningbadge.bmp`)
- **Gauges** (`Fuelgauge.bmp`, `Coolant.bmp`)
- **Info panels** (`Infotop.bmp`, `Infobottom.bmp`, `Mileinfo.bmp`)
- **Menu graphics** (`Menuoptions.bmp`, `Speedcorrection.bmp`, `Settime.bmp`, `Setunits.bmp`)
- **Navigation graphics** (`Navbg.bmp`, `Navgfx.bmp`)
- **TPMS graphics** (`tyreicon.bmp`, `tyresignal.bmp`, `tyretop.bmp`, `tyrebottom.bmp`)

## Adding a New Theme

1. Create a new directory: `themes/{theme_name}/`
2. Copy BMP files from an existing theme
3. Modify colours/contrast as needed
4. Create a thumbnail: `{theme_name}thumb.bmp` inside the theme directory
5. Update `main.c` to add theme selection logic

## File Format

- **Format**: Windows BMP (bitmap)
- **Colour depth**: Varies by file (typically 24-bit RGB)
- **Dimensions**: Specific to each graphic element
- **Screen resolution**: 1024x600 target display

## Theme Selection

Theme is controlled by:
1. **Firmware setting** - Stored in EEPROM, sent via serial protocol (field 20)
2. **Menu system** - On-screen theme selector in display app
3. **Auto day/night** - Can switch based on ambient light sensor

## Loading Mechanism

Assets are loaded at startup via `asset_store_load_theme()` into a hash map. Textures are retrieved by name:

```c
tex("Coolant.bmp")                          // current theme
tex_from("blue", "bluethumb.bmp")           // specific theme
```

## Deployment

When deploying to Raspberry Pi, ensure:
- `dashboard` binary and `assets/` directory are in the same location
- Relative path `assets/themes/` must be accessible from working directory
- All BMP files have correct permissions (readable)

## Asset Creation Guidelines

When creating or modifying BMP assets:

1. **Maintain dimensions** - Display code expects specific sizes
2. **Use consistent colour palette** per theme
3. **Test on target hardware** - Colours may appear different on TFT display
4. **Optimize file size** - Use RLE compression if supported
5. **Name consistently** - Follow existing naming conventions

## Memory Considerations

- BMPs are loaded into SDL surfaces at startup
- Each surface is converted to an SDL texture
- Total texture memory: ~50-100MB depending on theme
- Theme changes require reloading all surfaces

## Troubleshooting

**Black screen on startup:**
- Check `assets/themes/` path is correct relative to binary
- Verify BMP files exist and are readable
- Check stderr output for SDL_LoadBMP errors

**Wrong colours/missing graphics:**
- Ensure correct theme directory name
- Verify all required BMPs exist in theme directory
- Check file permissions

**Out of memory:**
- Reduce number of loaded themes
- Use lower colour depth BMPs
- Close other applications

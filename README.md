# M6 Reborn

Give a salvaged M6 fitness-band display a second life with a **Raspberry Pi Pico**, FreeRTOS, LVGL and a Windows watch-face designer.

This firmware runs on the **original RP2040 Pico** and drives the display directly over SPI. It does not flash or replace the original Telink processor. The tested TFT has a 160×80 controller area with a damaged right edge: faces use the working **120×80** area and the remaining 40 columns stay black.

![Example crest asset](assets/cadillac-crest.png)

The crest is an example face, not the project identity. Artwork provenance: [assets/SOURCE.md](assets/SOURCE.md).

## Features

- Native Windows designer: preview, drag objects, add text/rectangles/crest, edit properties, save/open JSON and export PNG.
- Validated USB scene uploads with atomic commit, supporting up to 12 elements.
- Startup design generation, firmware build/flash controls and source editor with automatic backups.
- Constant-brightness moving crest, static crest and numbered diagnostic grid.
- Clock, stopwatch, simulated vehicle dashboard, diagnostics, settings and USB navigation.
- Row-stride-aware RGB565 display driver, fixing slanted text and zigzag partial redraws.

The current refresh setting is 33 ms. The tested wiring at 2 MHz SPI measured approximately **9 visible frame updates/second**; this is not a 30/60 FPS claim. Custom uploaded designs are static; Animated crest selects the built-in animation.

## Start here

1. Read [wiring](docs/WIRING.md). Display connector pins, GPIO names and physical Pico pins are different numbering systems.
2. Follow [requirements and setup](docs/SETUP.md), configure your own Pico and perform smoke/display checks.
3. Double-click **Launch Designer.cmd**.
4. Add/select objects, edit properties, click **Apply properties**, then **Send to watch**.

Live designs are temporary. To keep one after power-off: **Program / firmware → Save startup design → Build firmware → Flash firmware**. See the [designer guide](desktop/README.md).

## Tested wiring

| Pico signal | Physical pin | Display connector | Role |
|---|---:|---|---|
| GP19 | 25 | 3 SDA | SPI MOSI |
| GP18 | 24 | 4 SCL | SPI clock |
| GP20 | 26 | 5 RS | Data/command |
| GP21 | 27 | 6 RES | Reset |
| GP17 | 22 | 7 CS | Chip select |
| GND | 23 / 28 | 8 and 13 GND | Common ground |
| 3V3 OUT | 36 | 10 VCC | Controller power |
| 3V3 OUT through **9.566 kΩ** | 36 | 12 LEDA | Low-current backlight branch |
| GND | 23 / 28 | 11 LEDK | Backlight return |

VCC and LEDA are separate branches. The resistor belongs in the LEDA branch, not display VCC. The LED's nominal rating is unknown; the tested resistor limits theoretical current to at most 0.345 mA at 3.3 V. Another display revision may have a different pinout. [Full wiring](docs/WIRING.md).

## Requirements

- Original RP2040 Pico, USB data cable, isolated compatible SPI TFT and wiring.
- Windows 10/11, PowerShell 5.1 or 7, Git and Python 3.12 with Tkinter.
- pyserial 3.5 and Pillow 12.3.0, pinned in `requirements.txt`.
- Portable CMake 3.31.8, Ninja 1.12.1 and Arm GNU 14.2.Rel1; fetch script provided.
- Pico SDK 2.1.1, FreeRTOS Kernel V11.2.0, LVGL v9.2.2 and SDK-pinned TinyUSB; exact source commits are checked by the dependency script.

Dependencies/toolchains are fetched locally rather than committed to Git. The portable-tool script does not change system PATH, registry or drivers. Allow several gigabytes for extracted tools and builds. [Setup instructions](docs/SETUP.md).

## Build and flash

After setup, from PowerShell in the checkout:

```powershell
.\scripts\Build.ps1 -Target All
# Enter BOOTSEL on the intended Pico after verifying wiring:
.\scripts\Flash.ps1 -Mode OS -DisplayHardwareConfirmed
```

Targets: Smoke, DisplayTest, OS, Headless, All. Full firmware: `dist/M6_Reborn.uf2`. Current settings: profile 0, rotation 0, offsets (1,26), inversion on, BGR off, SPI0 mode 0 at 2 MHz. A different panel may need different initialization.

Prebuilt UF2s and checksums are in [dist](dist). Source builds also generate ELF/BIN/manifests and validate RP2040 family, addresses, ARM ELF and binary round-trip. Local device identity and hardware confirmation records are excluded from Git; flashing verifies the configured board before writing.

## Project map

| Folder | Contents |
|---|---|
| src / include / config | Firmware, TFT driver, RTOS/LVGL settings and viewport limits |
| desktop | Tkinter designer, USB uploader and validation tests |
| scripts | Dependency/tool fetch, build, flash, image checks and serial diagnostics |
| assets | Example artwork, generated crest and provenance |
| dist | Prebuilt UF2 images and SHA-256 checksums |
| docs | Setup, wiring, USB protocol, architecture and verification |

## Status and limits

The prototype displayed readable text, app pages and the crest. Live USB uploads, invalid-bound rejection, cancellation, build/flash and task health were tested. Four designer tests pass; a generated startup scene compiles. [Verification details](docs/VERIFICATION.md).

The controller behaves as ST7735-compatible; its exact model was not identified by readback. The panel is damaged. Touch/buttons, BLE, sensors, battery measurement, charging, a battery-backed RTC and long-duration soak are not implemented/verified. Vehicle telemetry is simulated. Clock/settings/live designs reset at power loss unless a design is embedded at build time.

Disconnect power before rewiring, keep the original battery disconnected, and isolate display signals from the original band processor. Old SWire/Telink programmer wiring is a separate experiment and is not used here.

Research references: [rbaron's article](https://rbaron.net/blog/2021/07/06/Reverse-engineering-the-M6-smart-fitness-band) and [m6-reveng](https://github.com/rbaron/m6-reveng). Telink SoC pins are not Pico GPIOs. See [third-party notices](THIRD_PARTY_NOTICES.md).

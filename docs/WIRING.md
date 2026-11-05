# Tested display wiring

This is the display connector, not the original band PCB's test pads. Pico GPIO labels and physical header pins differ. The operator verified this prototype's connector; another revision may differ.

| Pico signal | Physical header pin | Display connector pin | Function |
|---|---:|---:|---|
| GP19 | 25 | 3 SDA | SPI0 MOSI |
| GP18 | 24 | 4 SCL | SPI clock |
| GP20 | 26 | 5 RS | Data/command |
| GP21 | 27 | 6 RES | Reset, active low |
| GP17 | 22 | 7 CS | Chip select, active low |
| GND | 23 / 28 | 8 and 13 | Common ground |
| 3V3 OUT | 36 | 10 VCC | Controller power |
| 3V3 OUT through 9.566 kΩ | 36 | 12 LEDA | Separate backlight branch |
| GND | 23 / 28 | 11 LEDK | Backlight cathode |
| Unconnected | — | 1 TP0, 2 TP1, 9 NC | Unused |

```text
Pico 3V3 OUT ────────────── display pin 10 VCC
            └─ 9.566 kΩ ── display pin 12 LEDA
Pico GND ───────────────── display pins 8, 13 and 11 LEDK
```

VCC and the backlight are separate circuits. The prototype showed faint readable content with LEDA/LEDK unconnected. The operator later confirmed glow with a measured 9.566 kΩ series resistor. The theoretical current ceiling is 3.3 V / 9566 Ω ≈ 0.345 mA. Actual LED current and normal brightness rating remain unmeasured. The resistor belongs in the LEDA branch, not display VCC. Firmware never drives the backlight from a GPIO.

Remove USB and other supplies before rewiring. The original battery is disconnected and the old processor must be isolated from the SPI nets. Do not combine old GP22/GP20 SWire wiring with this layout. Telink SoC pin numbers in the reference article do not map to Pico GPIOs.

Driver: profile 0, rotation 0, offsets (1,26), SPI0 mode 0 at 2 MHz, inversion on, BGR off. The controller is treated as ST7735-compatible; no exact ID was read. Grid cells 1–6, 9–14, 17–22 and 25–30 established the left 120×80 working area. Rightmost 40 columns are forced black.

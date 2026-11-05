# Backlight

Controller power and LED power are separate. Display pin 10 VCC connects directly to Pico 3V3 OUT (physical 36). The tested LED branch is 3V3 OUT → measured 9.566 kΩ resistor → display pin 12 LEDA, with pin 11 LEDK to GND.

The operator saw glow with this circuit. The theoretical current ceiling is 0.345 mA at 3.3 V; LED voltage drop makes actual current lower. Nominal backlight current and full-brightness operation were not measured. Do not treat this prototype value as a panel datasheet rating. Firmware uses no backlight GPIO.

Pins 11/12 disconnected still allowed faint readable content under ambient light. Lack of backlight is not proof of a SPI or initialization failure. Disconnect USB and all supplies before rewiring. Do not bypass the resistor or place it in series with pin 10. See WIRING.md for the full mapping.

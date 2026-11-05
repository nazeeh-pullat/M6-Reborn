# Prototype verification

Observed on the original RP2040 Pico and salvaged 160×80 M6 TFT with a 120×80 working area, 2026-11-05:

- Onboard LED/SDK USB smoke ran before display testing. The operator observed readable PICO OS text, changing app pages and the example crest.
- Zigzag/slanted partial rows were traced to ignored LVGL row padding. Flush now honors `header.stride`, transmits only real pixels and masks unused columns. Startup self-test checks padded RGB565 rows and the usable edge.
- Built-in opacity fading was removed. The operator confirmed smooth movement but low apparent FPS. Motion was then expanded from four pixels over 2.2 seconds to 15 pixels over one second; refresh interval changed from 100 to 33 ms.
- A four-second sample counted about 9.2 completed changed frames/second at 2 MHz SPI. This is a short prototype observation, not a long soak or 30 FPS guarantee. Latest faster motion still needs the operator's visual assessment.
- A real two-element USB scene committed. An invalid rectangle was rejected; cancel preserved the committed version. Test restored animation.
- Four designer tests passed. Generated startup-scene C header compiled. The native window, drag interaction and live upload button were checked.
- The first app build exposed Windows PowerShell stderr handling. Corrected app build/flash functions completed; the public script also explicitly handles native stderr.
- RTOS reported healthy tasks, roughly 40 KB free RTOS heap and 43 KB free LVGL heap in the live test.

Device identities, local confirmation records, raw session logs, downloaded dependencies and build caches are not transferable approvals for other hardware and are excluded from Git.

No exact controller readback, full-brightness backlight characterization, calibrated colors, physical controls, sensor integration or long-duration soak has been completed. A UF2 copy is distinct from runtime/visible confirmation.

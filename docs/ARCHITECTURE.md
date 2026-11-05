# Firmware architecture

| Task | Priority | Stack allocation | Schedule / ownership |
|---|---:|---:|---|
| UI | 2 | 2048 words /8192 bytes | LVGL handler every20ms; snapshot refresh at most4Hz; owns every LVGL and TFT operation |
| AppLogic | 1 | 1536 words /6144 bytes |20ms loop, bounded USB input, navigation, stopwatch and clock state |
| SimTelemetry | 1 | 512 words /2048 bytes |200ms synthetic vehicle update; no hardware or vehicle I/O |
| SystemMonitor | 1 | 1024 words /4096 bytes |500ms heartbeat/watchdog check; diagnostics every5s; GP25 onboard LED |

FreeRTOS is pinned to core0 with one configured core. A mutex protects shared application snapshots, settings revisions and progress timestamps. UI copies the snapshot under that mutex and releases it before rendering or SPI operations. Other tasks never call LVGL. LVGL's OS abstraction is disabled intentionally because the single UI owner serializes its API, while application state remains protected by FreeRTOS.

SPI0 is mode0, MSB-first, default2MHz. CS and D/C are manual GPIO outputs. RGB565 pixels are converted from LVGL native16-bit words to high-byte-first wire bytes in bounded320-byte chunks. Rendering uses two3200-byte partial draw buffers, each160x10 pixels, rather than a full-screen framebuffer. LVGL row stride is honored, including padded odd-width rows; padding is never sent as pixels. The damaged right 40 columns are masked black.

TFT profiles:0 minimal M6-compatible candidate,1 extended ST7735S candidate including power/frame/gamma commands,2 generic minimal ST7735 candidate. Actual text is observed with profile0, rotation0, offsets1,26, inversion on, BGR off. Four rotations are supported; odd rotations change logical resolution to80x160, but the primary UI layout is optimized for landscape160x80. Color order and other profiles remain physically unverified.

Initialization uses hardware reset, software reset, sleep-out delays, selected candidate setup, MADCTL orientation,16-bit COLMOD, inversion selection, normal mode and display-on. Every flush clips invalid bounds, sets the window and sends pixel data. No display-readback wire is connected, so software logs confirm commands sent, not identification or visible output.

Monitor enables a5-second watchdog. It feeds only while UI/AppLogic/SimTelemetry progress timestamps are at most3seconds old. If monitor itself stalls, feeding also stops. Allocation failures, assertions and stack-overflow hooks print a fatal message where possible and request watchdog recovery. A normal software BOOTSEL transition may set the reset-reason flag; this alone is not evidence of an application watchdog fault.

Runtime commands are bounded ASCII lines; overlong input is discarded. Settings changes are versioned under the model mutex and applied only by the UI owner. No configuration/clock state is persisted to flash. User controls currently use USB; no touch/button wiring is assumed.

The full UI adds static crest (page 7), grid (8), constant-brightness animation (9) and a transactional custom scene (10). Default is animation unless a compiled startup design is selected. The original UI contains a home launcher, clock/date, stopwatch, synthetic CTS dashboard, diagnostics, settings summary and system information. Automatic demo changes page every8s. UI fonts are Montserrat10/12/20 on black, white text, red accent/warning. Brightness is external hardware; the firmware never drives the backlight. See WIRING.md for the tested current-limited external branch.

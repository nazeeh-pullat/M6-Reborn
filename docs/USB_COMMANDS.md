# USB commands and designer protocol

Use the SDK USB CDC device configured in `config/device.json`, 115200 baud, DTR asserted. COM numbers may change. Commands are printable ASCII ending in newline; send one at a time and wait for `OK` or `ERROR`. `status` finishes at the `SIM` line. Lines longer than 127 characters are discarded.

| Command | Behavior |
|---|---|
| help / ? | List commands |
| status | Task/heap/page, viewport, stride/frame counts, scene, clock, stopwatch, simulated telemetry |
| n / p | Next/previous of 11 pages, demo off |
| app 0 .. app 10 | Home, Clock, Stopwatch, simulated dashboard, Diagnostics, Settings, Info, Logo, Grid, Animation, Designer |
| animate / logo / grid | Select animation / static crest / numbered grid, demo off |
| design show | Show last committed custom design |
| demo 1 / demo 0 | Enable/disable eight-second page cycling |
| start / stop / reset | Stopwatch controls |
| time EPOCH | UTC Unix seconds; supply current PC time |
| tz MINUTES | Display offset east of UTC; Riyadh is 180 |
| logging 0 / logging 1 | Disable/enable periodic monitor output |
| profile 0 .. profile 2 | Reinitialize candidate TFT profile |
| rotation 0 .. rotation 3 | Change orientation/default offsets |
| offset X Y | Set offsets, each 0..80 |
| invert 0/1 / bgr 0/1 | Change panel color/inversion configuration |
| spi HZ | Set 100000..16000000 Hz and reinitialize |
| test | RGB pattern then return to UI |

The tested configuration is profile 0, rotation 0, offsets 1,26, inversion on, BGR off, SPI 2000000. Other profiles/colors/rotations are not physically verified. Main UI is designed for the tested landscape viewport. Display-test firmware supports panel commands/test and `release` (signal pins return to inputs), but no OS pages.

## Transactional custom scenes

`design caps` replies with `DESIGN_CAPS version=1 width=120 height=80 max_elements=12` (no OK). All other design commands acknowledge with OK or ERROR.

```text
design begin 000000 0
design rect 0 0 120 2 4488FF
design crest 4 8 112
design text 29 65 12 FFFFFF CADILLAC
design commit
```

`begin HEX ANIMATE` creates staging state; the app uses ANIMATE=0 for static custom faces. Firmware also accepts 1 for crest opacity pulsing, which can appear to flash on this panel. Built-in motion uses constant brightness instead.

Rectangle: `design rect X Y W H HEX`. Text: `design text X Y FONT HEX TEXT`, fonts 10/12/20, 1..48 printable ASCII characters from the app. Crest: `design crest X Y W`, width 8..112, proportional height `(W*45+56)/112` using integer division. Rectangles and crests must fit inside 120×80; text origins must be inside and remaining content is clipped.

`design commit` atomically copies staging to the displayed model, increments version and selects page 10. `design cancel` discards staging. Rejecting an element does not automatically cancel staging: hosts must cancel after an error rather than committing a partial scene. The supplied uploader does this. Scene count is at most 12. Staging/live scenes are RAM-only; the desktop's generated startup header embeds a design at compile time.

No USB command reads a real vehicle or band battery. Clock/settings reset at power loss; only a compiled startup design persists.

# M6 Reborn Designer

Complete [setup](../docs/SETUP.md), then double-click **Launch Designer.cmd**. The native Tkinter app uses Python, Pillow and pyserial; no standalone EXE is supplied.

## Design and upload

1. Add text, rectangles or the example crest. Select in the preview/list; drag to move.
2. Edit JSON properties and click **Apply properties**. Coordinates/sizes are pixels. Colors use six hex digits without `#`. Text supports 1–48 printable ASCII characters and font sizes 10, 12 or 20.
3. **Send to watch** validates and uploads the whole scene. It becomes visible only after commit.
4. Save/Open JSON stores designs on the PC. Export PNG saves a 120×80 preview. The preview approximates the watch's Montserrat font.

Maximum 12 elements. The damaged 40 columns stay black. List order is draw order. Custom faces are static. **Animated crest** selects built-in motion; **Static crest**, **Numbered grid** and **Device status** are also available.

## Permanent startup and programming

Live uploads are temporary. Choose **Program / firmware → Save startup design → Build firmware → Flash firmware** to retain a design after power-off. To restore moving crest startup, choose **Use built-in animation at startup**, then Build and Flash.

Flash validates the image, local hardware records and the serials in `config/device.json`. If automatic BOOTSEL fails, hold BOOTSEL while reconnecting the intended Pico, then retry Flash. Leave verified wiring unchanged.

The source editor opens selected UI/runtime/config files. Saves create timestamped backups under `backups`. Build errors appear in the app log; complete subprocess output goes to `logs/designer-operations.log`. Keep the app open until a build/flash finishes. Applying properties or editing source does not automatically build/upload.

## Tests

`test_designer.py` checks bounds/text validation, C-string escaping and cancellation. `verify_device.py` exercises a real two-element upload, invalid-bound rejection, version preservation and animation frame counts, then restores animation.

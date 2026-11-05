# Firmware images

`M6_Reborn.uf2` is the current full firmware. The other UF2 files are the smoke, display-test and headless diagnostics. `SHA256SUMS.txt` records the release checksums.

Matching ELF, BIN and manifest files accompany the UF2s so the flash script can verify them. Source builds regenerate these files. Downloaded toolchains, source dependency trees and build intermediates are excluded from Git; the setup scripts fetch the pinned versions.

Use the smoke test first on new hardware. See [setup](../docs/SETUP.md) and [wiring](../docs/WIRING.md). A copied UF2 is not proof of visible display output.

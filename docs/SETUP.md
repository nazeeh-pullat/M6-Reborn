# Setup, requirements and first upload

Install Git for Windows and Python 3.12 with Tcl/Tk support. In PowerShell:

```powershell
git clone https://github.com/nazeeh-pullat/M6-Reborn.git
Set-Location M6-Reborn
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
.\scripts\Install-BuildTools.ps1
.\scripts\Fetch-Dependencies.ps1
.\scripts\Build.ps1 -Target All
```

The tool script downloads official CMake 3.31.8, Ninja 1.12.1 and Arm GNU 14.2.Rel1 archives into `tools`, without system PATH/registry/driver changes. Download hashes are recorded locally; they are records, not an independent trusted checksum allowlist. Dependency revisions are pinned in the fetch script. Allow several gigabytes for extracted sources/tools/builds. Review scripts before running and use an execution policy appropriate for your PC.

Build prefers `.venv/Scripts/python.exe`, then `PYTHON_EXECUTABLE`, then Python on PATH. The designer launcher uses the local virtual environment when present.

## Configure your own board

Connect the original RP2040 Pico alone, with display power/backlight disconnected. Hold BOOTSEL while connecting USB. List USB identities:

```powershell
.\scripts\Configure-Device.ps1 -ListDevices
```

Record the serial at the end of `USB\VID_2E8A&PID_0003\SERIAL`. To bootstrap, copy **only** `dist/picoos_smoke.uf2` to the `RPI-RP2` drive you physically identified. Smoke configures only onboard LED GPIO25 and USB. After restarting, list devices again: the SDK application uses VID 2E8A/PID 000A and a different serial. Record both:

```powershell
.\scripts\Configure-Device.ps1 -AppSerial YOUR_APP_SERIAL -BootSerial YOUR_BOOTSEL_SERIAL
.\.venv\Scripts\python.exe scripts\Capture-Serial.py --seconds 8 --label smoke
```

This writes ignored `config/device.json`. No prototype serial is assumed. Confirm LED activity and `SMOKE_OK` USB output before recording:

```powershell
.\scripts\Configure-Device.ps1 -SmokeRuntimeVerified
```

## Display checks

Unplug power. Check actual connector pinout, isolation from the old processor, common ground and 3.3 V suitability. Wire according to WIRING.md. Keep the backlight disconnected, or check a separate current-limited circuit. Only after actually performing those checks:

```powershell
.\scripts\Configure-Device.ps1 -DisplayWiringVerified -BacklightDisconnected
# Or, for a verified current-limited backlight circuit:
# .\scripts\Configure-Device.ps1 -DisplayWiringVerified -CurrentLimitedBacklightVerified
```

Enter BOOTSEL, then:

```powershell
.\scripts\Flash.ps1 -Mode DisplayTest -DisplayHardwareConfirmed
```

Observe actual text/colors on the screen. USB messages do not prove visible output. After confirming visible test output:

```powershell
.\scripts\Configure-Device.ps1 -DisplayVisibleVerified
# Re-enter BOOTSEL:
.\scripts\Flash.ps1 -Mode OS -DisplayHardwareConfirmed
```

These switches record operator checks; they do not electrically measure the board. Never copy another board's confirmation records. Future uploads can use the designer's Flash button, which requests BOOTSEL from the configured application and checks boot identity independently before copying.

## Designer and tests

Double-click `Launch Designer.cmd` or run `.venv\Scripts\python.exe desktop\app.py`. This is a Python app, not a standalone EXE. Read [designer usage](../desktop/README.md).

```powershell
.\.venv\Scripts\python.exe -m unittest discover -s desktop -p test_designer.py -v
.\.venv\Scripts\python.exe scripts\Test-Images.py
.\.venv\Scripts\python.exe desktop\verify_device.py
```

The last check requires configured hardware. It briefly uploads a test face, rejects an invalid rectangle and restores animation.

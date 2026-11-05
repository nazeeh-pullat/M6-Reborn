"""Enter BOOTSEL on the uniquely identified Pico SDK application via 1200 baud."""
import serial
import time
from pathlib import Path
from serial.tools import list_ports
from device_config import app_serial
devices=[p for p in list_ports.comports()
         if p.vid==0x2E8A and p.pid==0x000A
         and (p.serial_number or '').lower()==app_serial().lower()]
if len(devices)!=1: raise RuntimeError('Expected exactly the known Pico SDK USB device')
port=serial.Serial(devices[0].device,115200,timeout=.2,write_timeout=2)
try:
    try:
        port.baudrate=1200
    except serial.SerialException:
        # Windows may report device-gone while the 1200-baud request succeeds.
        # Treat it as a transition only after observing a matching BOOTSEL file.
        found=False
        for _ in range(40):
            for letter in 'DEFGHIJKLMNOPQRSTUVWXYZ':
                info=Path(letter+':/INFO_UF2.TXT')
                try:
                    if info.is_file() and 'Board-ID: RPI-RP2' in info.read_text(): found=True
                except OSError: pass
            if found: break
            time.sleep(.25)
        if not found: raise
finally:
    try: port.close()
    except OSError: pass
print('BOOTSEL request sent to known Pico; verify RPI-RP2 drive before copying firmware')

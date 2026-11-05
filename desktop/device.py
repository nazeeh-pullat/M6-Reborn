"""Connect only to the identified Pico; acknowledge each command before sending another."""
import time
from pathlib import Path
import serial
import json

def device_serial():
    path=Path(__file__).resolve().parent.parent/"config/device.json"
    if not path.is_file(): raise RuntimeError("Configure your Pico first: scripts/Configure-Device.ps1")
    return json.loads(path.read_text())["app_serial"].upper()
from serial.tools import list_ports
from model import commands

class Device:
    def __enter__(self):
        ports=[p for p in list_ports.comports() if p.vid==0x2E8A and p.pid==0x000A and
               (p.serial_number or '').upper()==device_serial()]
        if len(ports)!=1: raise RuntimeError('Known Pico not found. Connect its USB cable and close other serial tools.')
        self.port=serial.Serial(ports[0].device,115200,timeout=.1,write_timeout=2)
        self.port.dtr=True; time.sleep(.25); self.port.reset_input_buffer()
        return self
    def __exit__(self,*args): self.port.close()
    def command(self,line):
        self.port.write((line+'\n').encode('ascii')); self.port.flush()
        lines=[]; deadline=time.monotonic()+5
        while time.monotonic()<deadline:
            text=self.port.readline().decode('utf8',errors='replace').strip()
            if not text: continue
            lines.append(text)
            if text.startswith('ERROR'): raise RuntimeError(text)
            if text=='OK' or (line=='status' and text.startswith('SIM ')): return '\n'.join(lines)
        raise TimeoutError('No acknowledgement for '+line)
    def upload(self,scene):
        try:
            for line in commands(scene): self.command(line)
        except Exception:
            try: self.command('design cancel')
            except Exception: pass
            raise
        return 'Design displayed. Live upload is temporary; use Save startup + Build + Flash to retain it.'

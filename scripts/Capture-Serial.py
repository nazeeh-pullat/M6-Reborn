"""Capture the known Pico's SDK USB CDC diagnostics; no GPIO commands."""
import argparse
import json
import time
from pathlib import Path
import serial
from serial.tools import list_ports
from device_config import app_serial

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--seconds',type=float,default=12)
parser.add_argument('--label',default='serial')
parser.add_argument('--command',action='append',default=[])
args=parser.parse_args()
if not 1<=args.seconds<=60: parser.error('Capture must be 1..60 seconds')
root=Path(__file__).resolve().parent.parent
deadline=time.monotonic()+15
devices=[]
while time.monotonic()<deadline:
    devices=[p for p in list_ports.comports() if p.vid==0x2E8A and p.pid==0x000A
             and (p.serial_number or '').lower()==app_serial().lower()]
    if len(devices)==1: break
    time.sleep(.25)
if len(devices)!=1:
    raise RuntimeError('Expected Pico SDK CDC device not found; inspect USB before further flashing')
port_name=devices[0].device
data=bytearray()
with serial.Serial(port_name,115200,timeout=.2,write_timeout=2) as port:
    for command in args.command:
        port.write((command+'\n').encode('ascii'))
    end=time.monotonic()+args.seconds
    while time.monotonic()<end:
        chunk=port.read(port.in_waiting or 1)
        data.extend(chunk)
        if len(data)>1024*1024: raise RuntimeError('Capture exceeded size limit')
text=data.decode('utf-8',errors='replace')
stamp=time.strftime('%Y%m%dT%H%M%SZ',time.gmtime())
(root/'logs').mkdir(exist_ok=True)
log=root/'logs'/f'{args.label}-{stamp}.txt'
log.write_text(text,encoding='utf-8')
record={'utc':stamp,'port':port_name,'vid':'2E8A','pid':'000A',
        'serial':devices[0].serial_number,'seconds':args.seconds,'bytes':len(data),'log':str(log)}
log.with_suffix('.json').write_text(json.dumps(record,indent=2))
print(json.dumps(record,indent=2)); print(text)
if not text.strip(): raise RuntimeError('USB enumerated but no serial diagnostics received')

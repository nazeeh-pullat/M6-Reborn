"""Exercise non-destructive USB controls on PicoOS; no profile/power/wiring changes."""
import json
import re
import time
from pathlib import Path
import serial
from serial.tools import list_ports
from device_config import app_serial

devices=[p for p in list_ports.comports() if p.vid==0x2E8A and p.pid==0x000A
         and (p.serial_number or '').lower()==app_serial().lower()]
if len(devices)!=1: raise RuntimeError('Known Pico SDK USB device not found')
root=Path(__file__).resolve().parent.parent
transcript=[]
checks=[]
def require(condition,message):
    if not condition: raise AssertionError(message)
    checks.append(message)

with serial.Serial(devices[0].device,115200,timeout=.2,write_timeout=2) as port:
    def command(text):
        transcript.append('> '+text)
        port.write((text+'\n').encode('ascii'))
        deadline=time.monotonic()+3
        lines=[]
        while time.monotonic()<deadline:
            line=port.readline().decode('utf-8',errors='replace').strip()
            if not line: continue
            transcript.append(line); lines.append(line)
            if text=='status' and line.startswith('SIM '): return '\n'.join(lines)
            if text!='status' and (line=='OK' or line.startswith('ERROR')): return '\n'.join(lines)
        raise TimeoutError('No complete response to '+text)
    def status():
        text=command('status')
        return {key:int(value) for key,value in re.findall(r'(\w+)=(-?\d+)',text)}
    try:
        command('demo 0'); command('app 2'); command('stop'); command('reset')
        command('start'); time.sleep(1.1); command('stop'); a=status()
        require(a['page']==2,'Stopwatch app navigation')
        require(700<=a['ms']<=2500 and a['running']==0,'Stopwatch start and stop elapsed time')
        time.sleep(.4); b=status()
        require(a['ms']==b['ms'],'Stopped stopwatch remains stable')
        command('reset'); require(status()['ms']==0,'Stopwatch resets to zero')
        command('app 3'); c=status()
        require(c['page']==3 and 800<=c['rpm']<=6200 and 0<=c['speed']<=120,'Simulated dashboard ranges')
        require(80<=c['coolant']<=111 and 13600<=c['battery_mv']<=14400,'Simulated temperature/voltage ranges')
        command('app 5'); command('n'); require(status()['page']==6,'Next navigation')
        command('p'); require(status()['page']==5,'Previous navigation')
        command('time '+str(int(time.time()))); command('tz 180'); d=status()
        require(abs(d['utc']-int(time.time()))<=3 and d['tz_minutes']==180 and d['set']==1,'UTC clock and Riyadh timezone')
        require('ERROR' in command('spi 999999999'),'Invalid SPI rate rejected without reinitialization')
        require(d['tasks_ok']==1 and d['heap']>16000 and d['LVGL_free']>12000,'Task health and memory headroom')
        require(d['uptime']>=a['uptime'],'No reset during command checks')
        command('app 0'); command('demo 1'); first=status()
        time.sleep(8.3); last=status()
        require(last['demo']==1 and last['page']!=first['page'],'Automatic demonstration cycles pages')
        require(last['uptime']>first['uptime'] and last['tasks_ok']==1,'Runtime continues without watchdog reset')
        result={'passed':True,'checks':checks,'final_status':last}
    finally:
        # Leave the user's firmware in its automatic demo mode.
        try: command('demo 1')
        except Exception: pass
        stamp=time.strftime('%Y%m%dT%H%M%SZ',time.gmtime())
        (root/'logs'/f'runtime-commands-{stamp}.txt').write_text('\n'.join(transcript))
result['utc']=stamp
(root/'logs/runtime-verification.json').write_text(json.dumps(result,indent=2))
print(json.dumps(result,indent=2))

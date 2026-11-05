"""Exercise the real transaction protocol, preserve invalid transaction behavior, restore animation."""
import json
import re
import time
from device import Device
from model import DEFAULT

with Device() as device:
    print(device.upload(DEFAULT))
    status=device.command('status'); print(status)
    assert 'page=10' in status and 'elements=2' in status
    version=re.search(r'DESIGN version=(\d+)',status).group(1)
    device.command('design begin 000000 0')
    try: device.command('design rect 119 0 2 1 FFFFFF')
    except RuntimeError: pass
    else: raise AssertionError('Invalid rectangle was accepted')
    device.command('design cancel')
    assert 'DESIGN version='+version in device.command('status')
    device.command('animate'); first=device.command('status'); time.sleep(4); last=device.command('status')
    print(last)
    a=int(re.search(r'frames=(\d+)',first).group(1)); b=int(re.search(r'frames=(\d+)',last).group(1))
    print(json.dumps({'transaction_verified':True,'invalid_bounds_rejected':True,'approx_frames_per_second':round((b-a)/4,1)}))

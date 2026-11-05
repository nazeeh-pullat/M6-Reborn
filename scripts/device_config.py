import json
from pathlib import Path

def app_serial():
    path=Path(__file__).resolve().parent.parent/'config/device.json'
    if not path.is_file(): raise RuntimeError('Run scripts/Configure-Device.ps1 for your own Pico first.')
    return json.loads(path.read_text())['app_serial']

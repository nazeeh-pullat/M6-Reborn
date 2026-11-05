import runpy
from pathlib import Path
import sys
folder=Path(__file__).resolve().parent
sys.path.insert(0,str(folder))
try:
    runpy.run_path(str(folder/'app.py'),run_name='__main__')
except Exception:
    import traceback
    (folder/'startup-error.log').write_text(traceback.format_exc())
    from tkinter import messagebox
    messagebox.showerror('PicoOS Designer',traceback.format_exc())

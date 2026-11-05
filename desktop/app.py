"""PicoOS Designer: local Windows editor, USB uploader and firmware controls."""
import copy
import json
import queue
import subprocess
import threading
import shutil
import sys
import os
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from tkinter.scrolledtext import ScrolledText
from pathlib import Path
from datetime import datetime
from PIL import Image, ImageDraw, ImageFont, ImageTk
from model import DEFAULT, validate, header
from device import Device

ROOT=Path(__file__).resolve().parent.parent
PY_SERIAL=Path(sys.executable)
POWERSHELL=Path(shutil.which('pwsh.exe') or shutil.which('powershell.exe') or 'powershell.exe')

class Designer(tk.Tk):
    def __init__(self):
        super().__init__(); self.title('M6 Reborn Designer'); self.geometry('1160x790')
        self.scene=copy.deepcopy(DEFAULT); self.selected=0; self.busy=False; self.events=queue.Queue()
        self.crest=Image.open(ROOT/'assets/cadillac-crest.png').convert('RGB')
        self.tabs=ttk.Notebook(self); self.tabs.pack(fill='both',expand=True,padx=12,pady=10)
        design=ttk.Frame(self.tabs,padding=12); self.tabs.add(design,text='Design watch face')
        bar=ttk.Frame(design); bar.pack(fill='x')
        for text,cmd in [('New',self.new),('Open JSON',self.open_design),('Save JSON',self.save_design),('Export PNG',self.export),('Send to watch',self.upload)]:
            ttk.Button(bar,text=text,command=cmd).pack(side='left',padx=3)
        ttk.Label(design,text='Working area: 120 × 80. The damaged right 40 pixels stay black. Drag objects to move them.').pack(anchor='w',pady=10)
        body=ttk.Frame(design); body.pack(fill='both',expand=True)
        left=ttk.Frame(body); left.pack(side='left',anchor='n')
        self.canvas=tk.Canvas(left,width=640,height=320,bg='black',highlightthickness=1,highlightbackground='#777'); self.canvas.pack()
        self.canvas.bind('<Button-1>',self.pick); self.canvas.bind('<B1-Motion>',self.drag)
        ttk.Label(left,text='Preview approximates text; the watch uses its built-in Montserrat font.').pack(anchor='w',pady=8)
        row=ttk.Frame(left); row.pack(fill='x')
        for kind in ('text','rect','crest'): ttk.Button(row,text='Add '+kind,command=lambda k=kind:self.add(k)).pack(side='left',padx=3)
        ttk.Button(row,text='Delete selected',command=self.delete).pack(side='left',padx=3)
        self.objects=tk.Listbox(left,height=7,exportselection=False); self.objects.pack(fill='x',pady=10)
        self.objects.bind('<<ListboxSelect>>',self.select)
        right=ttk.Frame(body,padding=(14,0)); right.pack(side='left',fill='both',expand=True)
        ttk.Label(right,text='Selected element properties (JSON)').pack(anchor='w')
        self.props=ScrolledText(right,height=18,width=35); self.props.pack(fill='both',expand=True,pady=6)
        ttk.Button(right,text='Apply properties',command=self.apply).pack(anchor='w')
        self.bg=tk.StringVar(value='000000'); bgrow=ttk.Frame(right); bgrow.pack(fill='x',pady=10)
        ttk.Label(bgrow,text='Background hex: ').pack(side='left'); ttk.Entry(bgrow,textvariable=self.bg,width=9).pack(side='left')
        ttk.Button(bgrow,text='Apply',command=self.background).pack(side='left',padx=4)
        firmware=ttk.Frame(self.tabs,padding=18); self.tabs.add(firmware,text='Program / firmware')
        ttk.Label(firmware,text='Live design uploads are temporary. To keep your design after power-off:\n1. Save startup design  →  2. Build firmware  →  3. Flash firmware',justify='left').pack(anchor='w',pady=10)
        for text,cmd in [('Save startup design',self.startup),('Use built-in animation at startup',self.builtin),('Build firmware',lambda:self.worker(self.build)),('Flash firmware',lambda:self.worker(self.flash))]:
            ttk.Button(firmware,text=text,command=cmd).pack(anchor='w',pady=5)
        ttk.Separator(firmware).pack(fill='x',pady=18)
        ttk.Label(firmware,text='Edit firmware source (a timestamped backup is saved before each edit):').pack(anchor='w')
        self.source_path=tk.StringVar(value='src/ui.c')
        choices=ttk.Combobox(firmware,textvariable=self.source_path,values=['src/ui.c','src/runtime.c','config/display_area.h','config/designer_scene.h'],state='readonly'); choices.pack(anchor='w',pady=6)
        ttk.Button(firmware,text='Open source editor',command=self.edit_source).pack(anchor='w')
        ttk.Label(firmware,text='Flash targets your identified Pico only. Keep the verified display wiring unchanged.\nIf automatic BOOTSEL fails, hold BOOTSEL while reconnecting USB, then click Flash again.').pack(anchor='w',pady=18)
        controls=ttk.Frame(self); controls.pack(fill='x',padx=14)
        for text,line in [('Animated crest','animate'),('Static crest','logo'),('Numbered grid','grid'),('Device status','status')]:
            ttk.Button(controls,text=text,command=lambda c=line:self.worker(lambda:self.send(c))).pack(side='left',padx=3)
        self.state=tk.StringVar(value='Ready — USB actions connect only when requested.')
        ttk.Label(self,textvariable=self.state).pack(anchor='w',padx=16,pady=6)
        self.log=ScrolledText(self,height=7,state='disabled'); self.log.pack(fill='x',padx=14,pady=(0,12))
        self.redraw(); self.after(100,self.poll)

    def report(self,text):
        self.log.configure(state='normal'); self.log.insert('end',str(text)+'\n'); self.log.see('end'); self.log.configure(state='disabled')
    def guard(self,fn):
        try: return fn()
        except Exception as exc: messagebox.showerror('PicoOS Designer',str(exc),parent=self)
    def worker(self,fn):
        if self.busy: self.report('Please wait for the current USB/build operation.'); return
        self.busy=True; self.state.set('Working…')
        def run():
            try: self.events.put(('done',fn() or 'Completed'))
            except Exception as exc: self.events.put(('error',str(exc)))
        threading.Thread(target=run,daemon=True).start()
    def poll(self):
        while not self.events.empty():
            kind,text=self.events.get(); self.report(text)
            if kind in ('done','error'): self.busy=False; self.state.set('Ready' if kind=='done' else 'Operation failed — see log')
        self.after(100,self.poll)
    def image(self):
        image=Image.new('RGB',(120,80),'#'+self.scene['background']); draw=ImageDraw.Draw(image)
        for e in self.scene['elements']:
            x,y=e['x'],e['y']
            if e['type']=='crest': image.paste(self.crest.resize((e['w'],(e['w']*45+56)//112),Image.Resampling.LANCZOS),(x,y))
            elif e['type']=='rect': draw.rectangle((x,y,x+e['w']-1,y+e['h']-1),fill='#'+e['color'])
            else:
                try: font=ImageFont.truetype(str(Path(os.environ.get('WINDIR','C:/Windows'))/'Fonts/segoeui.ttf'),e['font'])
                except OSError: font=ImageFont.load_default()
                draw.text((x,y),e['text'],font=font,fill='#'+e['color'],anchor='lt')
        return image
    def bounds(self,e):
        return (e.get('w',120-e['x']),e.get('h',(e.get('w',0)*45+56)//112 if e['type']=='crest' else e.get('font',12)+2))
    def redraw(self):
        self.canvas.delete('all'); self.photo=ImageTk.PhotoImage(self.image().resize((480,320),Image.Resampling.NEAREST))
        self.canvas.create_image(0,0,image=self.photo,anchor='nw')
        for x in range(480,640,12): self.canvas.create_line(x,0,min(x+160,640),320,fill='#242424')
        self.canvas.create_text(560,160,text='DAMAGED\nBLACK',fill='#999',justify='center')
        if 0<=self.selected<len(self.scene['elements']):
            e=self.scene['elements'][self.selected]; w,h=self.bounds(e)
            self.canvas.create_rectangle(e['x']*4,e['y']*4,(e['x']+w)*4,(e['y']+h)*4,outline='#36caff',dash=(3,3))
        self.objects.delete(0,'end')
        for n,e in enumerate(self.scene['elements']): self.objects.insert('end',f"{n+1}. {e['type']}  ({e['x']}, {e['y']})  {e.get('text','')}")
        self.props.delete('1.0','end')
        if 0<=self.selected<len(self.scene['elements']):
            self.objects.selection_set(self.selected); self.props.insert('1.0',json.dumps(self.scene['elements'][self.selected],indent=2))
        self.bg.set(self.scene['background'])
    def select(self,event):
        selection=self.objects.curselection()
        if selection: self.selected=selection[0]; self.redraw()
    def pick(self,event):
        x,y=event.x//4,event.y//4
        for i in reversed(range(len(self.scene['elements']))):
            e=self.scene['elements'][i]; w,h=self.bounds(e)
            if e['x']<=x<e['x']+w and e['y']<=y<e['y']+h:
                self.selected=i; self.offset=(x-e['x'],y-e['y']); self.redraw(); return
        self.offset=None
    def drag(self,event):
        if not getattr(self,'offset',None) or self.selected<0: return
        e=self.scene['elements'][self.selected]; w,h=self.bounds(e)
        max_x=119 if e['type']=='text' else 120-w; max_y=79 if e['type']=='text' else 80-h
        e['x']=max(0,min(max_x,event.x//4-self.offset[0])); e['y']=max(0,min(max_y,event.y//4-self.offset[1])); self.redraw()
    def add(self,kind):
        if len(self.scene['elements'])>=12: self.report('Maximum 12 elements.'); return
        e={'type':kind,'x':4,'y':4}
        e.update({'text':'Your text','font':12,'color':'FFFFFF'} if kind=='text' else {'w':40,'h':16,'color':'0066AA'} if kind=='rect' else {'w':80})
        self.scene['elements'].append(e); self.selected=len(self.scene['elements'])-1; self.redraw()
    def delete(self):
        if 0<=self.selected<len(self.scene['elements']): self.scene['elements'].pop(self.selected)
        self.selected=min(self.selected,len(self.scene['elements'])-1); self.redraw()
    def apply(self):
        def action():
            scene=copy.deepcopy(self.scene)
            if self.selected<0: return
            scene['elements'][self.selected]=json.loads(self.props.get('1.0','end')); self.scene=validate(scene); self.redraw()
        self.guard(action)
    def background(self):
        def action():
            scene=copy.deepcopy(self.scene); scene['background']=self.bg.get().lstrip('#'); self.scene=validate(scene); self.redraw()
        self.guard(action)
    def new(self): self.scene=copy.deepcopy(DEFAULT); self.selected=0; self.redraw()
    def open_design(self):
        name=filedialog.askopenfilename(filetypes=[('Watch design','*.json')])
        if name:
            def action():
                self.scene=validate(json.loads(Path(name).read_text())); self.selected=0; self.redraw()
            self.guard(action)
    def save_design(self):
        name=filedialog.asksaveasfilename(defaultextension='.json',filetypes=[('Watch design','*.json')])
        if name: self.guard(lambda:Path(name).write_text(json.dumps(validate(self.scene),indent=2)))
    def export(self):
        name=filedialog.asksaveasfilename(defaultextension='.png',filetypes=[('PNG image','*.png')])
        if name: self.guard(lambda:self.image().save(name))
    def upload(self):
        scene=copy.deepcopy(self.scene)
        def action():
            with Device() as device: return device.upload(scene)
        self.worker(action)
    def send(self,line):
        with Device() as device: return device.command(line)
    def startup(self):
        def action():
            self.backup(ROOT/'config/designer_scene.h'); (ROOT/'config/designer_scene.h').write_text(header(self.scene),encoding='ascii'); self.report('Startup design saved. Build and Flash to install it.')
        self.guard(action)
    def builtin(self):
        def action():
            self.backup(ROOT/'config/designer_scene.h'); (ROOT/'config/designer_scene.h').write_text('#pragma once\n#define PICOOS_BOOT_DESIGN 0\nstatic const ui_scene_t designer_boot_scene={0};\n'); self.report('Built-in animation selected. Build and Flash to install it.')
        self.guard(action)
    def backup(self,path):
        dest=ROOT/'backups'/datetime.now().strftime('%Y%m%d-%H%M%S-%f')/path.relative_to(ROOT); dest.parent.mkdir(parents=True,exist_ok=True); dest.write_bytes(path.read_bytes())
    def process(self,args):
        run=subprocess.run(args,cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,creationflags=subprocess.CREATE_NO_WINDOW)
        with (ROOT/'logs/designer-operations.log').open('a',encoding='utf8') as log:
            log.write('\n'+datetime.now().isoformat()+' '+str(args)+'\n'+run.stdout)
        self.events.put(('log',run.stdout))
        if run.returncode: raise RuntimeError('Command failed; see output above.')
    def build(self): self.process([str(POWERSHELL),'-NoProfile','-ExecutionPolicy','Bypass','-File',str(ROOT/'scripts/Build.ps1'),'-Target','OS']); return 'Build complete. Ready to flash.'
    def flash(self):
        # The existing Flash script independently checks board identity, wiring records and image hash.
        try: self.process([str(PY_SERIAL),str(ROOT/'scripts/Enter-Bootsel.py')])
        except RuntimeError: self.events.put(('log','Automatic BOOTSEL unavailable; checking for your Pico already in BOOTSEL.'))
        self.process([str(POWERSHELL),'-NoProfile','-ExecutionPolicy','Bypass','-File',str(ROOT/'scripts/Flash.ps1'),'-Mode','OS','-DisplayHardwareConfirmed'])
        return 'Firmware copied. Pico will restart.'
    def edit_source(self):
        path=ROOT/self.source_path.get(); window=tk.Toplevel(self); window.title(str(path)); window.geometry('950x700')
        editor=ScrolledText(window,undo=True,font=('Consolas',10)); editor.pack(fill='both',expand=True); editor.insert('1.0',path.read_text())
        def save():
            def action(): self.backup(path); path.write_text(editor.get('1.0','end-1c'),encoding='utf8'); self.report('Saved '+str(path))
            self.guard(action)
        ttk.Button(window,text='Save source with backup',command=save).pack(pady=6)

if __name__=='__main__': Designer().mainloop()

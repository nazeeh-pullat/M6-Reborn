"""Validated 120x80 watch designs and the firmware's transactional protocol."""
import json
import re

DEFAULT = {'background':'000000','elements':[
    {'type':'crest','x':4,'y':8,'w':112},
    {'type':'text','x':29,'y':65,'font':12,'color':'FFFFFF','text':'CADILLAC'}]}

def validate(scene):
    def number(value, lo, hi):
        if type(value) is not int or not lo <= value <= hi:
            raise ValueError(f'Expected integer {lo}..{hi}: {value}')
        return value
    def color(value):
        if not isinstance(value,str) or not re.fullmatch('[0-9a-fA-F]{6}',value):
            raise ValueError('Colors require six hex digits, for example FFFFFF')
        return value.upper()
    result={'background':color(scene['background']),'elements':[]}
    if not isinstance(scene['elements'],list) or len(scene['elements'])>12:
        raise ValueError('Maximum 12 elements')
    for raw in scene['elements']:
        e={'type':raw['type'],'x':number(raw['x'],0,119),'y':number(raw['y'],0,79)}
        if e['type']=='crest':
            e['w']=number(raw['w'],8,112); h=(e['w']*45+56)//112
            if e['x']+e['w']>120 or e['y']+h>80: raise ValueError('Crest exceeds working area')
        elif e['type']=='rect':
            e['w']=number(raw['w'],1,120-e['x']); e['h']=number(raw['h'],1,80-e['y'])
            e['color']=color(raw['color'])
        elif e['type']=='text':
            e['font']=raw['font']
            if e['font'] not in (10,12,20): raise ValueError('Font must be 10, 12 or 20')
            e['text']=raw['text']
            if not isinstance(e['text'],str) or not 1<=len(e['text'])<=48 or any(not 32<=ord(c)<=126 for c in e['text']):
                raise ValueError('Text must contain 1..48 printable ASCII characters')
            e['color']=color(raw['color'])
        else: raise ValueError('Unknown element type')
        result['elements'].append(e)
    return result

def commands(scene):
    scene=validate(scene)
    lines=[f"design begin {scene['background']} 0"]
    for e in scene['elements']:
        prefix=f"design {e['type']} {e['x']} {e['y']}"
        if e['type']=='crest': line=f"{prefix} {e['w']}"
        elif e['type']=='rect': line=f"{prefix} {e['w']} {e['h']} {e['color']}"
        else: line=f"{prefix} {e['font']} {e['color']} {e['text']}"
        lines.append(line)
    return lines+['design commit']

def header(scene):
    scene=validate(scene); rows=[]
    for e in scene['elements']:
        fields=[f'.type=UI_SCENE_{e["type"].upper()}',f'.x={e["x"]}',f'.y={e["y"]}']
        if e['type']=='text': fields.extend([f'.w={120-e["x"]}',f'.h={80-e["y"]}'])
        for key in ('w','h'):
            if key in e: fields.append(f'.{key}={e[key]}')
        if 'color' in e: fields.append(f'.color=0x{e["color"]}')
        if 'font' in e: fields.append(f'.font_size={e["font"]}')
        if 'text' in e: fields.append('.text='+json.dumps(e['text']))
        rows.append('    {'+','.join(fields)+'}')
    return '#pragma once\n#define PICOOS_BOOT_DESIGN 1\nstatic const ui_scene_t designer_boot_scene={\n'+f'.background=0x{scene["background"]},.count={len(rows)},.animate=false,\n.elements={{\n'+',\n'.join(rows)+'\n}};\n'

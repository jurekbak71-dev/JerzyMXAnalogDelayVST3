#!/usr/bin/env python3
"""Verify the shipped material library, independent layers and runtime bindings."""
from pathlib import Path
import hashlib,json,itertools,xml.etree.ElementTree as E
from PIL import Image
root=Path(__file__).resolve().parents[1];folder=root/'resource/gui3d'
m=json.loads((folder/'manifest.json').read_text());decoded=0
for a in m['assets']:
 p=folder/a['file'];assert hashlib.sha256(p.read_bytes()).hexdigest()==a['sha256'],p
 with Image.open(p) as im:
  assert im.format=='PNG' and im.mode=='RGBA',p
  assert im.size==(a['width'],a['height']),p
  assert a['height']==a['rows']*a['frame_height'] and a['width']==a['columns']*a['frame_width'],p
  assert max(im.size)<=4096,p
  im.load();decoded+=im.width*im.height*4
  if a['frames']>1:
   size=a['frame_height'];width=a['frame_width'];cols=a['columns'];frames=[im.crop(((i%cols)*width,(i//cols)*size,(i%cols+1)*width,(i//cols+1)*size)) for i in [0,a['frames']//2,a['frames']-1]]
   assert len({x.tobytes() for x in frames})==3,p
   assert all(x.getpixel((0,0))[3]==0 for x in frames),p
  if a['file'].startswith('knob_') and a['frames']>1:assert a['frames']>=101
print(f"{len(m['assets'])} RGBA assets, checksums and filmstrips passed; decoded library ceiling {decoded/1024/1024:.1f} MiB")
r=E.parse(root/'resource/mxdelay.uidesc').getroot()
for view in r.iter('view'):
 if view.attrib.get('class')=='CTextLabel':
  assert view.attrib.get('transparent')=='true' and view.attrib.get('frame-width','0')=='0'
for side in 'AB':
 assert any(v.attrib.get('control-tag')==side+'Algo' for v in r.iter('view'))
assert {v.attrib['gui-c-page'] for v in r.iter('view') if 'gui-c-page' in v.attrib}=={'0','1','2','3'}
source=(root/'source/jerzy_gui_assets.cpp').read_text()
for f in ['knob_192.png','knob_96.png','toggle.png','chassis.png','panel.png','inset.png']:
 assert f in source and (folder/f).exists(),f
# Simultaneously visible mouse targets on each page may not overlap each other.
# Different algorithm templates are mutually exclusive and evaluated independently.
def controls(node,ox=0.,oy=0.):
 for v in node:
  if v.tag!='view':continue
  x,y=map(float,v.attrib.get('origin','0, 0').split(','));w,h=map(float,v.attrib.get('size','0, 0').split(','))
  if v.attrib.get('control-tag') and v.attrib.get('class') not in ('CTextLabel','LedBarMeter','AnalogMeter') and v.attrib.get('mouse-enabled')!='false':yield(v.attrib['control-tag'],ox+x,oy+y,ox+x+w,oy+y+h)
  if 'gui-c-page' not in v.attrib:yield from controls(v,ox+x,oy+y)
for template in r.findall('template'):
 if template.attrib['name']=='view':continue
 items=list(controls(template))
 for a,b in itertools.combinations(items,2):
  overlap=min(a[3],b[3])-max(a[1],b[1])>0 and min(a[4],b[4])-max(a[2],b[2])>0
  assert not overlap,(template.attrib['name'],a,b)
print('Transparent dynamic text, four pages, runtime mapping and algorithm hit-target separation passed')

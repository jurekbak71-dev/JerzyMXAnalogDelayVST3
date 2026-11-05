from pathlib import Path
import itertools,xml.etree.ElementTree as E
root=Path(__file__).resolve().parents[1]
r=E.parse(root/'resource/mxdelay.uidesc').getroot()
page=next(v for v in r.iter('view') if v.attrib.get('gui-c-page')=='2')
required={'ReverbEnable','ReverbWet','ReverbDecay','ReverbDamping','ReverbWidth','ReverbPreDelay','ReverbFreeze','ReverbMode','ReverbIR','ReverbExternal','ReverbIRLength','ReverbNormalize','ReverbLowCut','ReverbDiffusion','ReverbEarly','ReverbHybrid','ReverbIRDamping'}
controls=[v for v in page.iter('view') if v.attrib.get('control-tag') in required and v.attrib.get('class')!='CTextLabel']
assert {v.attrib['control-tag'] for v in controls}==required
items=[]
for v in page.iter('view'):
 a=v.attrib
 if not a.get('control-tag') or a.get('class')=='CTextLabel':continue
 x,y=map(float,a['origin'].split(','));w,h=map(float,a['size'].split(','))
 assert 0<=x<x+w<=1280 and 0<=y<y+h<=534,a
 items.append((a['control-tag'],x,y,x+w,y+h))
for a,b in itertools.combinations(items,2):assert min(a[3],b[3])<=max(a[1],b[1]) or min(a[4],b[4])<=max(a[2],b[2]),(a,b)
assert any(v.attrib.get('control-tag')=='9300' and v.attrib.get('kick-style')=='true' for v in page.iter('view'))
assert all(v.attrib.get('mouse-enabled')=='false' for v in page.iter('view') if v.attrib.get('control-tag') in ('9301','9302'))
print('17 real reverb bindings, 3D controls, independent status, momentary load button, geometry and hit-target separation passed')

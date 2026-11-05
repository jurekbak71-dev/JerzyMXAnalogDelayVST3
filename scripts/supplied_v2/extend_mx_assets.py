import os
from pathlib import Path
BASE_DIR = Path(os.environ.get('MX_ASSETS_OUTPUT', str(Path(__file__).resolve().parent.parent)))
from pathlib import Path
import math, json, zipfile, io
import cairosvg
from PIL import Image,ImageDraw,ImageFont,ImageFilter
R=BASE_DIR/'MX_Delay_2_GUI_Assets'; S=R/'svg';P=R/'png_2x';F=R/'filmstrips';G=R/'glow';L=R/'layouts';D=R/'docs'
for p in (S,P,F,G,L,D):p.mkdir(exist_ok=True)
manifest=json.loads((R/'manifest.json').read_text()); assets=manifest['assets']
def add(name,w,h,body,category,notes='',folder=S):
 src=f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}"><defs><linearGradient id="metal" x1="0" x2=".9" y1="0" y2="1"><stop stop-color="#f3f0e7"/><stop offset=".18" stop-color="#6c6b6a"/><stop offset=".45" stop-color="#c7c7c2"/><stop offset=".7" stop-color="#55595a"/><stop offset="1" stop-color="#181d20"/></linearGradient><linearGradient id="shaft" x1="0" y1="0" x2="1" y2=".15"><stop stop-color="#2a2b2d"/><stop offset=".3" stop-color="#f1e9d8"/><stop offset=".55" stop-color="#94928d"/><stop offset=".8" stop-color="#333538"/><stop offset="1" stop-color="#d5d3c9"/></linearGradient><radialGradient id="light"><stop stop-color="#ffffff" stop-opacity=".9"/><stop offset=".25" stop-color="#ffffff" stop-opacity=".28"/><stop offset="1" stop-color="#ffffff" stop-opacity="0"/></radialGradient></defs>{body}</svg>'
 (folder/(name+'.svg')).write_text(src)
 png=P/(name+'.png') if folder==S else folder/(name+'.png')
 cairosvg.svg2png(bytestring=src.encode(),write_to=str(png),output_width=w*2,output_height=h*2)
 assets.append(dict(name=name,category=category,size=[w,h],notes=notes))
# physically hinged toggle, no rotation around a knob; multiple poses for 3D mechanical movement
for state,tip in [('off',(-10,17)),('middle',(0,-6)),('on',(10,-22))]:
 x,y=tip;cx=42;cy=53
 body=f'<ellipse cx="42" cy="62" rx="24" ry="10" fill="#000" opacity=".4"/><path d="M19 42L65 42 70 56 62 68 22 68 14 56Z" fill="url(#metal)" stroke="#101216" stroke-width="3"/><ellipse cx="42" cy="52" rx="19" ry="12" fill="#161a1e" stroke="#a3a19a" stroke-width="2"/><path d="M{cx} {cy} L{cx+x} {cy+y}" stroke="#0b0d0f" stroke-width="14" stroke-linecap="round"/><path d="M{cx} {cy} L{cx+x} {cy+y}" stroke="url(#shaft)" stroke-width="10" stroke-linecap="round"/><circle cx="{cx+x}" cy="{cy+y}" r="7" fill="url(#metal)" stroke="#242629" stroke-width="1.2"/><circle cx="42" cy="53" r="8" fill="#6c6d6b" stroke="#d2cfc5" stroke-width="1"/>'
 add('toggle_hebel_'+state,84,84,body,'toggle','Three mechanical lever poses; separate state PNG')
# 9-frame mechanical flip animation: linear shaft endpoints off->on, smooth stepping
frames=[]
for i in range(9):
 t=i/8;x=-10+20*t;y=17-39*t;cx=42;cy=53
 body=f'<ellipse cx="42" cy="62" rx="24" ry="10" fill="#000" opacity=".4"/><path d="M19 42L65 42 70 56 62 68 22 68 14 56Z" fill="url(#metal)" stroke="#101216" stroke-width="3"/><ellipse cx="42" cy="52" rx="19" ry="12" fill="#161a1e" stroke="#a3a19a" stroke-width="2"/><path d="M{cx} {cy} L{cx+x} {cy+y}" stroke="#0b0d0f" stroke-width="14" stroke-linecap="round"/><path d="M{cx} {cy} L{cx+x} {cy+y}" stroke="url(#shaft)" stroke-width="10" stroke-linecap="round"/><circle cx="{cx+x}" cy="{cy+y}" r="7" fill="url(#metal)" stroke="#242629" stroke-width="1.2"/><circle cx="42" cy="53" r="8" fill="#6c6d6b" stroke="#d2cfc5" stroke-width="1"/>'
 name=f'toggle_frame_{i:02d}';add(name,84,84,body,'toggle_frames')
 frames.append(Image.open(P/(name+'.png')).convert('RGBA').resize((84,84),Image.Resampling.LANCZOS))
strip=Image.new('RGBA',(84,84*9));
for i,im in enumerate(frames):strip.alpha_composite(im,(0,i*84))
strip.save(F/'toggle_hebel_84px_9frames.png')
# mechanical ZERO scale ring separate from knob (position 12 o'clock = 0)
for size in (64,96):
 c=size/2;rr=size*.46
 b=f'<circle cx="{c}" cy="{c}" r="{rr}" fill="none" stroke="#070a0c" stroke-width="3"/>'
 for j in range(11):
  ang=math.radians(-225+j*270/10);x1=c+rr*.86*math.cos(ang);y1=c+rr*.86*math.sin(ang);x2=c+rr*.97*math.cos(ang);y2=c+rr*.97*math.sin(ang)
  b+=f'<path d="M{x1:.2f} {y1:.2f}L{x2:.2f} {y2:.2f}" stroke="#c7c2b8" stroke-width="{1.8 if j==5 else .9}"/>'
 b+=f'<path d="M{c} {c-rr*.97}L{c} {c-rr*.75}" stroke="#fff4df" stroke-width="2.4"/>'
 add(f'knob_zero_scale_{size}',size,size,b,'knob_scales','Transparent zero tick ring; does not rotate')
# illuminated button skins separated from glyph text, no baked text
for color,hexcolor in [('amber','#ff9d34'),('cyan','#40d9ed'),('green','#49ed85'),('red','#ff574c'),('white','#e5e6e8')]:
 add('button_blank_'+color,76,46,f'<rect x="2" y="3" width="72" height="40" rx="8" fill="#070a0c" stroke="#5d6063" stroke-width="2"/><rect x="6" y="6" width="64" height="32" rx="6" fill="#161b1f" stroke="{hexcolor}" stroke-width="2.4"/><path d="M12 9H64" stroke="#fff" stroke-opacity=".24"/><path d="M12 35H64" stroke="{hexcolor}" stroke-width="2" opacity=".55"/>','buttons','No baked text: draw label dynamically with VSTGUI')
# transparent glow overlays, no background
for name,color in [('amber','#ff942b'),('cyan','#23ddec'),('green','#37f16d'),('red','#f44836'),('white','#ffffff')]:
 for kind in ('halo','line','under_knob'):
  if kind=='halo':w,h=160,100;body=f'<ellipse cx="80" cy="50" rx="65" ry="34" fill="{color}" opacity=".11"/><ellipse cx="80" cy="50" rx="48" ry="23" fill="{color}" opacity=".16"/><ellipse cx="80" cy="50" rx="24" ry="12" fill="{color}" opacity=".24"/>'
  elif kind=='line':w,h=180,40;body=f'<ellipse cx="90" cy="20" rx="87" ry="15" fill="{color}" opacity=".13"/><path d="M12 20H168" stroke="{color}" stroke-width="3" stroke-linecap="round"/><path d="M12 19H168" stroke="#fff" stroke-opacity=".5" stroke-width=".7"/>'
  else:w,h=112,112;body=f'<circle cx="56" cy="56" r="44" fill="none" stroke="{color}" stroke-width="11" opacity=".09"/><circle cx="56" cy="56" r="39" fill="none" stroke="{color}" stroke-width="6" opacity=".24"/><circle cx="56" cy="56" r="38" fill="none" stroke="{color}" stroke-width="2" opacity=".8"/>'
  add('glow_'+name+'_'+kind,w,h,body,'glow','RGBA transparent overlay, composite with additive or alpha blending',folder=G)
# large steel plates and panels sized for 1280x790 GUI, no text and no controls
for name,w,h in [('top_chassis_1280',1280,118),('dual_engine_chassis_1280',1280,560),('engine_a_plate',612,520),('engine_b_plate',612,520),('page_fx_plate',1240,550),('page_reverb_plate',1240,550),('page_routing_plate',1240,550),('footer_plate',1260,92)]:
 accent= '#c78035' if 'engine_a' in name else '#3eafc1' if 'engine_b' in name else '#878681'
 body=f'<rect x="2" y="2" width="{w-4}" height="{h-4}" rx="12" fill="#303338" stroke="#111315" stroke-width="3"/><rect x="6" y="6" width="{w-12}" height="{h-12}" rx="9" fill="#3a3c3d" stroke="{accent}" stroke-opacity=".5" stroke-width="2"/><path d="M15 16H{w-15}" stroke="#ddd5c8" stroke-opacity=".34" stroke-width="2"/><path d="M15 {h-15}H{w-15}" stroke="#000" stroke-opacity=".7" stroke-width="3"/>'
 for x in (17,w-17):
  for y in (17,h-17):body+=f'<circle cx="{x}" cy="{y}" r="5" fill="url(#metal)" stroke="#171819" stroke-width="1"/><path d="M{x-2} {y+2}L{x+2} {y-2}" stroke="#222" stroke-width="1.4"/>'
 add(name,w,h,body,'page_plates','Panel artwork; no labels or controls')
# logical 4 page maps: first A+B, other separate pages
pages={
 'page_01_ab':{'title':'A/B','sections':['Engine A / Tape Echo','Engine B / Memory Delay'],'engine_count':2,'shared':['INPUT','FX CHAIN','REVERB','OUTPUT']},
 'page_02_fx':{'title':'FX','sections':['FILTER','DRIVE','MOD','LO-FI'],'engine_count':0,'shared':['FX ROUTING','PRE','WET ONLY']},
 'page_03_reverb':{'title':'REVERB','sections':['FDN','IR','HYBRID','PRE-DELAY','DECAY'],'engine_count':0,'shared':['REVERB ENABLE','REVERB MIX']},
 'page_04_routing':{'title':'ROUTING','sections':['A','B','SERIAL','PARALLEL','SPLIT'],'engine_count':0,'shared':['INPUT','OUTPUT','GLOBAL MIX']}
}
for name,data in pages.items():
 (L/(name+'.json')).write_text(json.dumps(data,ensure_ascii=False,indent=2))
# install guidance, real drop-in image resources, not compiled vst
(R/'INSTALL_WINDOWS.ps1').write_text(r'''param([string]$ProjectRoot = ".")
$ErrorActionPreference = "Stop"
$dest = Join-Path $ProjectRoot "resource\mxdelay_3d_assets"
New-Item -ItemType Directory -Force -Path $dest | Out-Null
Copy-Item -Path (Join-Path $PSScriptRoot "png_2x") -Destination $dest -Recurse -Force
Copy-Item -Path (Join-Path $PSScriptRoot "filmstrips") -Destination $dest -Recurse -Force
Copy-Item -Path (Join-Path $PSScriptRoot "glow") -Destination $dest -Recurse -Force
Copy-Item -Path (Join-Path $PSScriptRoot "layouts") -Destination $dest -Recurse -Force
Write-Host "Assets installed into $dest. Map bitmap paths in mxdelay.uidesc and register them in CMake/resources."
''')
(R/'docs'/'VSTGUI_MAPPING.md').write_text('''# MX Delay 2 / VSTGUI asset mapping

## Strony (zachować pierwotny układ)
- 01 A/B: **oba silniki A i B na jednej planszy**. Użyć `dual_engine_chassis_1280`, `engine_a_plate`, `engine_b_plate` oraz pasków LED.
- 02 FX: osobna plansza `page_fx_plate`.
- 03 REVERB: osobna plansza `page_reverb_plate`.
- 04 ROUTING: osobna plansza `page_routing_plate`.
- Wspólny nagłówek i stopka są warstwami globalnymi. Nie duplikować w każdej stronie.

## Animacje
- `filmstrips/knob_96px_31frames.png`: **96 × 2976 px**, pionowo 31 klatek, 96 × 96 każda; zakres −135°..+135°.
- `filmstrips/knob_64px_31frames.png`: **64 × 1984 px**, pionowo 31 klatek, 64 × 64 każda.
- `filmstrips/toggle_hebel_84px_9frames.png`: **84 × 756 px**, 9 klatek; pionowy przełącznik hebelkowy. Nie jest obrotowy.
- `knob_zero_scale_*`: nieruchoma nakładka podziałki z kreską ZERO na godzinie 12; sam potencjometr ma jasny wskaźnik pozycji.
- Stany przycisków są oddzielnymi bitmapami; tekst rysowany w VSTGUI przez CTextLabel/CParamDisplay z `transparent=true`, `frame-width=0`.

## Światło
- `glow/*.png`: przezroczyste nakładki RGB/alpha dla amber, cyan, green, red, white. Używać niezależnie od tła i kontrolki. Uwaga: nakładki PNG są bitmapami alfa, nie fizycznym HDR/emission.

## Integracja
1. Uruchomić `powershell -ExecutionPolicy Bypass -File INSTALL_WINDOWS.ps1 -ProjectRoot C:\sciezka\do\repo`.
2. W `resource/mxdelay.uidesc` dodać wpisy `bitmap` z realnymi ścieżkami do plików PNG i powiązać je z odpowiednimi widokami.
3. Dla filmstripów użyć VSTGUI CAnimKnob / kontrolki pochodnej `CControl` z 31 klatkami i kontrolą `valueNormalized`.
4. Dla hebelków użyć 9-klatkowej animacji stanu z rzeczywistym parametrem ON/OFF; dwustanowe mapowanie 0/1, animacja pośrednia wyłącznie wizualna.
5. LED i wyświetlacze powinny otrzymywać dane z DSP / kontrolera, a nie z obrazka.
6. Dopisać zasoby do `CMakeLists.txt` i pakietu `.vst3`, zbudować Windows x64, uruchomić testy hitboxów, automatyzacji i renderowania.

**Stan:** gotowa biblioteka plików graficznych i skrypt kopiujący; **nie** jest to skompilowany plugin VST3 i nie zastępuje automatycznie istniejącego GUI. Komponenty są odrysowanymi grafikami 2D symulującymi 3D, a nie modelami 3D ani plikami Blender.
''')
manifest['version']='2.0';manifest['layout_pages']=pages;manifest['assets']=assets;manifest['animation']={'knob_large':{'path':'filmstrips/knob_96px_31frames.png','frame_width':96,'frame_height':96,'frames':31},'knob_small':{'path':'filmstrips/knob_64px_31frames.png','frame_width':64,'frame_height':64,'frames':31},'toggle':{'path':'filmstrips/toggle_hebel_84px_9frames.png','frame_width':84,'frame_height':84,'frames':9}};(R/'manifest.json').write_text(json.dumps(manifest,indent=2,ensure_ascii=False))
# quick validation
assert all(Image.open(p).mode=='RGBA' for p in [P/'toggle_hebel_on.png',G/'glow_amber_halo.png',F/'toggle_hebel_84px_9frames.png'])
for k,v in manifest['animation'].items():
 im=Image.open(R/v['path']);assert im.size==(v['frame_width'],v['frame_height']*v['frames']),k
zipfile_path=BASE_DIR/'MX_Delay_2_VSTGUI_3D_Library_v2.zip'
with zipfile.ZipFile(zipfile_path,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as z:
 for p in R.rglob('*'):
  if p.is_file():z.write(p,p.relative_to(R.parent))
print('TOTAL',len(assets),'ZIP',zipfile_path.stat().st_size,'PNG',len(list(P.glob('*.png'))),'GLOW',len(list(G.glob('*.png'))),'PAGES',len(list(L.glob('*.json'))))

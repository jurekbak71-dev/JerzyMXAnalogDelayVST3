import os
from pathlib import Path
BASE_DIR = Path(os.environ.get('MX_ASSETS_OUTPUT', str(Path(__file__).resolve().parent.parent)))
from pathlib import Path
import math, io, zipfile, json
from PIL import Image,ImageDraw,ImageFont
import cairosvg
root=BASE_DIR/'MX_Delay_2_GUI_Assets'; svgdir=root/'svg'; pngdir=root/'png_2x'; strips=root/'filmstrips'
assets=[]
def svg(name,w,h,body,category,notes=''):
    defs='''<defs><linearGradient id="steel" x1="0" y1="0" x2=".92" y2="1"><stop stop-color="#77736e"/><stop offset=".11" stop-color="#4c4a47"/><stop offset=".48" stop-color="#343638"/><stop offset=".8" stop-color="#565452"/><stop offset="1" stop-color="#252627"/></linearGradient><linearGradient id="brushed" x1="0" y1="0" x2="0" y2="1"><stop stop-color="#9b9690"/><stop offset=".06" stop-color="#676562"/><stop offset=".35" stop-color="#454646"/><stop offset=".78" stop-color="#3a3c3d"/><stop offset="1" stop-color="#74716c"/></linearGradient><linearGradient id="dark" x1="0" y1="0" x2="0" y2="1"><stop stop-color="#30343a"/><stop offset=".3" stop-color="#161b20"/><stop offset="1" stop-color="#090c10"/></linearGradient><linearGradient id="black" x1="0" y1="0" x2="0" y2="1"><stop stop-color="#101619"/><stop offset="1" stop-color="#010405"/></linearGradient><linearGradient id="silver" x1="0" y1="0" x2=".9" y2="1"><stop stop-color="#ffffff"/><stop offset=".17" stop-color="#aaa7a0"/><stop offset=".38" stop-color="#696a68"/><stop offset=".57" stop-color="#c9c7bf"/><stop offset=".79" stop-color="#6d6e6d"/><stop offset="1" stop-color="#2b2d2e"/></linearGradient><linearGradient id="amber" x1="0" y1="0" x2="0" y2="1"><stop stop-color="#fbb15a"/><stop offset=".48" stop-color="#b85d0c"/><stop offset="1" stop-color="#4d2308"/></linearGradient><linearGradient id="cyan" x1="0" y1="0" x2="0" y2="1"><stop stop-color="#8efbff"/><stop offset=".5" stop-color="#1a9caf"/><stop offset="1" stop-color="#09444d"/></linearGradient><radialGradient id="glow"><stop stop-color="#f6f8d7" stop-opacity=".85"/><stop offset=".2" stop-color="#f2b857" stop-opacity=".48"/><stop offset="1" stop-color="#f3a63b" stop-opacity="0"/></radialGradient><radialGradient id="greenGlow"><stop stop-color="#d9ffe7" stop-opacity=".8"/><stop offset=".28" stop-color="#42ec91" stop-opacity=".48"/><stop offset="1" stop-color="#42ec91" stop-opacity="0"/></radialGradient><filter id="shadow" x="-50%" y="-50%" width="200%" height="200%"><feGaussianBlur stdDeviation="2"/></filter><pattern id="grain" width="4" height="4" patternUnits="userSpaceOnUse"><path d="M0 0h4 M0 2h4" stroke="#fff" stroke-opacity=".035" stroke-width=".45"/></pattern></defs>'''
    text=f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}">{defs}{body}</svg>'
    (svgdir/(name+'.svg')).write_text(text,encoding='utf-8')
    cairosvg.svg2png(bytestring=text.encode(),write_to=str(pngdir/(name+'.png')),output_width=w*2,output_height=h*2)
    assets.append(dict(name=name,category=category,size=[w,h],notes=notes))
def screw(cx,cy,r=5):return f'<circle cx="{cx}" cy="{cy}" r="{r}" fill="url(#silver)" stroke="#0a0a0a" stroke-width="1"/><path d="M{cx-r*.46} {cy+r*.46}L{cx+r*.46} {cy-r*.46}" stroke="#343536" stroke-width="1.6"/>'
def panel(w,h,fill='brushed',r=8,screws=True):
 b=f'<rect x="1.5" y="1.5" width="{w-3}" height="{h-3}" rx="{r}" fill="url(#{fill})" stroke="#0b0d10" stroke-width="3"/><rect x="4" y="4" width="{w-8}" height="{h-8}" rx="{max(1,r-2)}" fill="none" stroke="#b2aaa0" stroke-opacity=".5" stroke-width="1"/><rect x="6" y="6" width="{w-12}" height="{h-12}" rx="{max(1,r-3)}" fill="url(#grain)"/>'
 if screws:
  for x in [13,w-13]:
   for y in [13,h-13]:b+=screw(x,y,4.5)
 return b
svg('01_main_chassis',769,423,panel(769,423,'brushed',11),'chassis','Main frame, independently scalable in target UI')
svg('02_header_plate',769,67,panel(769,67,'dark',8),'plates')
svg('03_engine_inset',318,225,panel(318,225,'dark',10)+f'<rect x="12" y="11" width="294" height="201" rx="8" fill="#080c10" stroke="#76706b" stroke-opacity=".7"/>','plates')
svg('04_lower_plate',635,100,panel(635,100,'brushed',8),'plates')
svg('05_small_plate',128,100,panel(128,100,'brushed',8),'plates')
svg('06_black_display',278,36,'<rect x="1" y="1" width="276" height="34" rx="5" fill="url(#black)" stroke="#77736a" stroke-width="1"/><path d="M8 4H270" stroke="#ffffff" stroke-opacity=".13"/>','displays')
svg('07_orange_engine_header',310,34,'<rect x="1" y="1" width="308" height="32" rx="7" fill="url(#black)" stroke="#a96427" stroke-width="1.5"/><rect x="5" y="4" width="33" height="26" rx="5" fill="url(#amber)"/><text x="21.5" y="24" font-family="Arial" font-size="21" font-weight="bold" fill="#fff" text-anchor="middle">A</text><path d="M46 29H300" stroke="#e28c39" stroke-opacity=".6"/>','headers')
svg('08_cyan_engine_header',310,34,'<rect x="1" y="1" width="308" height="32" rx="7" fill="url(#black)" stroke="#258c9b" stroke-width="1.5"/><rect x="5" y="4" width="33" height="26" rx="5" fill="url(#cyan)"/><text x="21.5" y="24" font-family="Arial" font-size="21" font-weight="bold" fill="#fff" text-anchor="middle">B</text><path d="M46 29H300" stroke="#46cad9" stroke-opacity=".6"/>','headers')
# knobs: center 48,48 for big and 32,32 for small, dial line rendered as distinct overlay
for name,size in [('09_knob_large_base',96),('10_knob_small_base',64),('11_knob_trim_base',78)]:
 c=size/2;r=size*.375
 b=f'<circle cx="{c}" cy="{c+1}" r="{r+6}" fill="#000" opacity=".45"/><circle cx="{c}" cy="{c}" r="{r+4}" fill="url(#dark)" stroke="#a5a29c" stroke-width="2"/><circle cx="{c}" cy="{c}" r="{r}" fill="url(#silver)" stroke="#16181b" stroke-width="1.5"/><circle cx="{c}" cy="{c}" r="{r*.77}" fill="none" stroke="#fff" stroke-opacity=".2"/><path d="M{c-r*.78} {c-r*.18}L{c+r*.5} {c+r*.43}" stroke="#ffffff" stroke-opacity=".12" stroke-width="2"/>'
 svg(name,size,size,b,'knobs','Static base; rotate separate pointer or use supplied filmstrip')
for name,size in [('12_knob_pointer_large',96),('13_knob_pointer_small',64)]:
 c=size/2;r=size*.34
 svg(name,size,size,f'<path d="M{c} {c-r}L{c} {c-r*.45}" stroke="#1c1d1d" stroke-width="4" stroke-linecap="round"/><path d="M{c} {c-r}L{c} {c-r*.45}" stroke="#f4f1e9" stroke-width="2" stroke-linecap="round"/>','knobs','Rotatable overlay; pivot at image center')
svg('14_switch_off',46,46,'<circle cx="23" cy="23" r="20" fill="url(#dark)" stroke="#9e9a93" stroke-width="2"/><circle cx="23" cy="23" r="12" fill="url(#silver)"/><path d="M23 24V10" stroke="#e3ded4" stroke-width="6" stroke-linecap="round"/>','switches')
svg('15_switch_on',46,46,'<circle cx="23" cy="23" r="20" fill="url(#dark)" stroke="#9e9a93" stroke-width="2"/><circle cx="23" cy="23" r="12" fill="url(#silver)"/><path d="M23 24L33 13" stroke="#e3ded4" stroke-width="6" stroke-linecap="round"/>','switches')
svg('16_button_off',60,55,'<rect x="3" y="4" width="54" height="49" rx="8" fill="url(#dark)" stroke="#9a9287" stroke-width="2"/><rect x="8" y="8" width="44" height="36" rx="5" fill="#15191d" stroke="#56565a"/>','buttons')
svg('17_button_on_amber',60,55,'<rect x="3" y="4" width="54" height="49" rx="8" fill="url(#dark)" stroke="#c59052" stroke-width="2"/><rect x="8" y="8" width="44" height="36" rx="5" fill="url(#amber)" stroke="#f5b36c"/>','buttons')
svg('18_button_on_cyan',60,55,'<rect x="3" y="4" width="54" height="49" rx="8" fill="url(#dark)" stroke="#72c9cf" stroke-width="2"/><rect x="8" y="8" width="44" height="36" rx="5" fill="url(#cyan)" stroke="#a0f6ff"/>','buttons')
for name,col,glow in [('19_led_off','#1d292b',False),('20_led_green','#43f28d',True),('21_led_amber','#ffbb54',True),('22_led_red','#ff5b45',True),('23_led_cyan','#61e9f7',True)]:
 b='<circle cx="16" cy="16" r="9" fill="#050809" stroke="#787978" stroke-width="1.4"/>'
 if glow:b+=f'<circle cx="16" cy="16" r="15" fill="{col}" opacity=".13"/>'
 b+=f'<circle cx="16" cy="16" r="5" fill="{col}"/><circle cx="14.5" cy="14.5" r="1.5" fill="#fff" opacity=".7"/>'
 svg(name,32,32,b,'leds')
for name,on in [('24_meter_segment_off',False),('25_meter_segment_green',True),('26_meter_segment_amber',True),('27_meter_segment_red',True)]:
 col=('#16251e' if not on else {'25_meter_segment_green':'#41eb79','26_meter_segment_amber':'#ffbd41','27_meter_segment_red':'#fb5643'}.get(name,'#18241f'))
 svg(name,9,18,f'<rect x="1" y="1" width="7" height="16" rx="1" fill="{col}" stroke="#070c0d" stroke-width=".8"/>','meters')
svg('28_meter_housing_vertical',32,198,'<rect x="1" y="1" width="30" height="196" rx="8" fill="url(#dark)" stroke="#a39a8f" stroke-width="1.5"/><rect x="7" y="8" width="18" height="182" rx="4" fill="#050a0c" stroke="#404849"/>','meters')
svg('29_meter_housing_horizontal',168,26,'<rect x="1" y="1" width="166" height="24" rx="5" fill="url(#dark)" stroke="#888178"/><rect x="6" y="6" width="156" height="14" rx="2" fill="#050a0c"/>','meters')
svg('30_separator',10,210,'<rect x="2" y="2" width="6" height="206" rx="3" fill="url(#silver)" stroke="#0b0b0b"/>','hardware')
svg('31_screw',22,22,screw(11,11,8),'hardware')
svg('32_reverb_image_frame',196,77,'<rect x="1" y="1" width="194" height="75" rx="7" fill="url(#dark)" stroke="#9a9288" stroke-width="2"/><rect x="8" y="8" width="180" height="59" rx="4" fill="#151012" stroke="#a57a4e" stroke-opacity=".6"/>','reverb','Frame only; photograph/IR visualization is a separate replaceable layer')
svg('33_select_field',150,31,'<rect x="1" y="1" width="148" height="29" rx="6" fill="url(#black)" stroke="#807b76"/><path d="M133 13l5 5 5-5" fill="none" stroke="#d9d8d4" stroke-width="2"/>','fields')
# symbolic vector icons
icons={'filter':'M10 27Q19 8 28 27T46 27','drive':'M27 9V30 M18 14A17 17 0 1 0 36 14','mod':'M7 27L15 27 20 12 29 41 36 19 41 27 48 27','lofi':'M10 17h7v20h-7z M23 11h8v32h-8z M37 17h7v20h-7z','settings':'M27 11V16 M27 38V43 M11 27H16 M38 27H43 M16 16L20 20 M34 34L38 38 M38 16L34 20 M20 34L16 38','link':'M14 27H40 M21 20L14 27 21 34 M33 20L40 27 33 34'}
for n,path in icons.items():
 svg('icon_'+n,54,54,f'<path d="{path}" stroke="#edaa53" stroke-width="3" stroke-linejoin="round" stroke-linecap="round" fill="none"/>','icons')
# Filmstrip: generate from composited base+pointer transformed at different angles; 31 frames 96x96
for size,base,pointer in [(96,'09_knob_large_base','12_knob_pointer_large'),(64,'10_knob_small_base','13_knob_pointer_small')]:
 base_img=Image.open(pngdir/(base+'.png')).convert('RGBA').resize((size,size),Image.Resampling.LANCZOS)
 pointer_img=Image.open(pngdir/(pointer+'.png')).convert('RGBA').resize((size,size),Image.Resampling.LANCZOS)
 strip=Image.new('RGBA',(size,size*31),(0,0,0,0))
 for frame in range(31):
  # rotation range -135 to +135 degrees, pointing up at center
  angle=-135+frame*270/30
  p=pointer_img.rotate(-angle,Image.Resampling.BICUBIC,expand=False)
  image=base_img.copy();image.alpha_composite(p);strip.alpha_composite(image,(0,frame*size))
 strip.save(strips/f'knob_{size}px_31frames.png')
# Atlas preview, whiteboard without relying on text inside assets
font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',15)
thumb=Image.new('RGB',(1400,((len(assets)+5)//6)*205+105),(26,29,34));d=ImageDraw.Draw(thumb)
d.text((25,20),'MX DELAY 2   /   COMPONENT ASSET LIBRARY',font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',24),fill=(238,230,214))
d.text((25,57),'Independent SVG masters  |  transparent PNG @2x  |  knob animation filmstrips',font=font,fill=(177,187,195))
for i,a in enumerate(assets):
 col=i%6;row=i//6;x=20+col*230;y=105+row*205
 d.rounded_rectangle((x,y,x+213,y+180),radius=8,fill=(39,43,47),outline=(83,85,86),width=1)
 im=Image.open(pngdir/(a['name']+'.png')).convert('RGBA');im.thumbnail((193,125))
 thumb.paste(im,(x+(213-im.width)//2,y+8+(125-im.height)//2),im)
 label=a['name'].replace('_',' ');d.text((x+10,y+140),label[:25],font=font,fill=(232,228,219))
 d.text((x+10,y+160),a['category']+'  '+str(a['size'][0])+'x'+str(a['size'][1]),font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',11),fill=(155,168,173))
thumb.save(str(BASE_DIR/'MX_Delay_2_components_preview.png'))
(root/'manifest.json').write_text(json.dumps({'reference':'image(20261105-102645).png','reference_size':[769,423],'type':'independent redrawn assets, not extracted editable layers','assets':assets},indent=2),encoding='utf8')
(root/'docs'/'README_PL.md').write_text('''# MX Delay 2 — biblioteka elementów GUI (etap 1)

Źródło wizualne: makieta użytkownika 769 × 423 px. Elementy są **odrysowanymi, osobnymi zasobami** na podstawie makiety, a nie odzyskanymi oryginalnymi warstwami 3D/PSD. Zawartość: indywidualne SVG, PNG przezroczyste @2x, dwie animacje potencjometrów (31 klatek, pionowe filmstripy), manifest z wymiarami.

## Warstwy
1. Obudowa i blachy: 01–05. 2. Wyświetlacze i nagłówki: 06–08. 3. Pokrętła i osobne wskazówki: 09–13. 4. Przełączniki/przyciski: 14–18. 5. Diody: 19–23. 6. Wskaźniki poziomu: 24–29. 7. Elementy mechaniczne: 30–31. 8. Reverb i pola: 32–33. 9. Ikony: icon_*.

## Integracja z VSTGUI
Zachować logikę parametrów/DSP, wymienić wyłącznie rysowanie widoków. Tła blach jako nieruchome CBitmap, kontrolki jako CControl z osobnymi bitmapami stanów i animacją potencjometrów. Wskazówki można obracać o −135°..+135° lub użyć filmstripów. Nie przeskalowywać pokręteł nieproporcjonalnie. Śruby nie powinny znajdować się w obszarach aktywnych. Mierniki i LED to widoki dynamiczne, nie statyczne obrazki. UI @100% wymaga dopracowania rozkładu i testu w hostcie.

## Ograniczenia
Wektory odwzorowują język stylistyczny, ale metaliczność jest uproszczona względem fotorealistycznej makiety. To biblioteka do kolejnego etapu wdrożenia, nie gotowe GUI VST3 ani potwierdzony zrzut działającego pluginu. Przed wdrożeniem potrzebne są weryfikacja wyglądu i mapowanie kontrolek.
''',encoding='utf8')
zip_path=BASE_DIR/'MX_Delay_2_GUI_Assets.zip'
with zipfile.ZipFile(zip_path,'w',zipfile.ZIP_DEFLATED) as z:
 for p in root.rglob('*'):
  if p.is_file():z.write(p,p.relative_to(root.parent))
print('ASSETS',len(assets),'ZIP',zip_path,zip_path.stat().st_size,'PREVIEW',thumb.size)

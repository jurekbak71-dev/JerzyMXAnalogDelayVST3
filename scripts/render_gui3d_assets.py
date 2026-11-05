#!/usr/bin/env python3
"""Deterministic orthographic material renderer. No downloaded textures or fonts.
Python 3 + numpy 2.2.6 + Pillow 11.3.0. Geometry is in physical layer coordinates;
static scales, shadows, reflections and emission remain separate from moving caps.
"""
from pathlib import Path
import hashlib,json,math
import numpy as np
from PIL import Image,ImageDraw,ImageFilter
OUT=Path(__file__).resolve().parents[1]/'resource/gui3d'
OUT.mkdir(parents=True,exist_ok=True)
assets=[]
def save(name,im,frames=1,usage='',mapping=''):
    path=OUT/(name+'.png');im.save(path,optimize=False,compress_level=9)
    assets.append(dict(file=path.name,width=im.width,height=im.height,frames=frames,frame_width=im.width,frame_height=im.height//frames,sha256=hashlib.sha256(path.read_bytes()).hexdigest(),usage=usage,binding=mapping))
def radial(size):
    y,x=np.mgrid[:size,:size].astype(float);x=(x+.5-size/2)/(size/2);y=(y+.5-size/2)/(size/2)
    return x,y,np.sqrt(x*x+y*y),np.arctan2(y,x)
def rgba(rgb,alpha):return Image.fromarray(np.dstack([np.clip(rgb,0,255).astype('uint8'),np.clip(alpha,0,255).astype('uint8')]),'RGBA')
def cap(size,n):
    x,y,r,a=radial(size);mask=np.clip((.82-r)*size,0,1)
    # Bevel normals and anisotropic circular turning marks, fixed studio light.
    nz=np.sqrt(np.clip(1-(r/.86)**12,0,1));nx=x*np.clip((r-.62)*5,0,1);ny=y*np.clip((r-.62)*5,0,1)
    light=np.clip(-.48*nx-.62*ny+.62*nz,0,1)
    brushed=5*np.sin(r*size*8)+3*np.sin(a*127+r*3)
    anis=46*np.cos(a-.65)+24*np.cos(2*a+1.4)+9*np.cos(5*a)
    shade=110+65*light+anis+brushed
    # Distinct recessed rubber grip, machined outer rim and polished inner bevel.
    grip=24+9*np.cos(a*72)+8*light
    rim=(r>.73)&(r<.82);polish=(r>.67)&(r<.71)
    shade=np.where(rim,grip,shade);shade=np.where(polish,180+65*np.cos(a+1),shade)
    im=rgba(np.stack([shade,shade*.985,shade*.96],axis=-1),mask*255)
    d=ImageDraw.Draw(im)
    angle=math.radians(135+270*n);cx=cy=size/2
    p=lambda rr:(cx+math.cos(angle)*size*rr,cy+math.sin(angle)*size*rr)
    d.line([p(.12),p(.31)],fill=(15,18,20,255),width=max(3,size//38))
    d.line([p(.13),p(.30)],fill=(245,242,224,255),width=max(1,size//96))
    return im
for size in [96,144,192]:
    strip=Image.new('RGBA',(size,size*101))
    for i in range(101):strip.paste(cap(size,i/100),(0,i*size))
    save('knob_'+str(size),strip,101,'101-frame turned aluminium cap; moving pointer','ChickenKnob: normalized [0,1], 135..405 degrees')
# Independent soft-contact shadow and cover-glass highlights.
x,y,r,a=radial(192)
shadow=rgba(np.zeros((192,192,3)),180*np.exp(-((np.sqrt(x*x+(y-.08)**2)-.73)/.12)**2))
save('knob_shadow',shadow,usage='Contact shadow, beneath moving cap')
for name,color in [('amber',(255,166,45)),('cyan',(65,225,240)),('green',(65,240,98)),('red',(255,62,40)),('white',(235,245,255))]:
    glow=np.exp(-((r-.80)/.09)**2)*180
    save('glow_'+name,rgba(np.broadcast_to(color,(192,192,3)),glow),usage='Independent emission overlay; runtime alpha')
    im=Image.new('RGBA',(192,192));d=ImageDraw.Draw(im)
    for i in range(21):
        t=math.radians(135+i*13.5);lo=.425 if i%5==0 else .44;hi=.48
        d.line([(96+192*lo*math.cos(t),96+192*lo*math.sin(t)),(96+192*hi*math.cos(t),96+192*hi*math.sin(t))],fill=(*color,215 if i%5==0 else 90),width=2 if i%5==0 else 1)
    save('scale_'+name,im,usage='Fixed control scale; does not rotate')
# Radius plate normals give screws and washer a physical recess.
x,y,r,a=radial(64);s=80+65*np.cos(a+.6)+55*np.exp(-((r-.67)/.07)**2)
im=rgba(np.stack([s,s,s*.98],-1),np.clip((.9-r)*64,0,1)*255)
d=ImageDraw.Draw(im);d.line([(17,42),(44,21)],fill=(6,8,9,255),width=5);d.line([(17,40),(43,20)],fill=(190,190,180,255),width=1)
save('screw',im,usage='Fixed mounting hardware')
# Brushed metal panel from deterministic multiscale milling texture.
def plate(w,h,dark=False):
    y,x=np.mgrid[:h,:w].astype(float)
    grain=2*np.sin(y*8.31)+1.3*np.sin(y*3.173+x*.009)+.65*np.sin(x*1.3+y*33.7)
    edge=np.minimum.reduce([x,y,w-1-x,h-1-y])
    shade=(34 if dark else 112)+grain+(8 if dark else 36)*np.exp(-y/h*2)+4*np.cos(x/w*math.pi)
    shade+=np.where(edge<2,37,np.where(edge<5,-32,0))
    return rgba(np.stack([shade*1.025,shade*1.01,shade*.98],-1),np.full_like(x,255))
save('chassis',plate(1280,790),usage='Brushed steel chassis only, no controls or text')
save('panel',plate(768,384),usage='Independent satin aluminium module plate')
save('inset',plate(768,384,True),usage='Recessed dark enamel engine face')
# Glass is a translucent highlight layer, never contains a number or meter.
y,x=np.mgrid[:128,:512].astype(float);al=32*np.exp(-((y-12-x*.07)/13)**2)+10*np.exp(-((y-80)/40)**2)
save('glass',rgba(np.broadcast_to((220,237,246),(128,512,3)),al),usage='Display glass reflection overlay')
# Mechanically translated lever, shadow and specular moving with the lever.
strip=Image.new('RGBA',(128,128*17))
for i in range(17):
    im=Image.new('RGBA',(128,128));d=ImageDraw.Draw(im)
    d.ellipse((31,34,97,100),fill=(10,12,13,255),outline=(187,188,177,255),width=3)
    d.ellipse((39,42,89,92),fill=(47,49,48,255),outline=(107,107,99,255),width=3)
    end=95-i*3.6
    d.line([(69,75),(70,end+6)],fill=(0,0,0,140),width=18)
    d.line([(64,67),(64,end)],fill=(101,104,102,255),width=17)
    d.line([(61,65),(61,end)],fill=(218,221,212,255),width=6)
    d.ellipse((52,end-8,76,end+8),fill=(182,187,182,255),outline=(247,246,230,255),width=2)
    strip.paste(im,(0,i*128))
save('toggle',strip,17,'Metal toggle with moving contact shadow','ToggleSwitch and ThreeWaySwitch: endpoint/centre frames')
# Unlabelled bevelled keys; text remains a live VSTGUI control.
for state,col in [('off',(40,44,46)),('amber',(181,109,31)),('cyan',(28,125,139))]:
    im=Image.new('RGBA',(256,96));d=ImageDraw.Draw(im)
    d.rounded_rectangle((2,2,253,93),radius=10,fill=(7,9,10,255),outline=(159,158,149,255),width=2)
    for j in range(8,88):
        factor=.65+.5*(1-(j-8)/80);c=tuple(int(min(255,v*factor)) for v in col)
        d.line([(9,j),(247,j)],fill=(*c,255))
    d.line([(12,9),(244,9)],fill=(245,236,211,130),width=2)
    save('button_'+state,im,usage='Unlabelled key; active emission selected at runtime')
# Separate line icons, transparent backgrounds.
for name in ['filter','drive','mod','lofi','routing','reverb']:
    im=Image.new('RGBA',(96,64));d=ImageDraw.Draw(im)
    if name in ('filter','drive','mod'):
        pts=[(x,32+int(14*(math.sin((x-12)/10) if name=='mod' else math.tanh((x-48)/8) if name=='drive' else -math.atan((x-48)/9)))) for x in range(12,84)]
        d.line(pts,fill=(244,178,75,255),width=3)
    elif name=='lofi':d.line([(12,45),(28,45),(28,24),(44,24),(44,40),(60,40),(60,17),(84,17)],fill=(244,178,75,255),width=3)
    else:
        for i in range(4):d.line([(18+i*15,15),(18+i*15,49-i*6)],fill=(244,178,75,255),width=3)
    save('icon_'+name,im,usage='Independent module icon, not a simulated audio display')
manifest={'generator':'scripts/render_gui3d_assets.py','render':'Deterministic analytic geometry and anisotropic material shading; 2D orthographic assets, no recovered original 3D scene','assets':assets}
(OUT/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(f'{len(assets)} assets; {sum((OUT/a["file"]).stat().st_size for a in assets)/1024/1024:.2f} MiB PNG')

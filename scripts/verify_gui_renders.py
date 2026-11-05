#!/usr/bin/env python3
"""Validate authentic native-host captures, including live moving controls."""
from pathlib import Path
import sys
from PIL import Image,ImageChops,ImageStat
root=Path(sys.argv[1]);files={p.name:p for p in root.rglob('mxdelay2-render-*.png')}
required=[f'mxdelay2-render-tab-{i}.png' for i in range(4)]+[f'mxdelay2-render-state-{i}.png' for i in (0,50,100)]
for name in required:assert name in files,name
images=[Image.open(files[name]).convert('RGB') for name in required]
assert len({im.tobytes() for im in images[:4]})==4,'Page rendering did not change'
for im in images:
 assert min(im.size)>500
 assert max(ImageStat.Stat(im).stddev)>15,'Blank or flat screenshot'
def crop(im,rect):
 w,h=im.size
 return im.crop(tuple(round(v*(w/1280 if i%2==0 else h/790)) for i,v in enumerate(rect)))
a,b,c=(crop(im,(267,186,351,270)) for im in images[4:])
assert ImageChops.difference(a,b).getbbox() and ImageChops.difference(b,c).getbbox(),'Parameter automation did not move knob pixels'
# B's actual enable is on its own deck; compare the same area across states.
x=crop(images[4],(1092,122,1140,170));y=crop(images[6],(1092,122,1140,170))
assert ImageChops.difference(x,y).getbbox(),'Mechanical ON/OFF rendering did not change'
print('Four authentic pages + three knob states + mechanical ON/OFF pixel changes verified')

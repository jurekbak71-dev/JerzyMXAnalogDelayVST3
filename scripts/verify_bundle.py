#!/usr/bin/env python3
"""Validate the delivered VST3 bundle, not just the source resource directory."""
from pathlib import Path
import hashlib,json,sys
root=Path(__file__).resolve().parents[1]
bundle=Path(sys.argv[1]);res=bundle/'Contents/Resources'
assert (res/'mxdelay.uidesc').is_file(),'Missing UI description'
assert (res/'mxdelay.uidesc').read_bytes()==(root/'resource/mxdelay.uidesc').read_bytes(),'Stale UI description'
m=json.loads((root/'resource/gui3d/manifest.json').read_text())
for a in m['assets']:
 f=res/'gui3d'/a['file'];assert f.is_file(),f
 assert hashlib.sha256(f.read_bytes()).hexdigest()==a['sha256'],f
modules=list(bundle.glob('Contents/x86_64-win/*.vst3'))
assert len(modules)==1,modules
b=modules[0].read_bytes();assert b[:2]==b'MZ'
o=int.from_bytes(b[60:64],'little');assert b[o:o+4]==b'PE\0\0' and int.from_bytes(b[o+4:o+6],'little')==0x8664
print(f'Windows x64 VST3 + {len(m["assets"])} verified runtime PNG resources')

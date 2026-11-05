#!/usr/bin/env python3
"""Verify the actual install ZIP after compression, including its x64 bundle."""
from pathlib import Path, PurePosixPath
import subprocess,sys,tempfile,zipfile
archive=Path(sys.argv[1]);root=Path(__file__).resolve().parents[1]
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None, 'ZIP CRC failure'
    for name in z.namelist():
        p=PurePosixPath(name)
        assert not p.is_absolute() and '..' not in p.parts and '\\' not in name,name
    required=['MX Delay 2.vst3/Contents/Resources/mxdelay.uidesc','INSTALL_FL_STUDIO.md','REVERB_VALIDATION.md','CTEST_RESULTS.txt','COMMIT.txt','MX_Delay_2-source.zip','AGPL-3.0-LICENSE.txt','reverb-results/FDN.wav','reverb-results/IR.wav','reverb-results/HYBRID.wav','reverb-results/measurements.csv']
    for name in required:assert name in z.namelist(),name
    with tempfile.TemporaryDirectory(prefix='mx-zip-verify-') as tmp:
        z.extractall(tmp)
        subprocess.run([sys.executable,str(root/'scripts/verify_bundle.py'),str(Path(tmp)/'MX Delay 2.vst3')],check=True)
print(f'Install ZIP CRC, paths, documentation, audio evidence and complete x64 bundle verified: {archive.name}')

#!/usr/bin/env python3
"""Bundle experimental binaries with their license texts and full pinned source."""
import hashlib,shutil,subprocess,tarfile,zipfile
from pathlib import Path
root=Path(__file__).resolve().parents[1]
out=root/'build/release-0.1.0'
out.mkdir(parents=True,exist_ok=True)
for src,dst in [
 ('build/acid-fm1-0.1.0.fwsc','acid-fm1-0.1.0.fwsc'),
 ('build/acid-fm1.bin','acid-fm1-0.1.0-app.bin'),
 ('LICENSE','LICENSE-AGPL-3.0.txt'),('THIRD_PARTY.md','THIRD_PARTY.md'),
 ('base/LICENSE','LICENSE-MPL-2.0.txt'),('base/LICENSES/Apache-2.0.txt','LICENSE-Apache-2.0.txt'),
 ('base/assets/fonts/Barlow-OFL.txt','LICENSE-Barlow-OFL.txt'),
 ('docs/HARDWARE_TEST.md','HARDWARE_TEST.md'),
 ('docs/VALIDATION.md','VALIDATION.md'),('docs/BUILD_REPORT.json','BUILD_REPORT.json'),
]:shutil.copy(root/src,out/dst)
files=subprocess.check_output(['git','ls-files','--recurse-submodules'],cwd=root,text=True).splitlines()
archive=out/'acid-fm1-0.1.0-source.tar.gz'
with tarfile.open(archive,'w:gz') as tar:
    for name in files:
        p=root/name
        if p.is_file():tar.add(p,arcname='acid-fm1-0.1.0/'+name,recursive=False)
with zipfile.ZipFile(out/'acid-fm1-0.1.0-browser.zip','w',zipfile.ZIP_DEFLATED) as z:
    for p in sorted((root/'web').rglob('*')):
        if p.is_file():z.write(p,'acid-fm1-browser/'+str(p.relative_to(root/'web')))
    for name in ['LICENSE-AGPL-3.0.txt','LICENSE-MPL-2.0.txt','LICENSE-Apache-2.0.txt','LICENSE-Barlow-OFL.txt','THIRD_PARTY.md']:
        z.write(out/name,'acid-fm1-browser/'+name)
(out/'SHA256SUMS').write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.name+'\n' for p in sorted(out.iterdir()) if p.is_file() and p.name!='SHA256SUMS'))
print(out)

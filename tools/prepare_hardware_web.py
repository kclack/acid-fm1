#!/usr/bin/env python3
"""Adapt generated fm1-base emulator assets for a self-contained Pages site.
The upstream MPL-2.0 source remains pinned and unmodified in base/.
"""
from pathlib import Path
import shutil
root=Path(__file__).resolve().parents[1]
dest=root/'web/hardware'
p=dest/'index.html'
s=p.read_text()
s='\n'.join(line for line in s.splitlines() if 'fonts.googleapis.com' not in line and 'fonts.gstatic.com' not in line)
s=s.replace('<meta charset="utf-8">','<meta charset="utf-8"><link rel="icon" href="../favicon.svg" type="image/svg+xml">')
s=s.replace('<nav>','<nav><a href="../">Studio</a> ')
p.write_text(s+'\n')
(dest/'reader').mkdir(exist_ok=True)
for name in ['app.js','app_worklet.js','reader/words.js','reader/tts.js']:
    shutil.copy(root/'tools/hardware-assets'/name,dest/name)
shutil.copy(root/'base/LICENSE',dest/'LICENSE-MPL-2.0.txt')
(dest/'SOURCE.md').write_text('Hardware emulator: fm1-base, Charles Vestal and credited contributors, MPL-2.0.\nSource: https://github.com/charlesvestal/fm1-base/tree/6c0b47e4bfc56fd3cd5b34463210d0e7e0ce0431\nLocal generation changes: tools/prepare_hardware_web.py and tools/hardware-assets/.\nACID FM1 app/DSP: ../.., MIT.\nRasterized fonts: Barlow, SIL OFL 1.1; see the repository THIRD_PARTY.md and base/assets/fonts/.\n')
print('Hardware emulator: local fonts, optional hooks, and browser speech adapter prepared')

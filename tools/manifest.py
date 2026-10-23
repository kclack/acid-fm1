#!/usr/bin/env python3
"""Generate the browser parameter metadata from the C source of truth."""
import json,re
from pathlib import Path
root=Path(__file__).resolve().parents[1]
s=(root/'dsp/acid.c').read_text().split('#undef PARAM')[0]
entries=[]
for name,lo,hi,initial,discrete in re.findall(r'PARAM\("([^"]+)",([^,]+),([^,]+),([^,]+),([^\)]+)\)',s):
    entries.append(dict(id=len(entries),name=name,min=float(lo.rstrip('f')),max=float(hi.rstrip('f')),default=float(initial.rstrip('f')),discrete=bool(int(discrete))))
(root/'web/params.json').write_text(json.dumps(entries,indent=2)+'\n')
print(f'Generated {len(entries)} parameters')

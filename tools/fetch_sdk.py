#!/usr/bin/env python3
"""Fetch only the three Apache-2.0 SDK packaging files, with pinned hashes."""
import hashlib,json,sys,urllib.request
from pathlib import Path
rev='d179b4484759423312073f5fbb232501aa491047'
prefix=f'https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK/raw/{rev}/'
mirror=f'https://raw.githubusercontent.com/sqtech-ai/fw-AC79_AIoT_SDK/{rev}/'
files={
 'cpu/wl82/tools/uboot.boot':'4e3b4c220dc96641cb5a723f41e68ce41d5261ae9434bb33fbd7f2c59976ded4',
 'cpu/wl82/tools/cfg_tool.bin':'276579954f076886a6a7694f65dc71c034a63a2c204b76749065c0ac7b010d1b',
 'cpu/wl82/tools/cfg/eq_cfg_hw.bin':'41167491bffed4651750719c973d2758adeb9021a5670d02d6a53c85ed80ea7d',
}
dest=Path(sys.argv[1] if len(sys.argv)>1 else '.toolchains/ac79-sdk')
dest.mkdir(parents=True,exist_ok=True)
for name,sha in files.items():
    p=dest/name
    if p.exists() and hashlib.sha256(p.read_bytes()).hexdigest()==sha:continue
    error=None
    for base in (prefix,mirror):
        try:
            data=urllib.request.urlopen(base+name,timeout=45).read()
            if hashlib.sha256(data).hexdigest()!=sha:raise ValueError('SHA-256 mismatch: '+name)
            p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data);break
        except Exception as exc:error=exc
    else:raise SystemExit(str(error))
    print(name,sha)
for base in (prefix,mirror):
    try:(dest/'LICENSE').write_bytes(urllib.request.urlopen(base+'LICENSE',timeout=45).read());break
    except Exception:pass
else:raise SystemExit('Could not obtain the SDK Apache-2.0 license.')
(dest/'SOURCE.json').write_text(json.dumps({'revision':rev,'source':prefix,'mirror':mirror,'files':files},indent=2)+'\n')
print('AC79_SDK='+str(dest.resolve()))

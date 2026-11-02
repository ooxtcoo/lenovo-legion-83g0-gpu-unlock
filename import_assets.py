"""Import five hash-pinned, user-supplied assets for local compilation, without downloading."""
from pathlib import Path
import sys,json,hashlib,shutil
P=Path(__file__).resolve().parent
if len(sys.argv)!=2:raise SystemExit('Usage: python import_assets.py <locally-owned-assets-directory>')
S=Path(sys.argv[1]);pins=json.loads((P/'ASSET-PINS.json').read_text())['assets']
for name,v in pins.items():
 f=S/name
 if not f.is_file() or f.stat().st_size!=v['bytes'] or hashlib.sha256(f.read_bytes()).hexdigest()!=v['sha256']:raise SystemExit('Missing/mismatched asset: '+name)
(P/'assets').mkdir(exist_ok=True)
for name in pins:
 dst=P/'assets'/name
 if dst.exists() and dst.read_bytes()!=(S/name).read_bytes():raise SystemExit('Refusing to overwrite different asset: '+name)
 shutil.copyfile(S/name,dst)
print('PASS five pinned local assets copied; no hardware execution')

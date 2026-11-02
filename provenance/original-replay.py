"""Bounded offline whole-EfiMain replay; executes Windows fixture, never EFI."""
from pathlib import Path
import subprocess,sys,os,json,hashlib
P=Path(__file__).resolve().parent
lo,hi=map(int,sys.argv[1:3]);label=sys.argv[3] if len(sys.argv)>3 else f'replay-{lo}-{hi}'
trace=P/'evidence/traces'/label;trace.mkdir(parents=True,exist_ok=False)
cmd=[str(P/'host-app/app-tests.exe'),str(P.parent/'private/v4-host-prom-fixture.bin'),str(P.parent/'ad103m-uefi-port-candidate-v4/stock-assets'),str(lo),str(hi)]
r=subprocess.run(cmd,cwd=P,capture_output=True,text=True,env=dict(os.environ,GU_TRACE_DIR=str(trace)))
rec=dict(command=cmd,exit=r.returncode,stdout=r.stdout,stderr=r.stderr,source_sha256={str(f.relative_to(P/'src')):hashlib.sha256(f.read_bytes()).hexdigest() for f in (P/'src').rglob('*') if f.is_file()})
(P/'evidence'/f'{label}.json').write_text(json.dumps(rec,indent=2))
print(label,'exit',r.returncode,'passed',r.stdout.count('PASS whole-application case '));print((r.stdout+r.stderr)[-8500:] if r.returncode else '')
sys.exit(r.returncode)

"""Compile/run CPU-only actual checks.c classifier; never execute firmware."""
from pathlib import Path
import subprocess,json,sys
P=Path(__file__).resolve().parent
(P/'build').mkdir(exist_ok=True);(P/'evidence').mkdir(exist_ok=True)
I=P/'deps/edk2-include';J=P/'deps/reconnect-include'
exe=P/'build/classifier.exe'
cmd=['uv','run','--offline','--with','ziglang==0.13.0','python','-m','ziglang','cc','-std=c11','-fshort-wchar','-O1','-DGU_UEFI_PORT','-DGU_TARGET_PCI_ID=0x275710deU','-Isrc/compat','-I'+str(I),'-I'+str(I/'X64'),'-I'+str(J),'src/tests/test_final_classifier.c','src/checks.c','-o',str(exe)]
results=[]
for command in [cmd,[str(exe)]]:
 r=subprocess.run(command,cwd=P,capture_output=True,text=True)
 results.append(dict(command=command,exit=r.returncode,stdout=r.stdout,stderr=r.stderr))
 (P/'evidence/backup-classifier.json').write_text(json.dumps(results,indent=2))
 print(r.stdout+r.stderr)
 if r.returncode:sys.exit(r.returncode)

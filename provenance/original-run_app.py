"""Build and execute Windows synthetic callbacks ONLY. Never builds/executes EFI."""
from pathlib import Path
import shutil,subprocess,json,sys,hashlib,os
P=Path(__file__).resolve().parent
label=sys.argv[1] if len(sys.argv)>1 else 'latest'
S=Path(os.environ.get('GU_SOURCE',str(P/'src')))
H=Path(os.environ.get('GU_HOST_DIR',str(P/'host-app')));H.mkdir(parents=True,exist_ok=True)
shutil.copytree(S,H/'src',dirs_exist_ok=True)
freeze=P/'evidence/source-snapshots'/label
shutil.copytree(S,freeze,dirs_exist_ok=False)
for name in ['app.c','vendor/unlock_610.c']:
 f=H/'src'/name;t=f.read_text();line=t.splitlines()[0]
 assert 'executor_build_blocked.h' in line or 'V5_CODE_CLOSURE' in line
 f.write_text(t.replace(line,'#if !defined(_WIN32) || defined(__UEFI__)\n#error HOST_FIXTURE_WINDOWS_ONLY_NOT_UEFI\n#endif',1))
I=P.parent/'ad103m-port-research-v2/collector-headerfix-v4/edk2-include'
J=P.parent/'release-reconnect-v2/edk2-include'
# ConsoleOutDevice.h is separately pinned in the standalone fixture includes.
incs=[I,I/'X64',P.parent/'ad103m-port-research-v2/upstream/edk2/MdeModulePkg/Include']
for f in (P.parent/'release-reconnect-v2').rglob('ConsoleOutDevice.h'):incs.append(f.parents[1])
for f in (P.parent/'ad103m-uefi-port-candidate-v4').rglob('ConsoleOutDevice.h'):incs.append(f.parents[1])
cmd=['uv','run','--offline','--with','ziglang==0.13.0','python','-m','ziglang','cc','-std=c11','-fshort-wchar','-fno-lto','-O1','-D__KERNEL__','-DGU_UEFI_PORT','-DGU_TARGET_PCI_ID=0x275710deU','-Isrc/compat']+['-I'+str(i) for i in incs]+['src/tests/v5_app.c']+['src/'+x+'.c' for x in ['app','app_io','embedded','checks','ownership','port','compat','format','sha256','vendor/unlock_610','vendor/prep_610','vendor/layout_610','vendor/vbios_610','vendor/gfw_eval']]+(['src/lifecycle.c'] if (H/'src/lifecycle.c').exists() else [])+[os.environ.get('GU_EMBEDDED_ASM',str(P/'generated/embedded.s')),'-o','app-tests.exe']
results=[]
(P/'evidence'/f'{label}-source-sha256.json').write_text(json.dumps({str(f.relative_to(S)):hashlib.sha256(f.read_bytes()).hexdigest() for f in S.rglob('*') if f.is_file()},indent=2))
env=dict(os.environ);traces=P/'evidence/traces'/label;traces.mkdir(parents=True,exist_ok=False);env['GU_TRACE_DIR']=str(traces)
commands=[cmd,[str(H/'app-tests.exe'),str(P.parent/'private/v4-host-prom-fixture.bin'),str(P.parent/'ad103m-uefi-port-candidate-v4/stock-assets')]+(sys.argv[2:] or ['0'])]
for c in commands:
 r=subprocess.run(c,cwd=H,capture_output=True,text=True,env=env)
 results.append(dict(command=c,exit=r.returncode,stdout=r.stdout,stderr=r.stderr));(P/'evidence'/f'{label}.json').write_text(json.dumps(results,indent=2));print(r.stdout+r.stderr if r.returncode else "PASS command; whole-app cases="+str(r.stdout.count("PASS whole-application case "))+"; warnings="+str(r.stderr.count("warning:")))
 if r.returncode:sys.exit(r.returncode)

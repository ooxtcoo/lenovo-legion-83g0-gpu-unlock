"""Compile only. Never executes resulting EFI or accesses removable media."""
from pathlib import Path
import subprocess,json,sys,hashlib,struct
P=Path(__file__).resolve().parent
closure=json.loads((P/'evidence/code-closure-auto-v2.json').read_text())
assert closure['whole_application_cases']==575 and closure['core_cases']==43 and not closure['hardware_executed']
assert closure['source_sha256']=={str(f.relative_to(P/'src')):hashlib.sha256(f.read_bytes()).hexdigest() for f in (P/'src').rglob('*') if f.is_file()}
I=P/'deps/edk2-include'
J=P/'deps/reconnect-include'
(P/'build').mkdir(exist_ok=True)
cmd=['uv','run','--offline','--with','ziglang==0.13.0','python','-m','ziglang','cc','-target','x86_64-uefi','-std=c11','-fshort-wchar','-fno-lto','-ffreestanding','-fno-builtin','-fno-stack-protector','-mno-red-zone','-Os','-g0','-nostdlib','-D__KERNEL__','-DGU_UEFI_PORT','-DGU_TARGET_PCI_ID=0x275710deU','-Isrc/compat','-I'+str(I),'-I'+str(I/'X64'),'-I'+str(J)]+['src/'+n+'.c' for n in ['app','app_io','embedded','checks','ownership','port','compat','format','sha256','runtime','lifecycle','vendor/unlock_610','vendor/prep_610','vendor/layout_610','vendor/vbios_610','vendor/gfw_eval']]+[str(P/'generated/embedded.s'),'-Wl,-e,EfiMain','-v','-o','build/Lenovo-AD103M-Unlock-v5-auto-v2.efi']
r=subprocess.run(cmd,cwd=P,capture_output=True,text=True)
label=sys.argv[1] if len(sys.argv)>1 else 'efi-build'
(P/'evidence'/f'{label}.json').write_text(json.dumps(dict(command=cmd,exit=r.returncode,stdout=r.stdout,stderr=r.stderr),indent=2));print(r.stdout+r.stderr)
if r.returncode:sys.exit(r.returncode)
b=(P/'build/Lenovo-AD103M-Unlock-v5-auto-v2.efi').read_bytes();u16=lambda o:struct.unpack_from('<H',b,o)[0];u32=lambda o:struct.unpack_from('<I',b,o)[0]
pe=u32(60);opt=pe+24
assert b[:2]==b'MZ' and b[pe:pe+4]==b'PE\0\0'
assert u16(pe+4)==0x8664 and u16(opt)==0x20b and u16(opt+68)==10
assert not u16(pe+22)&1
for i in [1,4,9,11,12,13,14]:assert b[opt+112+i*8:opt+120+i*8]==bytes(8),i
rv,rz=u32(opt+152),u32(opt+156);assert rv and rz>=10
sections=[]
for i in range(u16(pe+6)):
 o=opt+u16(pe+20)+i*40;sections.append(dict(name=b[o:o+8].rstrip(b'\0').decode(),virtual_size=u32(o+8),rva=u32(o+12),raw_size=u32(o+16),raw=u32(o+20),flags=u32(o+36)))
assert all(s['raw']+s['raw_size']<=len(b) for s in sections)
padded=b+bytes(max(0,65536-len(b)));out=P/'build/Lenovo-AD103M-Unlock-v5-auto-v2-VENTOY.efi';out.write_bytes(padded)
assert out.read_bytes()[:len(b)]==b and out.read_bytes()[len(b):]==bytes(len(padded)-len(b))
report=dict(status='EFI_COMPILED_NOT_EXECUTED',original_bytes=len(b),sha256=hashlib.sha256(b).hexdigest(),delivery_bytes=len(padded),delivery_sha256=hashlib.sha256(padded).hexdigest(),zero_overlay_bytes=len(padded)-len(b),sections=sections,relocation_rva=rv,relocation_bytes=rz,imports=False,signature=False,hardware_executed=False)
(P/'evidence'/f'{label}-verification.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))

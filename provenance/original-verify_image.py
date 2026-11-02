"""Structural validation only: never loads/executes the EFI."""
from pathlib import Path
import hashlib,json,struct,re,difflib
P=Path(__file__).resolve().parent
sha=lambda b:hashlib.sha256(b).hexdigest()
F=P/'build/Lenovo-AD103M-Unlock-v5-auto-v2.efi';b=F.read_bytes()
u16=lambda o:struct.unpack_from('<H',b,o)[0]
u32=lambda o:struct.unpack_from('<I',b,o)[0]
u64=lambda o:struct.unpack_from('<Q',b,o)[0]
pe=u32(60);o=pe+24;entry=u32(o+16);base=u64(o+24);image=u32(o+56);sa=u32(o+32);fa=u32(o+36)
assert b[:2]==b'MZ' and b[pe:pe+4]==b'PE\0\0' and u16(pe+4)==0x8664 and u16(o)==0x20b and u16(o+68)==10
assert sa and not sa&(sa-1) and fa and not fa&(fa-1)
sections=[]
for i in range(u16(pe+6)):
 q=o+u16(pe+20)+40*i
 s=dict(name=b[q:q+8].rstrip(b'\0').decode(),size=u32(q+8),rva=u32(q+12),raw_size=u32(q+16),raw=u32(q+20),flags=u32(q+36));sections.append(s)
 assert s['rva']%sa==0 and s['raw']%fa==0 and s['raw_size']%fa==0
 assert s['raw']+s['raw_size']<=len(b) and s['rva']+s['size']<=image
 assert not (s['flags']&0x20000000 and s['flags']&0x80000000)
for a,c in zip(sections,sections[1:]):assert a['raw']+a['raw_size']<=c['raw'] and a['rva']+a['size']<=c['rva']
def raw(r,n=1):
 for s in sections:
  if s['rva']<=r and r+n<=s['rva']+s['raw_size']:return s['raw']+r-s['rva']
 raise AssertionError(('unmapped RVA',r,n))
assert any(s['rva']<=entry<s['rva']+s['size'] and s['flags']&0x20000000 for s in sections)
assert b[raw(entry):raw(entry)+16]!=bytes(16)
log=json.loads((P/'evidence/auto-v2-efi.json').read_text());assert log['exit']==0 and '-ENTRY:EfiMain' in log['stderr']
for i in [1,4,9,11,12,13,14]:assert b[o+112+8*i:o+120+8*i]==bytes(8)
rv,rs=u32(o+152),u32(o+156);assert rv and rs and not u16(pe+22)&1
r=raw(rv,rs);end=r+rs;fixups=[]
while r<end:
 page,sz=struct.unpack_from('<II',b,r);assert sz>=8 and sz%2==0 and r+sz<=end
 for k in range(r+8,r+sz,2):
  v=u16(k);typ=v>>12;assert typ in [0,10]
  if typ:target=page+(v&4095);raw(target,8);fixups.append(target)
 r+=sz
assert r==end and fixups
pins=json.loads((P.parent/'ad103m-uefi-port-candidate-v5-embedded-v1/evidence/embedded-pins.json').read_text());payloads={};ptrs=[];last=None
for name,pin in pins['assets'].items():
 data=(P/'assets'/name).read_bytes();assert len(data)==pin['bytes'] and sha(data)==pin['sha256']
 at=b.find(data);assert at>=0 and b.find(data,at+1)<0
 s=next(s for s in sections if s['raw']<=at and at+len(data)<=s['raw']+s['raw_size']);assert not s['flags']&0x20000000
 if last is not None:assert at==last
 last=at+len(data);rva=s['rva']+at-s['raw'];ptrs.extend([base+rva,base+rva+len(data)])
 payloads[name]=dict(raw=at,rva=rva,bytes=len(data),sha256=sha(b[at:at+len(data)]),section=s['name'])
table=struct.pack('<10Q',*ptrs);at=b.find(table);assert at>=0 and b.find(table,at+1)<0
s=next(s for s in sections if s['raw']<=at< s['raw']+s['raw_size']);table_rva=s['rva']+at-s['raw']
assert all(table_rva+8*i in fixups for i in range(10))
# Apply every DIR64 fixup to an offline bytearray, never execute it.
mapped=bytearray(image)
for s in sections:mapped[s['rva']:s['rva']+s['raw_size']]=b[s['raw']:s['raw']+s['raw_size']]
delta=0x10000000
for rva in fixups:struct.pack_into('<Q',mapped,rva,(struct.unpack_from('<Q',mapped,rva)[0]+delta)&((1<<64)-1))
for i,(name,pin) in enumerate(payloads.items()):
 begin,end=struct.unpack_from('<QQ',mapped,table_rva+i*16)
 assert begin==base+delta+pin['rva'] and end-begin==pin['bytes']
 assert sha(mapped[begin-base-delta:end-base-delta])==pin['sha256']

for f in [P.parent/'private/AD103-original.rom',P.parent/'private/v4-host-prom-fixture.bin']:
 assert b.find(f.read_bytes())<0

closure=json.loads((P/'evidence/code-closure-auto-v2.json').read_text())
assert closure['source_sha256']=={str(f.relative_to(P/'src')):sha(f.read_bytes()) for f in (P/'src').rglob('*') if f.is_file()}
report=dict(status='STRUCTURE_AND_PAYLOAD_PARITY_PASS_NOT_EXECUTED',efi_sha256=sha(b),efi_bytes=len(b),image_bytes=image,entry_rva=entry,entry_symbol='EfiMain',sections=sections,relocation_fixups=len(fixups),embedded_pointer_relocations=10,offline_relocation_simulation_passed=True,payloads=payloads,private_rom_embedded=False,hardware_executed=False,usb_deployed=False)
(P/'evidence/image-verification-auto-v2.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))

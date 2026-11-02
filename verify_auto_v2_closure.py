"""Verify actual offline runtime closure, negative controls and frozen baseline. No EFI execution."""
from pathlib import Path
import hashlib,json,re,difflib
P=Path(__file__).resolve().parent
B=P.parent/'ad103m-uefi-port-candidate-v5-auto-v1'
E=P/'evidence'
sha=lambda b:hashlib.sha256(b).hexdigest()
def pins(root):return {str(f.relative_to(root)):sha(f.read_bytes()) for f in root.rglob('*') if f.is_file()}
sources=pins(P/'src')
base=json.loads((E/'baseline-all-sha256.json').read_text())
assert pins(B)==base,'immutable auto-v1 baseline changed'
assert sha((B/'build/Lenovo-AD103M-Unlock-v5-auto-v1.efi').read_bytes())=='6f467dfcbfd513ba46ccd791616f74b1d25ae5aa6e73f9291fa01933e23689f0'
assert json.loads((E/'final-compile-source-sha256.json').read_text())==sources
compilelog=json.loads((E/'final-compile.json').read_text())
assert all(x['exit']==0 for x in compilelog)
for rel in sources:
    actual=(P/'host-app/src'/rel).read_text()
    expected=(P/'src'/rel).read_text()
    if rel in ['app.c',str(Path('vendor/unlock_610.c'))]:
        expected=expected.replace(expected.splitlines()[0],'#if !defined(_WIN32) || defined(__UEFI__)\n#error HOST_FIXTURE_WINDOWS_ONLY_NOT_UEFI\n#endif',1)
    assert actual==expected,rel
cases=set();records=[]
for f in sorted(E.glob('final-replay-*.json')):
    d=json.loads(f.read_text());assert d['exit']==0 and d['source_sha256']==sources,f
    lo,hi=map(int,d['command'][-2:]);assert hi-lo<30
    got=list(map(int,re.findall(r'PASS whole-application case (\d+)',d['stdout'])))
    assert got==list(range(lo,hi+1)) and not cases.intersection(got),f
    traces=E/'traces'/f.stem
    assert {x.name for x in traces.glob('case-*.txt')}=={f'case-{i}.txt' for i in got}
    cases.update(got);records.append(dict(file=f.name,sha256=sha(f.read_bytes()),cases=got,trace_sha256=pins(traces)))
assert cases==set(range(575))
negatives={}
for name,case in [('frozen-auto-v1',483),('omit-fresh-C',485),('omit-final-ss0-reread',525),('allow-zero-sentinel',508)]:
    f=E/f'negative-{name}.json';d=json.loads(f.read_text())
    assert len(d)==2 and d[0]['exit']==0 and d[1]['exit']!=0 and 'ASSERTION FAILED' in d[1]['stderr']
    assert int(d[1]['command'][-1])==case
    root=E/'negative-sources'/name
    assert json.loads((E/f'negative-{name}-source-sha256.json').read_text())==pins(root)
    assert pins(E/'source-snapshots'/f'negative-{name}')==pins(root)
    negatives[name]=dict(case=case,sha256=sha(f.read_bytes()),compile_exit=d[0]['exit'],runtime_exit=d[1]['exit'])
for f in (E/'negative-sources/frozen-auto-v1').rglob('*'):
    if f.is_file() and 'tests' not in f.relative_to(E/'negative-sources/frozen-auto-v1').parts:
        assert f.read_bytes()==(B/'src'/f.relative_to(E/'negative-sources/frozen-auto-v1')).read_bytes()
# Preserve first real RED and actual successful Green, not merely a source assertion.
for name in ['ss0-second-red','exceptional-red']:
    d=json.loads((E/(name+'.json')).read_text());assert d[0]['exit']==0 and d[1]['exit']!=0 and 'ASSERTION FAILED' in d[1]['stderr']
for name in ['ss0-second-green','exceptional-green-fixed-fixture']:
    assert all(x['exit']==0 for x in json.loads((E/(name+'.json')).read_text()))
changed=[];diff=[]
for f in sorted((P/'src').rglob('*')):
    if not f.is_file():continue
    rel=f.relative_to(P/'src');old=B/'src'/rel
    if f.read_bytes()!=old.read_bytes():
        changed.append(rel.as_posix())
        diff.extend(difflib.unified_diff(old.read_text().splitlines(True),f.read_text().splitlines(True),fromfile='auto-v1/'+rel.as_posix(),tofile='auto-v2/'+rel.as_posix()))
assert set(changed)=={'app.c','app_io.c','cold_report.inc','lifecycle.c','lifecycle.h','tests/v5_app.c'}
# All transaction, recovery, final graph/config classifier and StartImage-return bytes retained.
app=(P/'src/app.c').read_text();old=(B/'src/app.c').read_text()
assert app[app.index('    EFI_PCI_IO_PROTOCOL *prior='):]==old[old.index('    EFI_PCI_IO_PROTOCOL *prior='):]
assert re.findall(r'.*SetWatchdogTimer.*',app)==re.findall(r'.*SetWatchdogTimer.*',old)
life=(P/'src/lifecycle.c').read_text();oldlife=(B/'src/lifecycle.c').read_text()
assert life[:life.index('static int key(')]==oldlife[:oldlife.index('static int key(')]
assert life[life.index('BOOLEAN GuGraphAttached'):life.index('EFI_STATUS GuGraphConsent')]==oldlife[oldlife.index('BOOLEAN GuGraphAttached'):oldlife.index('EFI_STATUS GuGraphConsent')]
for rel in ['port.c','ownership.c','embedded.c','checks.c','checks.h','handoff.inc','final_config_report.inc','vendor/unlock_610.c','tests/test_efi_transaction.c','tests/test_final_classifier.c']:
    assert (P/'src'/rel).read_bytes()==(B/'src'/rel).read_bytes()
prior=json.loads((B/'evidence/code-closure-auto.json').read_text())
for kind in ['core','classifier']:
    f=Path(prior[kind+'_evidence_reused']);assert sha(f.read_bytes())==prior['reused_'+kind+'_sha256']
    assert json.loads(f.read_text())['exit']==0
# Assembly asset references are inherited immutable sibling files, not a new runtime transport.
assert (P/'generated/embedded.s').read_bytes()==(B/'generated/embedded.s').read_bytes()
for path in re.findall(r'\.incbin "([^"]+)"',(P/'generated/embedded.s').read_text()):
    f=Path(path);assert f.read_bytes()==(P/'assets'/f.name).read_bytes()==(B/'assets'/f.name).read_bytes()
(E/'auto-v2-vs-auto-v1.patch').write_text(''.join(diff))
report=dict(whole_application_cases=len(cases),case_ids=sorted(cases),new_cases=92,core_cases=prior['core_cases'],core_evidence_reused=prior['core_evidence_reused'],classifier_evidence_reused=prior['classifier_evidence_reused'],reused_core_sha256=prior['reused_core_sha256'],reused_classifier_sha256=prior['reused_classifier_sha256'],source_sha256=sources,host_executable_sha256=sha((P/'host-app/app-tests.exe').read_bytes()),baseline_files_verified=len(base),changed_source_files=changed,negative_controls=negatives,completed_replay_records=records,hardware_executed=False,efi_executed=False,usb_deployed=False,experimental_unknown_ss0_not_safety_proof=True)
(E/'code-closure-auto-v2.json').write_text(json.dumps(report,indent=2))
print('PASS',len(cases),'whole-EfiMain cases;',len(negatives),'negative controls;',len(base),'immutable baseline files; unchanged core/classifier reused')

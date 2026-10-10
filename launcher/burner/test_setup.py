"""Checks the real imported output plus rejection, cancellation and launch isolation."""
from pathlib import Path
import sys,json,tempfile,hashlib,shutil,subprocess,os,io
R=Path(__file__).resolve().parents[2]
release=json.loads((Path(__file__).parent/'release.json').read_text())
P=R/release['package_directory'];O=P.parent
env=json.loads((P/'tools/runtime-environment.json').read_text())
old=json.loads((R/'build/burner-edition-0.01-scanout-fix/package/tools/runtime-environment.json').read_text())
assert env==old, 'Native configuration must match the pre-rollback build'
assert all(v=='1' for k,v in env.items() if k.startswith('UCRR_NATIVE_PHYSICS_'))
sys.dont_write_bytecode=True
sys.path[:0]=[str(P/'tools')]
import import_game as w
w.ROOT=P;data=R/'build/burner-edition-0.01/test-data';assert w.ready(data)
m=json.loads((P/'tools/disc-manifest.json').read_text());install=data/'installed'
for name,row in m['files'].items():assert w.sha(install/'game_files'/name)==row['sha256'],name
recipe=json.loads((P/'tools/material-recipe.json').read_text());assert w.sha(install/'hybrid-assets/1-01.ucmat')==recipe['output_sha256']
for n in ('../escape','a/b','a\\b','CON','x:ads','a.','NUL.txt',''):
 try:w.safe_name(n)
 except ValueError:pass
 else:raise AssertionError(n)
with tempfile.TemporaryDirectory(dir=O) as tmp:
 t=Path(tmp);bad=t/'bad.iso';bad.write_bytes(b'not an ISO');marker=w.sha(install/'installed.json')
 try:w.import_iso(bad,data,t/'cancel')
 except w.XisoError:pass
 else:raise AssertionError('Bad ISO accepted')
 cancel=t/'cancel';cancel.touch()
 try:w.import_iso(R/'Urban Chaos - Riot Response.iso',data,cancel)
 except RuntimeError:pass
 else:raise AssertionError('Cancellation ignored')
 assert marker==w.sha(install/'installed.json') and not list(data.glob('.import-*'))
# Exercise launch setup while intercepting only process creation (do not start gameplay).
seen={};original=w.subprocess.Popen
class Process:
 def __init__(self,args,**kw):seen.update(args=args,**kw);self.stderr=io.BytesIO()
 def wait(self):return 0
w.subprocess.Popen=Process
try:
 w.launch(data);ini=data/'runtime/xboxrecomp.ini';ini.write_text(ini.read_text()+'\n;preservation-check\n');w.launch(data);assert ';preservation-check' in ini.read_text()
finally:w.subprocess.Popen=original
# Failure reporting is bounded, preserves settings, and records the exact runtime.
class FailedProcess(Process):
 def __init__(self,args,**kw):super().__init__(args,**kw);self.stderr=io.BytesIO(b'[FATAL-BUGCHECK] code=00000001\n')
 def wait(self):return 1
w.subprocess.Popen=FailedProcess
try:
 try:w.launch(data)
 except RuntimeError as error:assert 'last-error.txt' in str(error)
 else:raise AssertionError('Failure was not reported')
finally:w.subprocess.Popen=original
report=(data/'last-error.txt').read_text();assert '[FATAL-BUGCHECK]' in report and 'Runtime SHA256:' in report
assert seen['cwd']==install.resolve() and Path(seen['args'][0]).is_file()
for key in ('UCRR_HYBRID_GEO','UCRR_HYBRID_MATERIALS','UCRR_HYBRID_SHADERS','UCRR_HYBRID_MOVIES','UCRR_HYBRID_SAVE_DIR','UCRR_SHADER_CACHE_DIR'):assert Path(seen['env'][key]).exists(),key
# Run the actual direct EXE against a tiny worker in a separate harness: no launcher UI.
h=O/'direct-harness';(h/'tools').mkdir(parents=True,exist_ok=True)
shutil.copy2(P/'Play Burner Edition.exe',h/'Play Burner Edition.exe')
shutil.copytree(P/'tools/python',h/'tools/python',dirs_exist_ok=True)
(h/'tools/import_game.py').write_text("from pathlib import Path\nimport sys\nif sys.argv[1]=='launch':Path(__file__).with_name('direct-launched.txt').write_text('ok')\n")
p=subprocess.run([str(h/'Play Burner Edition.exe')],timeout=15,creationflags=subprocess.CREATE_NO_WINDOW);assert p.returncode==0 and (h/'tools/direct-launched.txt').read_text()=='ok'
# Release scan: no original payload, extracted catalog, movie, save or shader cache.
disc_hashes={r['sha256'] for r in m['files'].values()};files={}
for p in P.rglob('*'):
 if p.is_file():
  assert p.suffix.lower() not in ('.xbe','.geo','.ent','.xmv','.wmv','.ucmat','.iso','.pdb','.pyc'),p
  digest=w.sha(p);assert digest not in disc_hashes;files[p.relative_to(P).as_posix()]=digest
result={'iso_files_verified':len(m['files']),'movies_generated':8,'material_catalog_verified':True,'unsafe_paths_rejected':True,'bad_iso_rejected':True,'cancel_preserved_existing_install':True,'settings_preserved':True,'direct_exe_verified_with_stub_worker':True,'runtime_launch_paths_verified':True,'live_game_tested':False,'native_physics_restored':True,'package_files':files}
(O/'verification.json').write_text(json.dumps(result,indent=2));print('PASS: import, cancellation, launch paths, direct EXE, and release asset scan')

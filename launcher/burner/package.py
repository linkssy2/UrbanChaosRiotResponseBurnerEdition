"""Assemble Burner Edition from an explicit file allowlist; no disc payloads."""
from pathlib import Path
import sys,json,hashlib,struct,zlib,shutil,subprocess
R=Path(__file__).resolve().parents[2];S=Path(__file__).parent
sys.path[:0]=[str(S),str(R/'third_party/xboxrecomp/tools/xiso')]
import import_game as worker
release=json.loads((S/'release.json').read_text())
O=R/release['package_directory'];O.mkdir(parents=True,exist_ok=True)
assert worker.sha(R/release['runtime'])==release['runtime_sha256'], 'Runtime differs from tested release'
def copy(p,q):
 q.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,q)
def dump(p,v):p.write_text(json.dumps(v,indent=2)+'\n')
T=O/'tools';T.mkdir(exist_ok=True)
manifest=S/'disc-manifest.json'
if not manifest.exists():
 files={}
 with worker.Disc(R/'Urban Chaos - Riot Response.iso') as disc:
  for key,(name,e) in sorted(disc.entries().items()):
   disc._f.seek(disc.base+e.sector*2048);h=hashlib.sha256();left=e.size
   while left:
    b=disc._f.read(min(left,1024*1024));assert b;h.update(b);left-=len(b)
   files[name]={'size':e.size,'sha256':h.hexdigest()}
 dump(manifest,{'version':1,'files':files})
 print('Disc manifest:',len(files),flush=True)
raw=(R/'game_files/1-01.geo').read_bytes();entries=worker.archive_entries(raw);lookup={n:i for i,(n,*_) in enumerate(entries)}
catalog=R/'build/hybrid-pause-handoff-fix-20261009/package/hybrid-assets/1-01.ucmat';data=catalog.read_bytes();count=struct.unpack_from('<I',data,8)[0];at=44;rows=[]
for _ in range(count):
 kind,reason,n,t,*fields=struct.unpack_from('<BBHH6BI',data,at);identity=data[at+16:at+48].hex();periods=struct.unpack_from('<2f',data,at+48);at+=56
 name=data[at:at+n].rstrip(b'\0');at+=n;texture=data[at:at+t].rstrip(b'\0');at+=t
 i=lookup[name];_,off,size,packed=entries[i];payload=zlib.decompress(raw[off:off+packed]);assert hashlib.sha256(payload).hexdigest()==identity
 pos=payload.find(texture) if texture else None
 assert pos is None or pos>=0
 rows.append([i,[kind,reason,*fields],periods,pos,len(texture),identity])
assert at==len(data)
recipe={'version':data[:8].decode(),'archive':'1-01.geo','archive_sha256':hashlib.sha256(raw).hexdigest(),'output_sha256':worker.sha(catalog),'records':rows}
assert data[12:44].hex()==recipe['archive_sha256'];dump(S/'material-recipe.json',recipe)
for name in ('import_game.py','disc-manifest.json','material-recipe.json'):copy(S/name,T/name)
copy(R/'third_party/xboxrecomp/tools/xiso/xdvdfs.py',T/'xdvdfs.py')
env=json.loads((R/'build/hybrid-pause-handoff-fix-20261009/package/evidence.json').read_text())['launcher_environment']
for k in list(env):
 if k in ('UCRR_HYBRID_GEO','UCRR_HYBRID_MATERIALS','UCRR_HYBRID_SHADERS','UCRR_HYBRID_MOVIES','UCRR_HYBRID_SAVE_DIR','UCRR_SHADER_CACHE_DIR'):del env[k]
assert all('Recompilations' not in v and ':/' not in v and ':\\' not in v for v in env.values()),[(k,v)for k,v in env.items()if ':' in v]
dump(T/'runtime-environment.json',env)
for name in ('python.exe','python312.dll','python3.dll','python312.zip','vcruntime140.dll','vcruntime140_1.dll','_hashlib.pyd','libcrypto-3.dll','LICENSE.txt'):
 copy(R/'.tools/python'/name,T/'python'/name)
(T/'python/python312._pth').write_text('python312.zip\n.\n..\n')
ff=Path('E:/Launchbox/ThirdParty/FFMPEG')
for name in ('ffmpeg.exe','avcodec-62.dll','avdevice-62.dll','avfilter-11.dll','avformat-62.dll','avutil-60.dll','swresample-6.dll','swscale-9.dll','LICENSE.txt','README.txt'):copy(ff/name,T/'ffmpeg'/name)
for p in (R/'build/hybrid-pause-handoff-fix-20261009/package/hybrid-assets').glob('*.hlsl'):copy(p,O/'shaders'/p.name)
copy(R/release['runtime'],O/'runtime/urban_chaos_riot_response_recompiled.exe')
copy(R/'build/hybrid-model-lighting-20261006/runtime-build/bin/RelWithDebInfo/zlib1.dll',O/'runtime/zlib1.dll')
copy(R/'packaging/xboxrecomp.ini',O/'runtime/xboxrecomp.ini')
copy(S/'cover.png',O/'cover.png')
copy(R/'third_party/xboxrecomp/LICENSE',O/'licenses/xboxrecomp.txt')
copy(S/'README-release.txt',O/'README.txt')
print(O,flush=True)


copy(R/'third_party/xboxrecomp/NOTICE',O/'licenses/xboxrecomp-notice.txt')
for p in (R/'third_party/xboxrecomp/LICENSES').glob('*.txt'):copy(p,O/'licenses'/p.name)
copy(R/'third_party/JoltPhysics/LICENSE',O/'licenses/JoltPhysics.txt')
for name in ('Launcher.cs','import_game.py','build.ps1','release.json','README-release.txt'):copy(S/name,O/'launcher-source'/name)
(O/'licenses/DEPENDENCIES.txt').write_text('Python 3.12: tools/python/LICENSE.txt; https://www.python.org/downloads/source/\nFFmpeg 8.1.2 shared build: tools/ffmpeg/LICENSE.txt and README.txt; source https://github.com/FFmpeg/FFmpeg/commit/38b88335f9\nxboxrecomp and its component notices: xboxrecomp.txt and xboxrecomp-notice.txt.\nJolt Physics: JoltPhysics.txt.\nLauncher artwork was generated for Burner Edition from the local cover reference; it is not extracted disc content.\n')

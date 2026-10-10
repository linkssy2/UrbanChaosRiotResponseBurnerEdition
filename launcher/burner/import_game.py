"""Local ISO import and launch. Release carries conversion rules, never disc payloads."""
from pathlib import Path
import argparse, hashlib, json, os, shutil, struct, subprocess, sys, time, uuid, zlib
from collections import deque
from xdvdfs import Xiso, XisoError
import hud_import
ROOT=Path(__file__).resolve().parent.parent
BLOCK=1024*1024

def sha(path):
    with open(path,'rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def supported_hash(name,digest,expected):
    # This retail XBE differs only in certificate title/media fields; code and assets match.
    return digest==expected or (name.lower()=='default.xbe' and digest=='c6f0c99ad57eafa2fc9caedb5ab2a7b192ff5fdde7e5faddc5e62a1730f044d1')

def progress(message,percent=0):print(json.dumps({'message':message,'percent':percent}),flush=True)
def check_cancel(path):
    if path.exists():raise RuntimeError('Import cancelled. Your existing game data was kept.')
def safe_name(name):
    """Reject paths and Windows device names before extracting user-supplied files."""
    if not name or name in ('.','..') or name[-1] in '. ' or any(c in name for c in '/\\:<>"|?*') or any(ord(c)<32 for c in name):raise ValueError('Invalid ISO filename')
    if name.split('.')[0].upper() in {'CON','PRN','AUX','NUL',*[f'COM{i}' for i in range(10)],*[f'LPT{i}' for i in range(10)]}:raise ValueError('Reserved ISO filename')
    return name
class Disc(Xiso):
    def __init__(self,path):
        try:super().__init__(path)
        except BaseException:
            if hasattr(self,"_f"):self._f.close()
            raise
    def _read_at(self,sector,size):
        at=self.base+sector*2048
        if size<0 or at<self.base or at+size>os.fstat(self._f.fileno()).st_size:raise XisoError('Truncated ISO')
        self._f.seek(at);data=self._f.read(size)
        if len(data)!=size:raise XisoError('Truncated ISO')
        return data
    def entries(self):
        result={};seen=set();todo=[('',self.root_sector,self.root_size,0)]
        while todo:
            folder,sector,size,depth=todo.pop()
            if depth>16 or size>16*BLOCK or (sector,size) in seen:raise XisoError('Invalid ISO directory tree')
            seen.add((sector,size))
            for entry in self._read_dir(sector,size):
                name=safe_name(entry.name);path=(folder+'/'+name).lstrip('/');key=path.lower()
                if key in result:raise XisoError('Duplicate ISO path')
                if self.base+entry.sector*2048+entry.size>os.fstat(self._f.fileno()).st_size:raise XisoError('Truncated ISO file')
                result[key]=(path,entry)
                if len(result)>10000:raise XisoError('Excessive ISO directory')
                if entry.is_dir:todo.append((path,entry.sector,entry.size,depth+1))
        return {k:v for k,v in result.items() if not v[1].is_dir}

def archive_entries(raw):
    """Read bounded archive directory entries; payloads are checked by material_catalog."""
    if len(raw)<200:raise ValueError('Invalid material archive')
    end=196+struct.unpack_from('<I',raw,192)[0];count=struct.unpack_from('<I',raw,196)[0];at=200;entries=[]
    if count>100000 or end>len(raw):raise ValueError('Invalid material directory')
    for _ in range(count):
        n=struct.unpack_from('<H',raw,at)[0];at+=2;name=raw[at:at+n].rstrip(b'\0');at+=n
        offset,size,packed,bank=struct.unpack_from('<IIII',raw,at);at+=16
        if at>end or offset+packed>len(raw) or size>256*BLOCK:raise ValueError('Invalid material entry')
        entries.append((name,offset,size,packed))
    if at!=end:raise ValueError('Invalid material directory extent')
    return entries

def material_catalog(game,output,recipe):
    """Rebuild the scenery catalog from the verified local archive and conversion recipe."""
    raw=(game/recipe['archive']).read_bytes()
    if hashlib.sha256(raw).hexdigest()!=recipe['archive_sha256']:raise ValueError('Unsupported scenery archive')
    entries=archive_entries(raw);records=[]
    for row in recipe['records']:
        index,fields,periods,tex_at,tex_size,identity=row
        name,offset,size,packed=entries[index];decoder=zlib.decompressobj();payload=decoder.decompress(raw[offset:offset+packed],size+1)
        if len(payload)!=size or not decoder.eof or decoder.unused_data or decoder.unconsumed_tail or hashlib.sha256(payload).hexdigest()!=identity:raise ValueError('Invalid material payload')
        texture=payload[tex_at:tex_at+tex_size] if tex_at is not None else b''
        if len(texture)!=tex_size:raise ValueError('Invalid material texture reference')
        name+=b'\0';texture+=b'\0';kind,reason,cull,address,blend,exposure,bias,geometry,constant=fields
        records.append(struct.pack('<BBHH6BI',kind,reason,len(name),len(texture),cull,address,blend,exposure,bias,geometry,constant)+bytes.fromhex(identity)+struct.pack('<2f',*periods)+name+texture)
    output.write_bytes(recipe['version'].encode()+struct.pack('<I',len(records))+bytes.fromhex(recipe['archive_sha256'])+b''.join(records))
    if sha(output)!=recipe['output_sha256']:raise ValueError('Material conversion verification failed')

def movie_convert(game,out,cancel):
    """Remux Xbox video and mix its supported audio tracks locally; cancellation stops FFmpeg."""
    out.mkdir();ffmpeg=ROOT/'tools/ffmpeg/ffmpeg.exe'
    for index,name in enumerate(('dolby','eidoslogo','rocklogo','intro_12_edit','gamelogo','splash','fe_bk1','intro_1')):
        check_cancel(cancel);progress('Preparing video '+str(index+1)+' of 8',85+index)
        source=next(p for p in (game/'Movies').iterdir() if p.stem.lower()==name)
        with source.open('rb') as f:header=f.read(3108)
        version,w,h,duration,n,_=struct.unpack_from('<IIIIHH',header,16)
        if header[12:16]!=b'xobX' or version!=4 or n>256 or not 0<w<=4096 or not 0<h<=4096:raise ValueError('Unsupported movie')
        filters=[];chosen=list(range(min(2,n)))
        for i in chosen:
            fmt,channels,rate,bits,flags=struct.unpack_from('<HHIHH',header,36+12*i)
            if (flags,channels) in ((1,2),(0,2)):pan='pan=stereo|c0=c0|c1=c1'
            elif (flags,channels) in ((2,2),(0,1)):pan='pan=stereo|c0=0.70710678*c0|c1=0.70710678*c0'
            else:raise ValueError('Unsupported movie audio')
            filters.append(f'[0:a:{i}]{pan},aresample=48000[a{i}]')
        if len(chosen)>1:filters.append(''.join(f'[a{i}]' for i in chosen)+f'amix=inputs={len(chosen)}:normalize=0:duration=longest[audio]')
        args=[str(ffmpeg),'-nostdin','-hide_banner','-loglevel','error','-y','-i',str(source),'-map','0:v:0','-c:v','copy']
        if chosen:args+=['-filter_complex',';'.join(filters),'-map','[audio]' if len(chosen)>1 else '[a0]','-c:a','pcm_s16le','-ar','48000','-ac','2']
        else:args+=['-an']
        dest=out/(name+'.wmv');args+=['-map_metadata','-1','-f','asf',str(dest)]
        with (out/'conversion.log').open('ab') as log:
            p=subprocess.Popen(args,stdout=log,stderr=log,creationflags=subprocess.CREATE_NO_WINDOW)
            try:
                while p.poll() is None:check_cancel(cancel);time.sleep(.1)
                if p.returncode or not dest.is_file() or dest.stat().st_size<100:raise RuntimeError('Video conversion failed; see conversion.log')
            finally:
                if p.poll() is None:p.terminate();p.wait()

def import_iso(iso_path,data,cancel):
    """Verify and stage the complete import before swapping it into place; saves are separate."""
    manifest=json.loads((ROOT/'tools/disc-manifest.json').read_text());data.mkdir(parents=True,exist_ok=True)
    stage=data/('.import-'+uuid.uuid4().hex);stage.mkdir();game=stage/'game_files';game.mkdir()
    try:
        progress('Checking Xbox disc',1)
        with Disc(iso_path) as disc:
            entries=disc.entries();required=manifest['files']
            for name,row in required.items():
                if name.lower() not in entries or entries[name.lower()][1].size!=row['size']:raise ValueError('This ISO is not the supported Urban Chaos disc revision.')
            # Reject other executables/revisions before writing gigabytes.
            entry=entries['default.xbe'][1]
            if not supported_hash('Default.xbe',hashlib.sha256(disc.read(entry)).hexdigest(),required['Default.xbe']['sha256']):raise ValueError('Unsupported Xbox executable revision')
            total=sum(r['size'] for r in required.values());done=0
            free=shutil.disk_usage(data).free
            if free<total+2*1024**3:raise ValueError('At least 6 GB of free space is needed for import.')
            for name,row in required.items():
                check_cancel(cancel);entry=entries[name.lower()][1];dest=game.joinpath(*name.split('/'));dest.parent.mkdir(parents=True,exist_ok=True)
                disc._f.seek(disc.base+entry.sector*2048);digest=hashlib.sha256();left=entry.size
                with dest.open('xb') as output:
                    while left:
                        check_cancel(cancel);chunk=disc._f.read(min(BLOCK,left))
                        if not chunk:raise ValueError('Truncated ISO')
                        output.write(chunk);digest.update(chunk);left-=len(chunk);done+=len(chunk)
                if not supported_hash(name,digest.hexdigest(),row['sha256']):raise ValueError('Disc verification failed: '+name)
                progress('Importing game files',int(done*82/total)+2)
        check_cancel(cancel);assets=stage/'hybrid-assets';assets.mkdir()
        material_catalog(game,assets/'1-01.ucmat',json.loads((ROOT/'tools/material-recipe.json').read_text()))
        hud_import.prepare(game,assets/'pc-hud.bin',manifest,archive_entries)
        movie_convert(game,stage/'native-movies',cancel);check_cancel(cancel)
        (stage/'installed.json').write_text(json.dumps({'version':1,'disc_manifest':sha(ROOT/'tools/disc-manifest.json'),'files':len(required)}))
        install=data/'installed';backup=data/('.previous-'+uuid.uuid4().hex)
        if install.exists():install.rename(backup)
        try:stage.rename(install)
        except BaseException:
            if backup.exists():backup.rename(install)
            raise
        # Previous complete data is retained; saves live outside these directories.
        progress('Ready to play',100)
    finally:
        if stage.exists():
            assert stage.resolve().parent==data.resolve() and stage.name.startswith('.import-')
            shutil.rmtree(stage)

def ready(data):
    """Check installation identity and required outputs without hashing gigabytes on every launch."""
    install=data/'installed';marker=install/'installed.json'
    if not marker.is_file():return False
    try:
        if json.loads(marker.read_text())['disc_manifest']!=sha(ROOT/'tools/disc-manifest.json'):return False
        manifest=json.loads((ROOT/'tools/disc-manifest.json').read_text())
        if any(not (install/'game_files'/n).is_file() or (install/'game_files'/n).stat().st_size!=r['size'] for n,r in manifest['files'].items()):return False
        return (install/'hybrid-assets/1-01.ucmat').is_file() and all((install/'native-movies'/(n+'.wmv')).is_file() for n in ('dolby','eidoslogo','rocklogo','intro_12_edit','gamelogo','splash','fe_bk1','intro_1'))
    except (OSError,ValueError,KeyError):return False

def launch(data):
    """Preserve settings, isolate runtime flags, and retain bounded error output only on failure."""
    if not ready(data):raise ValueError('Please import your Xbox ISO first.')
    data=data.resolve();install=data/'installed';runtime=data/'runtime';runtime.mkdir(exist_ok=True)
    for name in ('saves','ShaderCache'):(data/name).mkdir(exist_ok=True)
    manifest=json.loads((ROOT/'tools/disc-manifest.json').read_text())
    if not supported_hash('Default.xbe',sha(install/'game_files/Default.xbe'),manifest['files']['Default.xbe']['sha256']):raise ValueError('Game executable verification failed; import the ISO again.')
    hud_import.prepare(install/'game_files',install/'hybrid-assets/pc-hud.bin',manifest,archive_entries)
    for name in ('urban_chaos_riot_response_recompiled.exe','zlib1.dll'):
        source=ROOT/'runtime'/name;dest=runtime/name
        if not dest.is_file() or sha(dest)!=sha(source):shutil.copy2(source,dest)
    ini=runtime/'xboxrecomp.ini'
    if not ini.exists():shutil.copy2(ROOT/'runtime/xboxrecomp.ini',ini)
    env={k:v for k,v in os.environ.items() if not k.upper().startswith(('UCRR_','XBOX_LOG_'))}
    env.update(json.loads((ROOT/'tools/runtime-environment.json').read_text()))
    env['UCRR_PC_HUD_PIXELS']=str(install/'hybrid-assets/pc-hud.bin')
    env.update(UCRR_HYBRID_GEO=str(install/'game_files/1-01.geo'),UCRR_HYBRID_MATERIALS=str(install/'hybrid-assets/1-01.ucmat'),UCRR_HYBRID_SHADERS=str(ROOT/'shaders'),UCRR_HYBRID_MOVIES=str(install/'native-movies'),UCRR_HYBRID_SAVE_DIR=str(data/'saves'),UCRR_SHADER_CACHE_DIR=str(data/'ShaderCache'))
    with open(os.devnull,'wb') as null:
        p=subprocess.Popen([str(runtime/'urban_chaos_riot_response_recompiled.exe')],cwd=install,env=env,stdout=null,stderr=subprocess.PIPE,creationflags=subprocess.CREATE_NO_WINDOW)
        tail=deque(maxlen=32)
        while True:
            chunk=p.stderr.read(4096)
            if not chunk:break
            tail.append(chunk)
        code=p.wait()
    if code:
        report=data/'last-error.txt';report.write_bytes(('Exit code: '+hex(code&0xffffffff)+'\nRuntime SHA256: '+sha(runtime/'urban_chaos_riot_response_recompiled.exe')+'\n'+ini.read_text(errors='replace')+'\n').encode()+b''.join(tail))
        raise RuntimeError('The game closed unexpectedly (code '+hex(code&0xffffffff)+'). Error details were saved to '+str(report))

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('action',choices=['import','status','launch']);p.add_argument('--data',type=Path,required=True);p.add_argument('--iso',type=Path);p.add_argument('--cancel',type=Path);a=p.parse_args()
    try:
        if a.action=='import':import_iso(a.iso,a.data,a.cancel)
        elif a.action=='status':sys.exit(0 if ready(a.data) else 2)
        else:launch(a.data)
    except Exception as e:progress(str(e),0);sys.exit(1)

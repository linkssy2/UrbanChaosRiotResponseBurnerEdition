"""Disc validation reused from Burner Edition 0.02; no game payloads are included."""
from pathlib import Path
import hashlib, os, json
from xdvdfs import Xiso, XisoError
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


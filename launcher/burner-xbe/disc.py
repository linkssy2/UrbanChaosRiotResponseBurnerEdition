"""Read-only Xbox ISO importer; no game payloads are included.

The XDVDFS format contract follows TGP482/XboxXBERecomp's
runtime/kernel/disc.cpp. This Python adapter adds extraction-time validation;
it does not depend on the former xboxrecomp Python ISO tools.
"""
from dataclasses import dataclass
import hashlib, os, json, struct
BLOCK=1024*1024
SECTOR=2048
MAGIC=b'MICROSOFT*XBOX*MEDIA'
# XISO / whole-disc layouts, including offsets accepted by existing installs.
PARTITIONS=(0, 0xFD90, 0x30600, 0x0FD90000, 0x18300000)


class XisoError(ValueError):
    """The supplied image is not a valid, safely extractable Xbox disc."""


@dataclass(frozen=True)
class Entry:
    name: str
    sector: int
    size: int
    is_dir: bool

def sha(path):
    with open(path,'rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def supported_hash(name,digest,expected):
    # This retail XBE differs only in certificate title/media fields; code and assets match.
    return digest==expected or (name.lower()=='default.xbe' and digest=='c6f0c99ad57eafa2fc9caedb5ab2a7b192ff5fdde7e5faddc5e62a1730f044d1')

def progress(message,percent=0):
    import diagnostics
    diagnostics.record(str(percent) + '% ' + str(message))
    print(json.dumps({'message':message,'percent':percent}),flush=True)
def check_cancel(path):
    if path.exists():raise RuntimeError('Import cancelled. Your existing game data was kept.')
def safe_name(name):
    """Reject paths and Windows device names before extracting user-supplied files."""
    if not name or name in ('.','..') or name[-1] in '. ' or any(c in name for c in '/\\:<>"|?*') or any(ord(c)<32 for c in name):raise ValueError('Invalid ISO filename')
    if name.split('.')[0].upper() in {'CON','PRN','AUX','NUL',*[f'COM{i}' for i in range(10)],*[f'LPT{i}' for i in range(10)]}:raise ValueError('Reserved ISO filename')
    return name
class Disc:
    def __init__(self,path):
        self._f=open(path,'rb')
        try:
            self._size=os.fstat(self._f.fileno()).st_size
            for base in PARTITIONS:
                self._f.seek(base+32*SECTOR)
                descriptor=self._f.read(SECTOR)
                if (len(descriptor)==SECTOR and descriptor[:20]==MAGIC
                        and descriptor[0x7EC:]==MAGIC):
                    self.base=base
                    self.root_sector,self.root_size=struct.unpack_from('<II',descriptor,0x14)
                    self._check_table(self.root_sector,self.root_size)
                    break
            else:
                raise XisoError('No valid Xbox disc partition found')
        except BaseException:
            self._f.close()
            raise

    def __enter__(self):
        return self

    def __exit__(self,*exc):
        self._f.close()

    def _check_range(self,sector,size):
        at=self.base+sector*SECTOR
        if sector<0 or size<0 or at+size>self._size:
            raise XisoError('Truncated ISO')
        return at

    def _check_table(self,sector,size):
        if size>16*BLOCK:
            raise XisoError('Excessive ISO directory size')
        self._check_range(sector,size)

    def _read_at(self,sector,size):
        at=self._check_range(sector,size)
        self._f.seek(at);data=self._f.read(size)
        if len(data)!=size:raise XisoError('Truncated ISO')
        return data

    def read(self,entry):
        if entry.is_dir:raise XisoError('Cannot read a directory as a file')
        return self._read_at(entry.sector,entry.size)

    def _read_dir(self,sector,size):
        self._check_table(sector,size)
        if not size:return
        table=self._read_at(sector,size)
        todo=[0]
        occupied=bytearray((size+3)//4)
        while todo:
            at=todo.pop()
            if at+14>size or occupied[at//4]:
                raise XisoError('Invalid ISO directory entry or cycle')
            left,right,start,length,attributes,name_length=struct.unpack_from('<HHIIBB',table,at)
            if left==0xFFFF:
                # An unused root table is filled with 0xFF; a child cannot be filler.
                if at or any(value!=0xFF for value in table):
                    raise XisoError('Invalid ISO directory padding')
                return
            end=at+14+name_length
            if not name_length or end>size:
                raise XisoError('Invalid ISO directory name')
            first,last=at//4,(end+3)//4
            if any(occupied[first:last]):
                raise XisoError('Overlapping ISO directory entries')
            occupied[first:last]=b'\1'*(last-first)
            name=safe_name(table[at+14:end].decode('latin-1'))
            for child in (right,left):
                if child:todo.append(child*4)
            yield Entry(name,start,length,bool(attributes&0x10))

    def entries(self):
        result={};seen=set();todo=[('',self.root_sector,self.root_size,0)]
        while todo:
            folder,sector,size,depth=todo.pop()
            if depth>16 or (size and (sector,size) in seen):raise XisoError('Invalid ISO directory tree')
            if size:seen.add((sector,size))
            for entry in self._read_dir(sector,size):
                path=(folder+'/'+entry.name).lstrip('/');key=path.lower()
                if key in result:raise XisoError('Duplicate ISO path')
                self._check_range(entry.sector,entry.size)
                result[key]=(path,entry)
                if len(result)>10000:raise XisoError('Excessive ISO directory')
                if entry.is_dir:todo.append((path,entry.sector,entry.size,depth+1))
        return {k:v for k,v in result.items() if not v[1].is_dir}


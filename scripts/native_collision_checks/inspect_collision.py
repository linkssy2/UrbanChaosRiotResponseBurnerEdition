"""Read-only .geohk oracle: 2282C0 prefix, 2E3720 packfile, embedded reflection."""
from pathlib import Path
import struct
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from analyze_frontend import parse_archive,unpack_entry

class Collision:
    def __init__(self,data):
        self.data=data;pos=0
        def word():
            nonlocal pos
            v=struct.unpack_from('<I',data,pos)[0];pos+=4;return v
        self.groups=[]
        for _ in range(word()):
            n=word();self.groups.append(n)
            assert not any(word() for _ in range(n*3))
        present=word();self.parts=[]
        if not present:
            assert pos==len(data);self.p=b'';return
        assert present==1
        size=word();assert pos+size==len(data);self.prefix=pos;self.p=data[pos:]
        p=self.p
        assert p[:20]==bytes.fromhex('57e0e05710c0c010736e61700200000004010001')
        self.sections=[struct.unpack_from('<7I',p,84+48*i) for i in range(5)]
        self.fix={}
        for s in self.sections:
            for at in range(s[1],s[2]-7,8):
                a,b=struct.unpack_from('<2I',p,s[0]+at)
                if a!=0xffffffff:self.fix[s[0]+a]=s[0]+b
            for at in range(s[2],s[3]-11,12):
                a,b,c=struct.unpack_from('<3I',p,s[0]+at)
                if a!=0xffffffff:self.fix[s[0]+a]=self.sections[b][0]+c
        self.classes={}
        s=self.sections[1]
        for at in range(0,s[1],8):
            sec,off=struct.unpack_from('<2I',p,s[0]+at);a=self.sections[sec][0]+off
            name=self.string(self.fix[a]);members={}
            if self.u(a+28):
                m=self.fix[a+24]
                for i in range(self.u(a+28)):
                    q=m+20*i;members[self.string(self.fix[q])]=struct.unpack_from('<BBHHH',p,q+12)
            self.classes[name]={'offset':a,'size':self.u(a+8),'members':members}
        self.root=self.sections[3][0];self.mesh=self.fix[self.root+12];self.code=self.fix[self.root+16]
        self.scaling=p[self.mesh+16:self.mesh+32];self.radius=p[self.mesh+48:self.mesh+52]
        count=self.u(self.mesh+40);parts=self.fix[self.mesh+36];storage=self.fix[self.mesh+64]
        assert count==self.u(self.mesh+68)
        for i in range(count):
            o=self.fix[storage+4*i];part=parts+48*i
            arrays=[]
            for j,stride in enumerate([4,2,4,1,4,2]):
                n=self.u(o+j*12+4);at=self.fix.get(o+j*12,0)
                arrays.append((at,n,p[at:at+n*stride]))
            nv=self.u(part+8);nt=self.u(part+24)
            assert arrays[0][1]==nv*3 and arrays[1][1]==nt*3
            assert not arrays[2][1] and not arrays[3][1] and arrays[5][1]==nt
            self.parts.append({'object':o,'part':part,'arrays':arrays,'vertices':nv,'triangles':nt})
        self.mopp=self.array(self.code+32,1);self.mopp_offset=p[self.code+16:self.code+32]
    def u(self,at):return struct.unpack_from('<I',self.p,at)[0]
    def string(self,at):return self.p[at:self.p.index(b'\0',at)].decode('ascii')
    def array(self,at,stride):
        n=self.u(at+4);b=self.fix.get(at,0);return self.p[b:b+n*stride]

def load(path):
    raw,entries=parse_archive(path)
    entry=next(e for e in entries if e['name'].endswith('.geohk'))
    return Collision(unpack_entry(raw,entry))

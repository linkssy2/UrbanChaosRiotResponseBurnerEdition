"""Integrate finite ragdoll forces against their existing 60-Hz envelope."""
from pathlib import Path
import hashlib
import re
import struct

INCLUDE='#include "xbe_ragdoll_force.h"\n'
ENTRY='\ttrial_ragdoll_force::Scope force_step(ctx,base);\n'
TAIL='\tif(trial_ragdoll_force::Tail(base,ctx.ecx))goto loc_002A5E31;\n'
PINS=(
 (71,0x2a33a0,92,'e12ac628e4a6d38ab837e4cfe831fb9e96bee5f55f4a6901abd5c65b9bd0d7b1','f8add46a1d755ebb80ba6546482daf9df0b6158bf5d2c01722638bdfba648c11'),
 (71,0x2a5da0,231,'16aee95dbd1452a1ee4e3505e9357e4064e832050b5bd074142c4fa10e33892e','b5d4db7e4bf64cade8852e25f65e0d289e88a920029ae8edc0b8c10ed50e905a'),
 (71,0x2a5c60,231,'a2846ca56fc75a9aa79551791c0a125627cc087380befcfb151230a9e9dadc34','1dddc6a6d354407ab6ab6d89dbb0db867d4f8d443f959b23514ca7a0a50d5fdb'),
 (82,0x2f4ff0,87,'ff86222783e610ad699f9afebe58617074437a68ce655f1ebb67d2b6a7947162','4c946fb2c96f9c9c49a38dd6dd63498b73cde45738618a848cb336473b56b326'),
)
def body(s,a):return re.search(rf'X86_FUNC\(__imp__sub_{a:08X}\) \{{.*?^\}}',s,re.M|re.S).group()
def clean(s):return s.replace(INCLUDE,'').replace(ENTRY,'').replace(TAIL,'')
def unprepare(area):
    p=Path(area)/'generated/game/x86_recomp.71.cpp'
    if p.exists():
        s=p.read_text();t=clean(s)
        if s!=t:p.write_text(t)
def prepare(area):
    area=Path(area);local=Path(__file__).parent
    x=(area/'assets/Default.xbe').read_bytes();base=struct.unpack_from('<I',x,0x104)[0]
    count,table=struct.unpack_from('<II',x,0x11c)
    sections=[struct.unpack_from('<6I',x,table-base+i*56)for i in range(count)]
    def raw(a,n):
        s=next(s for s in sections if s[1]<=a and a+n<=s[1]+s[4]);o=s[3]+a-s[1];return x[o:o+n]
    sources={u:clean((area/f'generated/game/x86_recomp.{u}.cpp').read_text())for u in (71,82)}
    originals={}
    for u,a,n,rh,sh in PINS:
        assert hashlib.sha256(raw(a,n)).hexdigest()==rh,hex(a)
        originals[a]=body(sources[u],a)
        assert hashlib.sha256(originals[a].encode()).hexdigest()==sh,hex(a)
    assert struct.unpack('<I',raw(0x48d9ec+8,4))[0]==0x2a33a0
    s=sources[71];anchor='X86_FUNC(__imp__sub_002A33A0) {\n';assert s.count(anchor)==1
    s=s.replace(anchor,anchor+ENTRY,1)
    # The second copy is the generated mid-function resume entry.
    anchor='\t// 002A5E08 ';assert s.count(anchor)==2;s=s.replace(anchor,TAIL+anchor)
    (area/'generated/game/x86_recomp.71.cpp').write_text(INCLUDE+s)
    (area/'src/common/xbe_ragdoll_force.h').write_text((local/'xbe_ragdoll_force.h').read_text())
    parts=[]
    for a,label in ((0x2a33a0,'Force'),(0x2a5da0,'Ragdoll')):
        for name,text in (('Original',originals[a]),('Fixed',body(s,a))):
            parts.append(text.replace(f'__imp__sub_{a:08X}',name+label,1))
    for a in (0x2a5c60,0x2f4ff0):parts.append(originals[a].replace('__imp__sub_', 'sub_',1))
    declarations=sorted(set(re.findall(r'\bsub_[0-9A-F]{8}(?=\(ctx)','\n'.join(parts))))
    fixture=(local/'test_xbe_ragdoll_force.cpp').read_text()
    fixture=fixture.replace('// NATIVE_DECLARATIONS','\n'.join('X86_FUNC('+n+');'for n in declarations))
    fixture=fixture.replace('// NATIVE_BODIES','\n'.join(parts))
    (area/'src/compat/test_ragdoll_force.cpp').write_text(fixture)

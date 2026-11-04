"""Advance the retail eyelid cycle at its original 30-Hz cadence."""
from pathlib import Path
import hashlib
import re
import struct

INCLUDE='#include "xbe_blink.h"\n'
ENTRY='\tunsigned blink_steps=trial_blink::Steps(ctx,base);\n'
SKIP='\tif(!blink_steps)goto loc_002B801A;\n'
REPEAT='\tif(blink_steps>1){--blink_steps;goto loc_002B7FF1;}\n'
DESTROY='\ttrial_blink::Forget(ctx.ecx);\n'
PINS=(
 (0x2b7f10,570,'082d72951969c6af5c4900377c628f633a649359d611a392bf3417375d3ed5c6','3d495912cd40b83e770a69814713aec6cd439b6fcd78712b635999ebbed3f4ad'),
 (0x2b5bd0,31,'a745090bdc5e917eff9a904484a0b733248b1f1f8acaa12687d4d172fd964306','156f03aaab7419138768c016f951fe60a3354759b281536e605094088e728cda'),
)
def body(s,a):return re.search(rf'X86_FUNC\(__imp__sub_{a:08X}\) \{{.*?^\}}',s,re.M|re.S).group()
def clean(s):
    for hook in (INCLUDE,ENTRY,SKIP,REPEAT,DESTROY):s=s.replace(hook,'')
    return s
def unprepare(area):
    p=Path(area)/'generated/game/x86_recomp.73.cpp'
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
    p=area/'generated/game/x86_recomp.73.cpp';s=clean(p.read_text());originals={}
    for a,n,rh,sh in PINS:
        assert hashlib.sha256(raw(a,n)).hexdigest()==rh,hex(a)
        originals[a]=body(s,a)
        assert hashlib.sha256(originals[a].encode()).hexdigest()==sh,hex(a)
    assert struct.unpack('<II',raw(0x48f1c4,8))==(0x2b5bd0,0x2b7f10)
    # B9830 forwards animation delta as argument one; world +8 advances once per update.
    for a,n,h in [(0xb9898,42,'a012b5fb91c05e96a852b0b9450f623abf8ae1227f7457e235b1a7c366e08a74'),
                  (0x21c34c,18,'3297018b93377abf72e16f7f6acdbd462a16c9e212c93cad4fc2b41a1ab3fdf5'),
                  (0x208f80,27,'c4644100eac27b429d68c1cef61f70704cc5b8b0ac7d7318bd1a56a27350631b')]:
        assert hashlib.sha256(raw(a,n)).hexdigest()==h,hex(a)
    assert raw(0x1f9010,9).hex()=='a1dcc45100d94014c3'
    for anchor,hook in [('X86_FUNC(__imp__sub_002B7F10) {\n',ENTRY),
                        ('loc_002B7FF1:\n',SKIP),('loc_002B801A:\n',REPEAT),
                        ('X86_FUNC(__imp__sub_002B5BD0) {\n',DESTROY)]:
        assert s.count(anchor)==1,anchor;s=s.replace(anchor,anchor+hook,1)
    p.write_text(INCLUDE+s)
    (area/'src/common/xbe_blink.h').write_text((local/'xbe_blink.h').read_text())
    parts=[originals[0x2b7f10].replace('__imp__sub_002B7F10','OriginalBlink',1),
           body(s,0x2b7f10).replace('__imp__sub_002B7F10','FixedBlink',1),
           body(s,0x2b5bd0).replace('__imp__sub_002B5BD0','DestroyBlink',1)]
    constants='\n'.join(f'Put(0x{a:x},0x{struct.unpack("<I",raw(a,4))[0]:x});'for a in
                         (0x44bf94,0x48f308,0x48f30c,0x48f310,0x48f314,0x48f318,0x48f31c))
    fixture=(local/'test_xbe_blink.cpp').read_text().replace('// NATIVE_BODIES','\n'.join(parts)).replace('// NATIVE_CONSTANTS',constants)
    (area/'src/compat/test_blink.cpp').write_text(fixture)

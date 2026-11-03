"""Optional held zoom: preserve the retail input and magnification transitions."""
from pathlib import Path
import hashlib
import re
import struct

INCLUDE='#include "xbe_aim_mode.h"\n'
PINS=(
    (0xbe700,0xc4,'4d527175b19d80a7d759efd7d2a268bd38e8ac451efd01c9a87f54a19deafa66'),
    (0xbe870,0xbf,'8d0b0c5f5087b5efbb82d123a981aa71ce8fc262b1a8a1c948b6b222224a2071'),
    (0xbee50,0x123,'849366b75b9097a70968abe48c201db58b865896c977535d7e5e9902fd223e18'),
    (0xd25b0,0x70,'54d44f21d4d0cb84e83f62c0149f21f7796e1cc0e9d5c97948369f1a16eeb0fd'),
    (0xd2ab0,0xf,'2c0a32f728d14663320296e782d1791e322bcbdd020db1223b276cf68f2e5c4d'),
    (0xd2ad0,0x1a,'ecfeda084421a126d3362a5fb0a3c1d7743fe9546202d822b279b95d8013bf66'),
    (0xd2d57,0x15,'765924ba0fa9b93068fe756a8d7b82236933889e8da8ba5a110393e884bacd5f'),
    (0x567f0,0x10c,'9b7896bfab6de1664f1cb67c77552ff07f90e08a9479ababe476a06d9d0b022f'),
    (0x56cc8,8,'4797eb13171ceecb4567f820f20d101f8bf636c4ee487b5612bc22e48c32a07c'),
)
HOOKS={
    19:(('X86_FUNC(__imp__sub_000BE870) {',
         'X86_FUNC(__imp__sub_000BE870) {\n\tTrialAimModeScope trial_aim_mode(ctx,base);'),
        ('loc_000BE700:\n','loc_000BE700:\n\ttrial_aim_mode.Cycle();\n'),
        ('\t// 000BE8A6 push 0x01\n\t{ uint32_t v = (uint32_t)(0x1u); X86_PUSH32(v); }',
         '\t// 000BE8A6 push 0x01\n\t{ uint32_t v = trial_aim_mode.QueryMode(); X86_PUSH32(v); }'),
        ('\t// 000BE8AE fld [0x0044C174]',
         '\ttrial_aim_mode.Filter(ctx);\n\t// 000BE8AE fld [0x0044C174]')),
    21:(('X86_FUNC(__imp__sub_000D2AB0) {',
         'X86_FUNC(__imp__sub_000D2AB0) {\n\tTrialAimUpdate(ctx,base);'),),
}

def clean(source,unit):
    source=source.replace(INCLUDE,'')
    for old,new in HOOKS.get(unit,()):
        if new in source:
            assert source.count(new)==1,(unit,old)
            source=source.replace(new,old,1)
    return source

def unprepare(area):
    for unit in HOOKS:
        p=Path(area)/f'generated/game/x86_recomp.{unit}.cpp'
        if p.exists():
            before=p.read_text();after=clean(before,unit)
            if after!=before:p.write_text(after)

def function(source,name):
    return re.search(r'X86_FUNC\(__imp__sub_'+name+r'\) \{.*?^\}',source,re.M|re.S).group()

def prepare(area):
    area=Path(area);local=Path(__file__).parent
    x=(area/'assets/Default.xbe').read_bytes();base=struct.unpack_from('<I',x,0x104)[0]
    count,table=struct.unpack_from('<II',x,0x11c)
    sections=[struct.unpack_from('<6I',x,table-base+i*56) for i in range(count)]
    for va,size,digest in PINS:
        sec=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4])
        at=sec[3]+va-sec[1]
        assert hashlib.sha256(x[at:at+size]).hexdigest()==digest,hex(va)
    originals={};patched={}
    for unit in HOOKS:
        p=area/f'generated/game/x86_recomp.{unit}.cpp'
        originals[unit]=s=clean(p.read_text(),unit)
        for old,new in HOOKS[unit]:
            assert s.count(old)==1,(unit,old)
            s=s.replace(old,new,1)
        patched[unit]=s
    native=function(originals[19],'000BE870')
    assert hashlib.sha256(native.encode()).hexdigest()=='9a786c5be78ea80f51a094fb501c3d9da6264a24e48db58f29bf144bcc9e8e14'
    derived=function(originals[19],'000BEE50')
    assert hashlib.sha256(derived.encode()).hexdigest()=='61f0f471d99e939bac18a6eb15e409d5b69e5f9134c728fbd2213f65ef1fe16b'
    query=(area/'generated/game/x86_recomp.7.cpp').read_text()
    a=query.index('X86_FUNC(__imp__sub_000567F0)');z=query.index('loc_000568FC:',a);query=query[a:z]
    assert hashlib.sha256(query.encode()).hexdigest()=='8ca86ecd5e0c8b9796e2e6c568c8c24f68cffaa69ebe7bd00b17cae18a9d1874'
    query+='loc_000568FC:\nloc_00056B9F:\nloc_00056C34:\n\tassert(false);\n}\n'
    parts=(query.replace('__imp__sub_000567F0','NativeQuery',1),
           native.replace('__imp__sub_000BE870','OriginalZoom',1),
           function(patched[19],'000BE870').replace('__imp__sub_000BE870','HeldZoom',1),
           derived.replace('__imp__sub_000BEE50','NativeCameraZoom',1))
    fixture=(local/'test_xbe_aim_mode.cpp').read_text();marker='// NATIVE_AIM_CONTRACTS'
    assert fixture.count(marker)==1
    for unit,s in patched.items():(area/f'generated/game/x86_recomp.{unit}.cpp').write_text(INCLUDE+s)
    for name in ('xbe_aim_mode.cpp','xbe_aim_mode.h'):(area/'src/common'/name).write_text((local/name).read_text())
    (area/'src/compat/test_aim_mode.cpp').write_text(fixture.replace(marker,'\n'.join(parts)))

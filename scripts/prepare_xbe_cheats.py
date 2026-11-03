"""Expose the original Xbox cheat actions to the validated in-game pause UI."""
from pathlib import Path
import hashlib
import struct

PINS=(
    (0x501728,200,'96df99adc077fb67129317aaf98dbff3ee84580c851eb5698e6a4032e7d1a138'),
    (0x643e0,6,'2f50be2683201136880faa73c01094369ebb3428808d2188c528381e7a624d6a'),
    (0x280e93,132,'88501d79308cd4b090dcb99e346c5d182b65af4510f24c3e12d8088db9c28b1f'),
    (0x28139e,102,'d3300aa056423d176c73978709d33e9d94592ef0d8319364f7a732b42c5c876e'),
    (0x280d00,125,'17aae2a9b179719a82bb300b695941269cbc6276e96a71f5895048d687932850'),
    (0x64230,71,'a212462cad730cd91b48edc61275faba9b6005fdc51da7ee76e0cfd56d36aecf'),
    (0x64310,197,'7ff768039eaccc9ce67096e057f7cdeda4bae7dc85cc800af11664e74ce46534'),
)
INCLUDE='#include "xbe_cheats_native.inc"\n'

def unprepare(area):
    path=Path(area)/'generated/game/x86_recomp.66.cpp'
    if path.exists():
        source=path.read_text();clean=source.replace(INCLUDE,'')
        if clean!=source:path.write_text(clean)

def prepare(area):
    area=Path(area);local=Path(__file__).parent;out=area/'src/common'
    image=(area/'assets/Default.xbe').read_bytes()
    base=struct.unpack_from('<I',image,0x104)[0]
    count,table=struct.unpack_from('<II',image,0x11c)
    sections=[struct.unpack_from('<6I',image,table-base+i*56) for i in range(count)]
    def read(va,size):
        section=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4])
        offset=section[3]+va-section[1];return image[offset:offset+size]
    for va,size,digest in PINS:
        assert hashlib.sha256(read(va,size)).hexdigest()==digest,hex(va)
    codes=('MINIFUN','KEYTOTHECITY','DANCINGFEET','BURNERSREVENGE','FRYINGTIME',
           'ZEROTOLERANCE','WHATWASTHAT','BURNINGBULLET','KEEPYOURHEAD','ULTIMATEPOWER')
    for i,code in enumerate(codes):
        pointer=struct.unpack('<I',read(0x501728+i*20,4))[0]
        assert read(pointer,len(code)+1)==code.encode()+b'\0',code
    path=area/'generated/game/x86_recomp.66.cpp';source=path.read_text().replace(INCLUDE,'')
    def fragment(start,end,digest):
        a=source.index('\t// '+start+' ')
        marker='loc_'+end+':' if end in ('00280F17','00281404') else '\t// '+end+' '
        z=source.index(marker,a);text=source[a:z]
        assert hashlib.sha256(text.encode()).hexdigest()==digest,start
        return text
    activate=fragment('00280EA9','00280F17','b02e9c81281a6b392608c0f47aa447b892bdba4f0db25fca3eaed59fe9171e9b')
    toggle=fragment('0028139E','00281404','61b085481877a0f6058e817b6823ef9cc18188d79067711d5d49a62700e515a8')
    prefix='\tX86_LOAD_FLAGS();\n\tuint32_t ca=0,cb=0;(void)ca;(void)cb;\n'
    native=('// Exact retail activation/toggle instructions; generated, not a second cheat implementation.\n'
            'void TrialCheatsNativeToggle(X86Context& ctx,uint8_t* base,unsigned index,bool discover) {\n'+prefix+
            '\tif(discover){ctx.esi=index*20;ctx.edi=index;ctx.ebp=ctx.esp+16;\n'+activate+
            'loc_00280F17:\n\t\treturn;\n\t}\n\tctx.esi=index*5;\n'+toggle+
            'loc_00281404:\n\treturn;\n}\n')
    (out/'xbe_cheats_native.inc').write_text(native)
    path.write_text(source+INCLUDE)
    for name in ('xbe_cheats.cpp','xbe_cheats.h'):(out/name).write_text((local/name).read_text())
    # The fixture executes the production action, original reset/permission
    # paths and independent retail activation/toggle excerpts against RAM.
    pieces=[]
    for unit,name,new,digest in (
        (9,'000643E0','sub_000643E0','65b95e293f8c3716565c1374654c761393adda622e455e39302fc425780aa031'),
        (66,'00280D00','OriginalReset','64f5aed773446d00ca9f8336d2bf6970534dca0a2744832b5367931fcc1bbdda')):
        text=(area/f'generated/game/x86_recomp.{unit}.cpp').read_text()
        a=text.index('X86_FUNC(__imp__sub_'+name+') {');z=text.index('\nX86_WEAK_FUNC',a)
        body=text[a:z];assert hashlib.sha256(body.encode()).hexdigest()==digest,name
        pieces.append(body.replace('__imp__sub_'+name,new,1))
    a=source.index('\t// 00280E93 ');z=source.index('loc_00280EA9:',a);gate=source[a:z]
    assert hashlib.sha256(gate.encode()).hexdigest()=='8270d2e6e356a21a0db8c9e23a80373137a0acc1ec38e9bd2942af12d7a94fd9'
    pieces.append('X86_FUNC(OriginalPermission) {\n'+prefix+gate+
                  'loc_00280EA9:\n\tctx.eax=1;return;\nloc_00280F0A:\n\tctx.eax=0;return;\n}\n')
    pieces.append('X86_FUNC(OriginalActivate) {\n'+prefix+activate+'loc_00280F17:\n\treturn;\n}\n')
    pieces.append('X86_FUNC(OriginalToggle) {\n'+prefix+toggle+'loc_00281404:\n\treturn;\n}\n')
    raw=read(0x501728,200)
    pieces.append('static const uint8_t NativeTable[200]={'+','.join(str(v) for v in raw)+'};\n')
    test=(local/'test_xbe_cheats.cpp').read_text();marker='// NATIVE_CHEAT_CONTRACTS'
    assert test.count(marker)==1
    (area/'src/compat/test_cheats.cpp').write_text(test.replace(marker,'\n'.join(pieces)))

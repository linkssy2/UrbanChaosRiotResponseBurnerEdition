"""Keep original level details, without outgoing copies or text motion trails."""
from pathlib import Path
import hashlib
import struct


def body(text,address):
    begin=f'X86_FUNC(__imp__sub_{address}) {{'
    start=text.index(begin)
    return text[start:text.index(f'\nX86_WEAK_FUNC(sub_{address}',start)]


def prepare(area):
    area=Path(area);local=Path(__file__).parent
    xbe=(area/'assets/Default.xbe').read_bytes()
    base=struct.unpack_from('<I',xbe,0x104)[0]
    count,table=struct.unpack_from('<II',xbe,0x11c)
    sections=[struct.unpack_from('<6I',xbe,table-base+i*56) for i in range(count)]
    def image(va,size):
        s=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4])
        at=s[3]+va-s[1];return xbe[at:at+size]
    for va,size,digest in [
        (0x274320,132,'3f091881ed5e2d0e964d634baaabc3972be6ad83f6dd22c5b46ea7cf74b5f2e2'),
        (0x273730,292,'954725a2c2ff4378c0c9dd1d390eee521ff015ce0c81bca81b5d6e73084b2100'),
        (0x273860,281,'5e6001ceef33b5965ba6b0459efbd1ab10b54fcbf8d3bd60863516f5ee1ded72'),
        (0x2739a0,343,'81d5e3ce97cffd7ba695c5da6c17ce9cde0153d7dc3baa48a95c57ec90d27884'),
        (0x17730,14,'de71b679d61acf9c519bdc01dfd2247889ae3c656d94a4a4efc7c9b9b5fb1e10'),
    ]:assert hashlib.sha256(image(va,size)).hexdigest()==digest,hex(va)
    assert image(0x2704d3,24)==bytes.fromhex('8d8ef400000055e8413e00008d8e0001000055e8353e0000')
    for va,fn in [(0x489060,0x273860),(0x452f20,0x2739a0)]:
        assert image(va,4)==struct.pack('<I',fn)
    chunk=area/'generated/game/x86_recomp.65.cpp';text=chunk.read_text()
    old='uint32_t t = X86_MEM32((uint32_t)(ctx.edx + 0x4u)); X86_PUSH32(0x274398u);'
    new='uint32_t t = TrialLevelDetailTarget(ctx, base, X86_MEM32((uint32_t)(ctx.edx + 0x4u))); X86_PUSH32(0x274398u);'
    assert text.count(old)+text.count(new)==1
    text=text.replace(old,new)
    declaration='extern uint32_t TrialLevelDetailTarget(const X86Context&,uint8_t*,uint32_t);'
    if declaration not in text:text=text.replace('#include "x86_recomp_shared.h"','#include "x86_recomp_shared.h"\n'+declaration,1)
    chunk.write_text(text)
    font_path=area/'generated/game/x86_recomp.0.cpp';font=font_path.read_text()
    declaration='extern void TrialLevelDetailBlur(X86Context&,uint8_t*);'
    if declaration not in font:font=font.replace('#include "x86_recomp_shared.h"','#include "x86_recomp_shared.h"\n'+declaration,1)
    marker='X86_FUNC(__imp__sub_00017730) {\n'
    hook='\tTrialLevelDetailBlur(ctx, base);\n'
    assert font.count(marker)==1
    if marker+hook not in font:font=font.replace(marker,marker+hook,1)
    font_path.write_text(font)
    for name in ('xbe_level_details.h','xbe_level_details.cpp'):
        (area/'src/common'/name).write_text((local/name).read_text())
    generated=''
    for address in ('00273730','00273860','002739A0','00274320'):
        generated+=body(text,address).replace('__imp__sub_'+address,'sub_'+address,1)+'\n'
    generated=body(font,'00017730').replace('__imp__sub_00017730','sub_00017730',1)+'\n'+generated
    fixture=(local/'test_xbe_level_details.cpp').read_text()
    constants='\n'.join(f'W32(0x{a:x},0x{struct.unpack("<I",image(a,4))[0]:x});'
                         for a in [0x44c174,0x44bf94,0x453014,0x44c72c,0x4e8394,0x4505cc,0x44c1f8])
    for marker,value in [('// GENERATED_ORIGINAL',generated),('// GENERATED_CONSTANTS',constants)]:
        assert fixture.count(marker)==1
        fixture=fixture.replace(marker,value,1)
    (area/'src/compat/test_level_details.cpp').write_text(fixture)

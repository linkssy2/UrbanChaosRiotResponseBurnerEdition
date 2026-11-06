"""Replace native shield-block feedback only while the corpse shield is rendered."""
from pathlib import Path
import hashlib
import re
import struct

INCLUDE='#include "xbe_corpse_shield_impact.h"\n'
PINS=(
 (19,0xbc080,31,'51c00a3b08a6f5cea771a3def288acfe5f8d3a2cdc03b7b418cba0ed216e9887','f2e98c5c4727c8737da63770d30ee337a754bf46b255704fc51048d520101410'),
 (19,0xbe6b0,29,'042552c6e73ac5739755a652fdf300baeb78736f304cc545e07b022ef2f47144','eddb6cf406e9c504aa9819cc458e71226186517c3bf53d5f3077df9204958e1d'),
 (20,0xc8e90,5311,'af55457b5bf944197925e9bd018ce9d014a8ceede506df8e3cadee81578074ba','1557f0f6298496f700cb7108f41c45f5489f0c119a4927132beef1e4a12e482a'),
 (23,0xeb150,896,'8f20e6ad3620cf10c8d024e10fe5a7f99e086add87762dcb4c882af5f6ce20a4','90389f249e09c0309868a937280000fb8592c357a03c9c7f9e8f9758d6e70dcc'),
 (8,0x5e780,479,'ef4e9611944a2f667c1f37db6af34f6e9d08a6fd6fcdcfe38422fa7c3ba4cfe8','80cbb270702f609123f0fa6075c3b42590cf4b7b928d4f925a60cb007c645af5'),
 (52,0x20a230,46,'46ed0f6bb09ef6c4111f864d06722430432e556ee4661018ac7a54c8c2ef7f9c','b270c258e71a3367d2348f1ed6aeae118045d69be0e2467197f9c47e8750bec4'),
)

def body(source,address):
    return re.search(rf'X86_FUNC\(__imp__sub_{address:08X}\) \{{.*?^\}}',source,re.M|re.S)[0]

def replacements(unit=20):
    if unit==19:
        old='\t// 000BC080 mov eax, [ecx+0x18]\n\t{ uint32_t ea = (uint32_t)(ctx.ecx + 0x18u); ctx.eax = (uint32_t)(X86_MEM32(ea)); }'
        yield old,old.replace('(uint32_t)(X86_MEM32(ea))','TrialCorpseShieldDecalCount(base,ctx.ecx,X86_MEM32(ea))')
        return
    for address,original in ((0xc9233,0x4587ac),(0xc9241,0x458798),(0xc9cf6,0x4587ac)):
        old=f'\t// {address:08X} push 0x{original:X}\n\t{{ uint32_t v = (uint32_t)(0x{original:X}u); X86_PUSH32(v); }}'
        new=old.replace(f'(uint32_t)(0x{original:X}u)',f'TrialCorpseShieldSound(base,ctx.esi,0x{original:X}u)')
        yield old,new
    for ret in (0xc9407,0xc9ec0):
        old=f'\tX86_PUSH32(0x{ret:X}u); sub_0020A230(ctx, base);'
        yield old,f'\tif(!TrialCorpseShieldImpact(ctx,base)){{X86_PUSH32(0x{ret:X}u); sub_0020A230(ctx, base);}}'

def clean(source):
    source=source.replace(INCLUDE,'')
    for unit in (19,20):
        for old,new in replacements(unit):source=source.replace(new,old)
    return source

def patch(source,unit=20):
    source=clean(source);result=source
    for old,new in replacements(unit):
        assert result.count(old)==1,old
        result=result.replace(old,new,1)
    result=INCLUDE+result
    assert clean(result)==source
    return result

def unprepare(area):
    for unit in (19,20):
        path=Path(area)/f'generated/game/x86_recomp.{unit}.cpp'
        if path.exists():
            before=path.read_text();after=clean(before)
            if before!=after:path.write_text(after)

def prepare(area):
    area=Path(area);local=Path(__file__).parent
    image=(area/'assets/Default.xbe').read_bytes();base=struct.unpack_from('<I',image,0x104)[0]
    count,table=struct.unpack_from('<II',image,0x11c)
    sections=[struct.unpack_from('<6I',image,table-base+i*56)for i in range(count)]
    def raw(address,size):
        section=next(s for s in sections if s[1]<=address and address+size<=s[1]+s[4])
        offset=section[3]+address-section[1];return image[offset:offset+size]
    sources={};originals={}
    for unit,address,size,raw_hash,source_hash in PINS:
        source=clean((area/f'generated/game/x86_recomp.{unit}.cpp').read_text())
        original=body(source,address)
        assert hashlib.sha256(raw(address,size)).hexdigest()==raw_hash,hex(address)
        assert hashlib.sha256(original.encode()).hexdigest()==source_hash,hex(address)
        sources[unit]=source;originals[address]=original
    assert raw(0x45f69c,6)==b'Flesh\0'
    assert raw(0x457b8c,16)==bytes.fromhex('90d30b00c0c20b0080c00b00a0c00b00')
    assert raw(0xd45c6,6)==bytes.fromhex('89bea4020000')
    assert raw(0x456038,4)==bytes.fromhex('30ad0900')
    assert raw(0x9ad8c,30)==bytes.fromhex('8d442438508d442440508d44242850ff52088b44242085c00f842a030000')
    assert raw(0x4fa5c4,20)==bytes.fromhex('09000000bcf6450044be51003ca84f0000000000')
    assert raw(0x4fa83c,16)==bytes.fromhex('44fb450044fb450018fb450000000000')
    fixture=(local/'test_xbe_corpse_shield_impact.cpp').read_text()
    marker='// NATIVE_CORPSE_SHIELD_IMPACT_CONTRACTS'
    assert fixture.count(marker)==1
    old,new=next(replacements(19))
    originals[0xbc080]=originals[0xbc080].replace(old,new,1)
    fixture=fixture.replace(marker,'\n'.join(originals[a].replace('__imp__sub_','sub_',1)for a in (0x5e780,0xeb150,0xbc080)))
    marker='// NATIVE_CORPSE_SHIELD_IMPACT_DATA'
    assert fixture.count(marker)==1
    ranges=((0x4fa510,620),(0x4fa83c,32),(0x45fb18,0xbe),(0x44bf94,4),(0x44c174,4))
    data=[]
    for address,size in ranges:
        values=','.join(str(v)for v in raw(address,size))
        data.append(f'{{const uint8_t values[]={{ {values} }};std::memcpy(B()+0x{address:x},values,sizeof values);}}')
    fixture=fixture.replace(marker,'\n'.join(data))
    results={unit:patch(sources[unit],unit)for unit in (19,20)}
    for name in ('xbe_corpse_shield_impact.h','xbe_corpse_shield_impact.cpp'):
        (area/'src/common'/name).write_text((local/name).read_text())
    (area/'src/compat/test_corpse_shield_impact.cpp').write_text(fixture)
    for unit,result in results.items():
        (area/f'generated/game/x86_recomp.{unit}.cpp').write_text(result)

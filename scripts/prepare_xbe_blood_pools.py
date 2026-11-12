"""Clip blood pools to supporting ground and retry missed ground placement."""
from pathlib import Path
import hashlib
import struct
import re

INCLUDE = '#include "xbe_blood_pools.h"\n'

def replacements(unit):
    if unit == 16:
        old='X86_FUNC(__imp__sub_0009B8A0) {\n'
        yield old,old+'\tTrialClearBloodSupport();\n'
        old='\t// 0009BE49 pop esi\n'
        yield old,'\tTrialBloodSupport(ctx,base);\n'+old
        old='loc_0009C21F:\n'
        yield old,old+'\tif(TrialBloodVertex(ctx,base))goto loc_0009C24D;\n'
        old='\t// 0009C24D inc esi\n'
        yield old,'loc_0009C24D:\n'+old
        old='X86_FUNC(__imp__sub_0009BA60) {\n'
        yield old,old+'\tTrialGrowDecalRing(ctx,base);\n'
        old='\t// 0009BE4E ret 0x10\n'
        yield old,'\tTrialDecalSubmitted();\n'+old
    if unit == 7:
        yield 'X86_FUNC(__imp__sub_00056D20) {\n', 'X86_FUNC(__imp__sub_00056D20) {\n\tconst uint64_t trial_pool_start=TrialDecalSubmissions();\n'
        old='\tconst uint64_t trial_pool_start=TrialDecalSubmissions();\n'
        yield old,old+'\tTrialBloodPoolScope trial_blood_scope;\n'
        old='\t// 00056EC9 push 0x03\n\t{ uint32_t v = (uint32_t)(0x3u); X86_PUSH32(v); }'
        yield old,old.replace('(0x3u)', '(0x7u)')
        old='\t// 00056F53 ret 0x0C\n\tctx.esp += 16; return;'
        yield old,'\tctx.eax=TrialDecalSubmissions()!=trial_pool_start?1u:0u;\n'+old
    if unit == 43:
        old='\t// 001B5736 mov byte ptr [esi+0xF0], 0x01\n\t{ uint32_t ea = (uint32_t)(ctx.esi + 0xF0u); X86_MEM8(ea) = (uint8_t)(0x1u); }'
        yield old,'\t// Mark the one-shot only after ground placement succeeds.'
        old='\tX86_PUSH32(0x1B5768u); sub_00056D20(ctx, base);'
        yield old,old+'\n\tif(ctx.eax)X86_MEM8(ctx.esi+0xF0u)=1;'

def clean(source,unit):
    source=source.replace(INCLUDE,'')
    for old,new in reversed(list(replacements(unit))):source=source.replace(new,old)
    return source

def patch(source,unit):
    source=clean(source,unit);original=source
    for old,new in replacements(unit):
        expected=2 if old in ('loc_0009C21F:\n','\t// 0009C24D inc esi\n') else 1
        assert source.count(old)==expected,old
        source=source.replace(old,new,expected)
    if unit in (7,16):source=INCLUDE+source
    assert clean(source,unit)==original
    return source

def unprepare(area):
    for unit in (7,16,43):
        p=Path(area)/f'generated/game/x86_recomp.{unit}.cpp'
        before=p.read_text();after=clean(before,unit)
        if before!=after:p.write_text(after)

def prepare(area):
    verify(area)
    source=clean((Path(area)/'generated/game/x86_recomp.43.cpp').read_text(),43)
    bodies=[]
    for name,text in (('Original',source),('Candidate',patch(source,43))):
        body=re.search(r'X86_FUNC\(__imp__sub_001B5700\) \{.*?^}',text,re.M|re.S)[0]
        body=body[:body.index('loc_001B5768:')]+'loc_001B5768: return;\n}'
        bodies.append(body.replace('__imp__sub_001B5700',name,1))
    template=(Path(__file__).parent/'test_xbe_blood_creation.cpp').read_text()
    placement=patch((Path(area)/'generated/game/x86_recomp.7.cpp').read_text(),7)
    placement=re.search(r'X86_FUNC\(__imp__sub_00056D20\) \{.*?^}',placement,re.M|re.S)[0].replace('__imp__sub_00056D20','PlacePool',1)
    (Path(area)/'src/compat/test_blood_creation.cpp').write_text(template.replace('// BLOOD_CREATION_BODIES','\n'.join(bodies)).replace('// BLOOD_PLACEMENT_BODY',placement))
    for unit in (7,16,43):
        p=Path(area)/f'generated/game/x86_recomp.{unit}.cpp'
        p.write_text(patch(p.read_text(),unit))
    for name in ('xbe_blood_pools.h','xbe_blood_pools.cpp','xbe_blood_clip.h'):
        (Path(area)/'src/common'/name).write_text((Path(__file__).parent/name).read_text())
    (Path(area)/'src/compat/test_blood_pools.cpp').write_text((Path(__file__).parent/'test_xbe_blood_pools.cpp').read_text())

def verify(area):
    xbe=(Path(area)/'assets/Default.xbe').read_bytes()
    base=struct.unpack_from('<I',xbe,0x104)[0]
    count,table=struct.unpack_from('<II',xbe,0x11c)
    sections=[struct.unpack_from('<6I',xbe,table-base+i*56) for i in range(count)]
    for address,size,digest in PINS:
        section=next(s for s in sections if s[1]<=address and address+size<=s[1]+s[4])
        offset=section[3]+address-section[1]
        assert hashlib.sha256(xbe[offset:offset+size]).hexdigest()==digest,hex(address)

PINS=((355616, 564, 'b32617e03e36650838702c5e2d6a7d09febac5abe5707bb91ebc7ff185a73f4f'), (1791744, 105, 'b260f667da1173a5bc3c13771453dedb969c8db82381fd5e9dd982fcb190831f'), (637536, 95, '1e7e3109313a983fbd1d628eaa699e3f1d4f4e6615fd16048b573724a947a67d'), (638592, 198, '262ebb947e3658317c6114237131517b6978a809405755907352cd61c37d17a9'), (637088, 128, '198a8d064781ecc5bce62ab7165c0e62bb41d367f12ce5de10279d50e0a539c3'))

PINS += ((637872, 673, '8808bfc5d6f9da5c1a2a90b6b9eb5d3cb945c734d8dff983bb04874c2411c4fe'),)

PINS += ((638800, 818, '0bfcfe8b088131d50dfadb14786b4278e520c939031a85b8af969d981ecb0682'),)

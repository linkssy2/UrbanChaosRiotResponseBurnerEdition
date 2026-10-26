"""Keep the retail spin detector and killcam, with each co-op player's own latch."""
from pathlib import Path
import hashlib
import struct

INCLUDE='#include "xbe_coop_spin.h"\n'
REGIONS=(
    (20,0xcc319,0xcc31b,'ActorOwner','57ff03e26abab83cf46384bfae912a4b8a5efe37fac2dd2623e4199584f948d0','f939e6f2642c9a74c13d6612f0b78989a3b2e06db21fed7147e697b52cbc3eb9'),
    (20,0xc6617,0xc664b,'Grant','e776d2a3830f7248cbad432db50a910f5dc8bfd833656bb650af7a99dd62c657','be43393ee177a993a87e2fa12bfbe3c73920f92a4b4963ff7d3791a1347c44ec'),
    (20,0xc664b,0xc665b,'Option','4b318e32b0a2a9951f073408a3bb7fbe735de65d1078aa88115dc4aaf5e31dbf','78341c92348ab2fb2ec00eea2c505c9934780dc799f75bd6fa6f21a256c628fc'),
    (11,0x739a1,0x739f9,'Hud','6004b3d2ac1bba38b92d54e483c72acdc8b68cd22118127c0da0ff4ff48de9f6','d50cea3ed89365fc11322fac6c9daab1f5336f9040abfd28d9ee50daa98018af'),
    (4,0x38823,0x38838,'Reward','55c2891cc6e1cc5ef24f3a28cc74753b1b45fb63269f51b765f90513bbf738f4','5a8f1f992fff1fc2eaf13ca80bc170377348f4a4d4e44a34eb8ee6527badcffe'),
    (20,0xcc404,0xcc40b,'ActorReset','9acd00fd6f8446b4c14c732240d1238a870c0713f5f51a6ec6e0c36bcb634d2a','266ecd1a211aa6ca3d4f2234134a39defc2a3c770a4ed44ef38bfe01a30ef474'),
)
LOAD='{ uint32_t ea = (uint32_t)(0x51BDF4u); ctx.eax = (ctx.eax & 0xFFFFFF00u) | (uint8_t)(X86_MEM8(ea)); }'
STORE='{ uint32_t ea = (uint32_t)(0x51BDF4u); X86_MEM8(ea) = (uint8_t)('
REPLACEMENTS={
    20:(
        (0xc6617,STORE+'0x1u); }','TrialCoopSpinWrite(base,ctx.esi,1);'),
        (0xc6655,STORE+'((uint8_t)ctx.ebx)); }','TrialCoopSpinWrite(base,ctx.esi,uint8_t(ctx.ebx));'),
        (0xcc404,STORE+'0x0u); }','TrialCoopSpinActorReset(base,ctx.ebx);\n\t'+STORE+'0x0u); }'),
    ),
    11:((0x739a1,LOAD,'ctx.eax=(ctx.eax&0xffffff00u)|TrialCoopSpinHud(base,ctx.esi);'),),
    4:(
        (0x38823,LOAD,'ctx.eax=(ctx.eax&0xffffff00u)|TrialCoopSpinReward(base,ctx.esi,ctx.edi);'),
        (0x38831,STORE+'0x0u); }','TrialCoopSpinConsume(base,ctx.esi,ctx.edi);'),
    ),
}

def replace(source,address,old,new):
    anchor=f'\t// {address:08X} '
    start=source.index(anchor);line=source.index('\n',start)+1
    assert source[line:].startswith('\t'+old),hex(address)
    return source[:line]+source[line:].replace('\t'+old,'\t'+new,1)

def clean(source,unit):
    source=source.replace(INCLUDE,'')
    for address,old,new in REPLACEMENTS[unit]:
        if '\t'+new not in source:continue
        source=replace(source,address,new,old)
    return source

def unprepare(area):
    for unit in REPLACEMENTS:
        p=Path(area)/f'generated/game/x86_recomp.{unit}.cpp'
        if p.exists():
            before=p.read_text();after=clean(before,unit)
            if after!=before:p.write_text(after)

def prepare(area):
    area=Path(area);local=Path(__file__).parent
    xbe=(area/'assets/Default.xbe').read_bytes();base=struct.unpack_from('<I',xbe,0x104)[0]
    count,table=struct.unpack_from('<II',xbe,0x11c)
    sections=[struct.unpack_from('<6I',xbe,table-base+i*56)for i in range(count)]
    sources={u:clean((area/f'generated/game/x86_recomp.{u}.cpp').read_text(),u)for u in REPLACEMENTS}
    fixtures=[]
    for unit,first,last,name,raw,lifted in REGIONS:
        sec=next(s for s in sections if s[1]<=first and last<=s[1]+s[4]);offset=sec[3]+first-sec[1]
        assert hashlib.sha256(xbe[offset:offset+last-first]).hexdigest()==raw,name
        source=sources[unit];a=source.index(f'\t// {first:08X} ');z=source.index(f'\t// {last:08X} ',a)
        part=source[a:z];assert hashlib.sha256(part.encode()).hexdigest()==lifted,name
        for fixed in (False,True):
            body=part
            if fixed:
                for address,old,new in REPLACEMENTS[unit]:
                    if first<=address<last:body=replace(body,address,old,new)
            fixtures.append('X86_FUNC('+('Coop'if fixed else'Original')+name+') {\n\tX86_LOAD_FLAGS();\n\tuint32_t ca=0,cb=0;\n'+body+'\t;\n}\n')
    for unit,source in sources.items():
        for address,old,new in REPLACEMENTS[unit]:source=replace(source,address,old,new)
        source=source.replace('#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+INCLUDE,1)
        (area/f'generated/game/x86_recomp.{unit}.cpp').write_text(source)
    for name in ('xbe_coop_spin.cpp','xbe_coop_spin.h'):
        (area/'src/common'/name).write_text((local/name).read_text())
    test=(local/'test_xbe_coop_spin.cpp').read_text()
    marker='// ORIGINAL_AND_COOP_SPIN_REGIONS';assert test.count(marker)==1
    (area/'src/compat/test_coop_spin.cpp').write_text(test.replace(marker,'\n'.join(fixtures)))

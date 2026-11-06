"""Select each local co-op player once in the three original hazard loops."""
from pathlib import Path
import hashlib
import struct

INCLUDE='#include "xbe_coop_hazards.h"\n'
HOOK='\tTrialCoopHazardPlayer(ctx,base);\n'
MEDIC='\tTrialHealthPackMedicPlayer(ctx,base,false);\n'
PINS=(
    (0x4acd0,9,'3d88229ba8f81117aa293b673da37176d12ed5fdadb9eeb56bfda6da97e01c38'),
    (0x4acc0,4,'74032ad0e931f3c4730df99da43479f9f5315cc898c7c8614278631cb2c16a62'),
    (0x4a780,42,'7ed0295eacfc935c7d62322f41d5632b89ef70ce8f7bade3dc378a31f734922a'),
    (0x4a840,6,'e51598aae247076e5665f4a60dfcf3e4ed9a470afbcf020165528094ba6f6c58'),
    (0x18ef10,533,'c414f79520be131199b0e127d847e016f56791f84077756ebe99aaa4917d0c66'),
    (0x191210,402,'d5717dc0152c3081e665c84fbb6cac7e3425b39ac0e27005983993c098413100'),
    (0x1a5c60,586,'c1194a721f4d0580298a7e20112a89d5a3e5032fbf9b1d915843f680cfa7ca8e'),
    (0x2180d0,41,'07d4906bba6bb3743f6aa845ff231ebda1708fe5eda55239798ca936a14d40c7'),
    (0xc96e9,58,'00c9622bce7980bb9cb7719eccd2ff793718afc845cb20450ce0c3530ed0cb57'),
    (0xc97ea,72,'3f7652fc0975af97ee974ed544bf1150059bbe2100d266324c234856abb8b77a'),
    (0xd2c3b,55,'3ae02ae9f1694e478ed98a2158dcdcfa54622c8817532f91cb78f82a5cb8632f'),
)
BODIES=(
    (6,0x4a780,'68f5aefbda65aeea9fd3d9db3518b810f2fcf1cd5ee689b930bc7249bbefaa0e'),
    (6,0x4a840,'77e02a84134be3bac2bfcea70562957f6be493f348198ff2ef8a30e72d891f4b'),
    (6,0x4acc0,'aede657579ca6b80241462a94cf708b82a041bfc6bc0a894f94ce6f2645240ee'),
    (6,0x4acd0,'5e658c29e77f331c045498dba19928e7629042aec91a0cc55482940bbc4e1a05'),
    (40,0x18ef10,'8bdcd58758e6873c798ae08cc2c56ea27ceb95e3d2a50e20b64cc21c23fbaab8'),
    (40,0x191210,'2e619ef1a8166d56cc140ad739b0d3e9d04bfe7c11fba832881441c66e170fac'),
    (42,0x1a5c60,'2c773362961584df2fac40b481fb37f63b401a01fd3f09b8c5eb7c891a01b6de'),
    (54,0x2180d0,'1779dbfcfe055e358dc9455f29ccd25469cf76711e072862e3b0c6651cac8f4c'),
)

def unprepare(area):
    # Run before every other preparer that pins the shared retail getter.
    path=Path(area)/'generated/game/x86_recomp.6.cpp'
    if path.exists():
        before=path.read_text();after=before.replace(INCLUDE,'').replace(HOOK,'')
        if before!=after:path.write_text(after)

def prepare(area):
    area=Path(area);local=Path(__file__).parent
    raw=(area/'assets/Default.xbe').read_bytes()
    base=struct.unpack_from('<I',raw,0x104)[0]
    count,table=struct.unpack_from('<II',raw,0x11c)
    sections=[struct.unpack_from('<6I',raw,table-base+i*56) for i in range(count)]
    def data(va,size):
        section=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4])
        offset=section[3]+va-section[1]
        return raw[offset:offset+size]
    for va,size,digest in PINS:
        assert hashlib.sha256(data(va,size)).hexdigest()==digest,hex(va)
    sources={unit:(area/f'generated/game/x86_recomp.{unit}.cpp').read_text()
             for unit in (6,20,21,40,42,54)}
    source=sources[6].replace(INCLUDE,'').replace(HOOK,'')
    canonical=dict(sources);canonical[6]=source.replace(MEDIC,'')
    bodies=[]
    for unit,va,digest in BODIES:
        text=canonical[unit];anchor=f'X86_FUNC(__imp__sub_{va:08X}) {{\n'
        assert text.count(anchor)==1
        first=text.index(anchor);last=text.index('\nX86_WEAK_FUNC',first)
        body=text[first:last]
        assert hashlib.sha256(body.encode()).hexdigest()==digest,hex(va)
        body=body.replace(f'__imp__sub_{va:08X}',f'sub_{va:08X}',1)
        if va==0x4acd0:
            body=body.replace('\t// 0004ACD6 ',
                '\tif(hazards_test::repair)TrialCoopHazardPlayer(ctx,base);\n'
                '\thazards_test::Lookup(ctx);\n\t// 0004ACD6 ',1)
        bodies.append(body)
    ranges=[]
    for unit,first,last,digest in (
        (20,0xc96e9,0xc9723,'c69c803eefada6b535e762700c3ebfc93b4d034da1da60af4ae4893e7324432e'),
        (20,0xc97ea,0xc9832,'ddeb83ab16cee5fd9240e9f8111571b045f7e70dca3ebdb513c106846e031143'),
        (21,0xd2c3b,0xd2c72,'03b4d5fcc01171bf5d966390e84105db58e7eaadf37b50bc5941a5014409c97c'),
    ):
        text=sources[unit];a=text.index(f'\t// {first:08X} ');z=text.index(f'\t// {last:08X} ',a)
        part=text[a:z];assert hashlib.sha256(part.encode()).hexdigest()==digest,hex(first)
        ranges.append(part)
    prologue='X86_LOAD_FLAGS(); uint32_t ca=0,cb=0;\n'
    bodies.append('void OriginalBurnCadence(X86Context& ctx,uint8_t* base){\n'+prologue+
                  ranges[0]+'return;\nloc_000C97EA:\n'+ranges[1]+'return;\n}\n')
    bodies.append('void OriginalBurnAge(X86Context& ctx,uint8_t* base){\n'+prologue+ranges[2]+'}\n')
    test=(local/'test_xbe_coop_hazards.cpp').read_text()
    assert test.count('// ORIGINAL_COOP_HAZARD_BODIES')==1
    test=test.replace('// ORIGINAL_COOP_HAZARD_BODIES','\n'.join(bodies))
    # The fixture uses retail constants, including the different fire/sphere
    # second sample heights and electricity's authored instant-kill amount.
    constants=[]
    for va in (0x44c6c4,0x44bf94,0x458d74,0x44db94,0x44c178,0x44c47c,0x44c174):
        constants.append(f'Put(0x{va:x},0x{struct.unpack("<I",data(va,4))[0]:08x});')
    assert test.count('// ORIGINAL_COOP_HAZARD_CONSTANTS')==1
    test=test.replace('// ORIGINAL_COOP_HAZARD_CONSTANTS','\n'.join(constants))
    marker='\t// 0004ACD6 ret 0x04'
    assert source.count(marker)==1
    # Keep the earlier medic override, then qualify only these hazard callers.
    changed=source.replace(marker,HOOK+marker,1)
    assert changed.replace(HOOK,'')==source
    changed=changed.replace('#include "x86_recomp_shared.h"\n',
                            '#include "x86_recomp_shared.h"\n'+INCLUDE,1)
    (area/'generated/game/x86_recomp.6.cpp').write_text(changed)
    for name in ('xbe_coop_hazards.h','xbe_coop_hazards.cpp'):
        (area/'src/common'/name).write_text((local/name).read_text())
    (area/'src/compat/test_coop_hazards.cpp').write_text(test)

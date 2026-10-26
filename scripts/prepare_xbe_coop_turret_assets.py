"""Load only the current campaign's gun-free helicopter alias at its native callback."""
from pathlib import Path
import hashlib
import re
import struct

INCLUDE = '#include "xbe_coop_turret_assets.h"\n'
SCOPE = '\tTrialCoopTurretAssetScope coop_turret_asset(ctx,base);\n'
SELECT = '\tcoop_turret_asset.Select(ctx);\n'
LOADED = '\tTrialCoopTurretResourceLoaded(ctx,base);\n'
PINS = (
    (0x20bc40,374,'8b804112368ad0494d8167c37bc8d3f5517abd40a61017e06689529fe135cb6a'),
    (0x21d0e0,167,'fbea20ad1cbd5df273d831c1c30542a008d9014588426a847a23193e85475b33'),
    (0x199810,55,'51c6bfc95c9d57cd8215bcfd125039c2ff425d7fd6aac3bd9a97f78ca0e78871'),
    (0x54b70,122,'346e51ec44380ac147c78e6c0cc10e9fb9788a9c749c29bd6ef77f84bcfd02f6'),
    (0x8dac0,99,'ce3336cdd4c0cafcd40f47f70eec9def02a8cdb79933fae56ab0539f4bfe1f28'),
    (0xa6a50,192,'f527966e3efc03816624a65eeb5abbcd42c9172af7f998468ab45edf3ae98a91'),
    (0x4753a0,208,'bb005a3b99080ba959a3dcc922dcf991a7276021c1c3bb8e5399cfb0eb8dc18a'))
SOURCES = (
    (53,0x20bc40,'1e84d3d071a94c61af22370ac648cf685b88c9902f72a9c91618dcb6c54f465f'),
    (55,0x21d0e0,'c617eeb551027c6d7fb91609d169e8d45f4c492bd13bdd9f2d6448f2a6e47b4b'),
    (41,0x199810,'d03e254de3c211e3e669b36852133bd6eb16a17edcf5f34badd098cc0c14eadc'),
    (7,0x54b70,'a03df7c385386256dd1c6972c6daa8cb71815496e170682c46501aee9fd94f43'),
    (14,0x8dac0,'5560658165d2e334f31fa6bd28fa0b57fc9bb9d618b29f0e11eb3cc295b72a64'))

def function(source, va):
    return re.search(rf'X86_FUNC\(__imp__sub_{va:08X}\) \{{.*?^\}}', source, re.S | re.M)[0]

def strip(source):
    for hook in (INCLUDE,SCOPE,SELECT,LOADED):
        source=source.replace(hook,'')
    return source

def unprepare(area):
    for unit in (14,53):
        path=Path(area)/f'generated/game/x86_recomp.{unit}.cpp'
        if path.exists():
            old=path.read_text();new=strip(old)
            if old!=new:path.write_text(new)

def prepare(area):
    area=Path(area);local=Path(__file__).parent
    image=(area/'assets/Default.xbe').read_bytes()
    base=struct.unpack_from('<I',image,0x104)[0]
    count,table=struct.unpack_from('<II',image,0x11c)
    sections=[struct.unpack_from('<6I',image,table-base+i*56) for i in range(count)]
    for va,size,digest in PINS:
        section=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4])
        offset=section[3]+va-section[1]
        assert hashlib.sha256(image[offset:offset+size]).hexdigest()==digest,hex(va)
    sources={unit:strip((area/f'generated/game/x86_recomp.{unit}.cpp').read_text()) for unit,_,_ in SOURCES}
    for unit,va,digest in SOURCES:
        # The existing shared-character observer remains installed; it is not
        # part of this original source pin or this fixture's isolated boundary.
        original=sources[unit].replace('\tTrialCoopResourceLoaded(ctx,base);\n','')
        assert hashlib.sha256(function(original,va).encode()).hexdigest()==digest,hex(va)
    original=function(sources[53],0x20bc40)
    patched=original.replace('X86_FUNC(__imp__sub_0020BC40) {\n','X86_FUNC(__imp__sub_0020BC40) {\n'+SCOPE,1)
    assert patched.count('\t// 0020BC7C ')==1
    patched=patched.replace('\t// 0020BC7C ',SELECT+'\t// 0020BC7C ',1)
    sources[53]=sources[53].replace(original,patched,1)
    assert sources[14].count('\t// 0008DB16 ')==1
    sources[14]=sources[14].replace('\t// 0008DB16 ',LOADED+'\t// 0008DB16 ',1)
    for unit in (14,53):
        assert sources[unit].count('#include "x86_recomp_shared.h"\n')==1
        sources[unit]=sources[unit].replace('#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+INCLUDE,1)
        (area/f'generated/game/x86_recomp.{unit}.cpp').write_text(sources[unit])
    for name in ('xbe_coop_turret_assets.h','xbe_coop_turret_assets.cpp'):
        (area/'src/common'/name).write_text((local/name).read_text())
    witness=[]
    for name,body in (('OriginalModel',original),('CoopModel',patched)):
        witness.append(body.replace('__imp__sub_0020BC40',name))
    source=sources[14].replace('\tTrialCoopResourceLoaded(ctx,base);\n','')
    first=source.index('\t// 0008DB10 ');last=source.index('\t// 0008DB16 ',first)
    witness.append('X86_FUNC(ResourceResult) {\n'+source[first:last]+'}\n')
    test=(local/'test_xbe_coop_turret_assets.cpp').read_text()
    marker='// ORIGINAL_COOP_TURRET_ASSET_WITNESSES';assert test.count(marker)==1
    (area/'src/compat/test_coop_turret_assets.cpp').write_text(test.replace(marker,'\n'.join(witness)))

if __name__=='__main__':
    prepare(Path(__file__).resolve().parents[1]/'build/xbe-full-integration-20261014')

"""Use each mounted player's real posed minigun in both local views."""
from pathlib import Path
import hashlib
import struct

PINS=((756576, 736, 'b4cfb3b3ae174279a5606a6ac472fbe982044e5ff207c2752c1d11ce5e1b3780'), (744592, 38, '166f3352f6077c8ad16827e4be4adc43cb85f9bc46fb955d69304a840471b8eb'), (742768, 47, 'e393bde54e3b8abbeb4ddf2f008fb1704d6a2d0692d9dff63655f908c7ebd163'), (745152, 11, '144869400e549c4796731a3ceb2776a5a3e82b4595262537b3e60881f07dd751'), (745104, 4, 'b0b7cb280e0f183e8088b419423d32cefe14dfde3785a7245a47f4ff1eafb3da'), (2207696, 62, '048dfff8c0045bec2ebfb60f3db59581506663c2663cfb9df2a64136c61b5f8f'), (2138832, 61, '70b6ae1c271f03f8dade3ed513053ffe05c6976e86abf20023f93912ba8ed5a0'), (848863, 54, 'b9518ffce2e8a4061e60975704e1fe6622b6b771cf704294bec7899c8ad53a56'), (786978, 28, '48e554471244f478cf80efa09f6c86570ecc91c5497ef37f9cc4654b1430e7fa'), (849468, 16, 'a25c97d3eb73d1a0f328a165705ea1e7f099f6f61b88765c04b63658dac1a76c'), (742960, 91, 'abb8b104ef9bea97cc95c57d65f330ddcfdcb0747f4fae50ff62e580de7d5a73'), (764944, 836, '82e28ff7598be95e3df73f3eb8656fda5760b933767e8ea43b4349cf8598602a'), (2183264, 794, 'a3710b17e87403e3138092af5081882c84c6af71c03b249ee4122961d611167d'), (1895746, 16, '94615cacf728c686cfdd3064e2be8f3cf80d3a4014817abfe6b24ab79e73da54'), (2144544, 38, '85881c00feb5a923681f381f9940202463284a959d6f695f0675fcf2eb9ddc34'), (4552832, 80, 'dfe84f7c0701601dca91e13ca00d88ed2bfbb73e35001cd229dd4d05851f051e'), (4552752, 20, 'c0eb8f922a38a6ff71672af75711337d2e028b27fbac94a6d70e2709fd30f322'), (3830330, 6, '8df471ed38424bb4041ca7c8bb78e0ed7769b738c4582339833c4db16eb2ea32'), (939504, 346, '9b63482b7de38115789d9d2007a7c002c5a5a6796d3525459a74924f487271ba'), (1036272, 64, '58ac3389ba0a60d8458574e870a34e954cc7b110fe37949b99376ccba500cd51'))
FUNCTIONS=((19, 756576, 'da931cc4c29857b06b1b740b5c927b02134add21c9deaf8d64d75656af7c8cba'), (18, 744592, 'dc1e8116b78056304c4805b164c3d4109f8581b82c7f5452d364dea7757002f6'), (18, 742768, '38bdd08920ba41a22c92742210a53179ce22aeeb4bf86aca263394a49d5ca592'), (18, 745152, '445fbe33b4d0e95d9795b10bdf8d397fea628e53efeff80560d65dde8570e41d'), (18, 745104, '9363b1647bff1e55c2081cc41e19ccd0b7da663c80614a468a1325fe9c2a2698'), (55, 2207696, '6f1102aa116735ff80a359b49a6f69ebafe4f879c52d411f1e2f09b365efe98f'), (52, 2138832, '86404722688b3cf4bec5bc4763bb1e546569dd3d9c4c0db451ab4e5373708aaa'))
HOOKS={
    20:[('\t// 000CF3F3 mov ecx, [esi+0x2B8]','\tTrialCoopTurretWeaponMatrix(ctx,base);\n')],
    54:[('X86_FUNC(__imp__sub_00215060) {\n','\tTrialCoopTurretView trial_coop_turret_view;\n'),
        ('\t// 0021525D push 0xFF','\ttrial_coop_turret_view.Select(ctx,base);\n'),
        ('\t// 0021532D inc esi','\ttrial_coop_turret_view.Clear();\n')],
}
INCLUDE='#include "xbe_coop_turret_view.h"\n'
def unprepare(area):
    for unit,seams in HOOKS.items():
        path=Path(area)/f'generated/game/x86_recomp.{unit}.cpp'
        if not path.exists():continue
        source=path.read_text().replace(INCLUDE,'')
        for _,hook in seams:source=source.replace(hook,'')
        path.write_text(source)
def prepare(area):
    area=Path(area);local=Path(__file__).parent;unprepare(area)
    xbe=(area/'assets/Default.xbe').read_bytes();base=struct.unpack_from('<I',xbe,0x104)[0]
    count,table=struct.unpack_from('<II',xbe,0x11c)
    sections=[struct.unpack_from('<6I',xbe,table-base+i*56) for i in range(count)]
    for va,size,digest in PINS:
        section=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4]);off=section[3]+va-section[1]
        assert hashlib.sha256(xbe[off:off+size]).hexdigest()==digest,hex(va)
    original=[]
    for unit,address,digest in FUNCTIONS:
        source=(area/f'generated/game/x86_recomp.{unit}.cpp').read_text()
        start=source.index(f'X86_FUNC(__imp__sub_{address:08X}) {{')
        stop=source.index('\nX86_WEAK_FUNC',start);body=source[start:stop]
        assert hashlib.sha256(body.encode()).hexdigest()==digest,hex(address)
        original.append(body)
    source=(area/'generated/game/x86_recomp.19.cpp').read_text()
    start=source.index('\t// 000C0222 ');stop=source.index('\t// 000C023E ',start)
    body=source[start:stop]
    assert hashlib.sha256(body.encode()).hexdigest()=="94eaea7747050e9e07e4218a8ef926d141751107b8493f531a1c22d66725e83d"
    selector='X86_FUNC(OriginalTurretMuzzleSelector) {\n\tX86_LOAD_FLAGS();\n\tuint32_t ca=0,cb=0;\n'+body+'\nreturn;\nloc_000C0386: assert(false);\n}\n'
    for unit,seams in HOOKS.items():
        path=area/f'generated/game/x86_recomp.{unit}.cpp';source=path.read_text()
        for anchor,hook in seams:
            assert source.count(anchor)==1,anchor
            source=source.replace(anchor,anchor+hook if anchor.startswith('X86_FUNC') else hook+anchor,1)
        source=source.replace('#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+INCLUDE,1)
        path.write_text(source)
    for name in ('xbe_coop_turret_view.h','xbe_coop_turret_view.cpp'):
        (area/'src/common'/name).write_text((local/name).read_text())
    test=(local/'test_xbe_coop_turret_view.cpp').read_text()
    for marker,body in [('// ORIGINAL_TURRET_FUNCTIONS','\n'.join(original)),('// ORIGINAL_TURRET_MUZZLE_SELECTOR',selector)]:
        assert test.count(marker)==1;test=test.replace(marker,body)
    (area/'src/compat/test_coop_turret_view.cpp').write_text(test)

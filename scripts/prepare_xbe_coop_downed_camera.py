"""Bind a downed player's existing camera to the original named head pose."""
from pathlib import Path
import hashlib
import re
import struct

PINS=(
    (0x20b950,528,'3bdf00742df1000436f99b2cee446bcf4f911c0a24f7ae283f966ec662a0c808'),
    (0x21afb0,24,'cd62958666a316878db6c614c35fc40a24f4d44a2c2e6556e61b53ac287d2cca'),
    (0x21afd0,62,'048dfff8c0045bec2ebfb60f3db59581506663c2663cfb9df2a64136c61b5f8f'),
    (0x21b0d0,204,'fd20eb18f8a7b18dbf36a84045929f6526b982034e110f6d025b24315cec1dc9'),
    (0xb5ec0,11,'144869400e549c4796731a3ceb2776a5a3e82b4595262537b3e60881f07dd751'),
    (0x45c6dc,11,'515e09d1c4d42dc74960ec4924eae3ecbabce515b820f2149832ae79b49b656b'),
    (0x457880,40,'239ee51c5c5187260431340ea10751648f0b4d35dff30c70c4d0241045b4c0cc'),
    (0xc01ea,56,'dbc167376678d04fb6246ec44a206fb8edb6f21f7792cb39ce47a2f87ec4ae81'),
    (0x214e70,4,'108eb05660dfe1a041240deb6da273082575a75017fa45a5f13e0086c713bc3d'),
    (0x1d60b6,101,'31e91bb629a874d40e26433fdca475543a908f1890c6c428bbb2366af1aded0c'),
    (0xc7f40,1541,'13e200fb09a2086e02c0f949db37a820ee41b1e9f8b309989453f2622517106d'),
    (0x1cdee0,2188,'3e663bffac54a5898cdd612b5ed257a2ad66f0d6616ff052a1e1abcf9de9da5e'),
)
FUNCTIONS=(
    (53,'0020B950','673fcf89c84136f5dfcfe15c2105c51a43ab18088da3038bbc97ea779abbcb24'),
    (55,'0021AFB0','efcdbfc524928d4817878b8778fef21308d43894f2ea8c0b133d5f0a2b1fe180'),
    (55,'0021AFD0','6f1102aa116735ff80a359b49a6f69ebafe4f879c52d411f1e2f09b365efe98f'),
    (55,'0021B0D0','4867bcf7c559a08f1abde91d616f0f88d3d78106d36d86ba7573dd06c21bef80'),
    (18,'000B5EC0','445fbe33b4d0e95d9795b10bdf8d397fea628e53efeff80560d65dde8570e41d'),
)
HOOKS={
    20:('X86_FUNC(__imp__sub_000C7F40) {\n','\tTrialCoopDownedCameraCapture(ctx,base);\n'),
    46:('\t// 001CE767 pop esi','\tTrialCoopDownedCameraUpdate(ctx,base);\n'),
}
INCLUDE='#include "xbe_coop_downed_camera.h"\n'

def unprepare(area):
    for unit,(_,hook) in HOOKS.items():
        path=Path(area)/f'generated/game/x86_recomp.{unit}.cpp'
        if path.exists():
            before=path.read_text();after=before.replace(INCLUDE,'').replace(hook,'')
            if after!=before:path.write_text(after)

def prepare(area):
    area=Path(area);local=Path(__file__).parent
    xbe=(area/'assets/Default.xbe').read_bytes();base=struct.unpack_from('<I',xbe,0x104)[0]
    count,table=struct.unpack_from('<II',xbe,0x11c)
    sections=[struct.unpack_from('<6I',xbe,table-base+i*56) for i in range(count)]
    for va,size,digest in PINS:
        s=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4]);start=s[3]+va-s[1]
        assert hashlib.sha256(xbe[start:start+size]).hexdigest()==digest,hex(va)
    for name in ('xbe_coop_downed_camera.h','xbe_coop_downed_camera.cpp'):
        (area/'src/common'/name).write_text((local/name).read_text())
    test=(local/'test_xbe_coop_downed_camera.cpp').read_text()
    for unit,address,digest in FUNCTIONS:
        source=(area/f'generated/game/x86_recomp.{unit}.cpp').read_text()
        f=re.search(r'X86_FUNC\(__imp__sub_'+address+r'\) \{.*?^\}',source,re.M|re.S).group()
        assert hashlib.sha256(f.encode()).hexdigest()==digest,address
        marker='// ORIGINAL_'+address;assert test.count(marker)==1
        test=test.replace(marker,f)
    (area/'src/compat/test_coop_downed_camera.cpp').write_text(test)
    unprepare(area)
    for unit,(anchor,hook) in HOOKS.items():
        path=area/f'generated/game/x86_recomp.{unit}.cpp';source=path.read_text()
        assert source.count(anchor)==1,anchor
        source=source.replace(anchor,anchor+hook if anchor.startswith('X86_FUNC') else hook+anchor,1)
        source=source.replace('#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+INCLUDE,1)
        assert source.count(hook)==1
        path.write_text(source)

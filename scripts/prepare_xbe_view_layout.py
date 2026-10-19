"""Player/weapon FOV, the retail widescreen HUD, and weapon-reticle sizing."""
from pathlib import Path
import hashlib
import struct


def prepare(area):
    area=Path(area);local=Path(__file__).parent
    # Byte-identical code in the retail and launcher-normalized executable.
    xbe=(area/'assets/Default.xbe').read_bytes()
    base=struct.unpack_from('<I',xbe,0x104)[0]
    count,table=struct.unpack_from('<II',xbe,0x11c)
    sections=[struct.unpack_from('<6I',xbe,table-base+i*56) for i in range(count)]
    for va,size,digest in [
        (0x1cdee0, 2188, '3e663bffac54a5898cdd612b5ed257a2ad66f0d6616ff052a1e1abcf9de9da5e'),
        (0x73d80, 395, '2580ad629ab2db40193f06f710a4f7bbeebc1b4920efa86c20a8c88f4fdc02c3'),
        (0x9d9b0, 242, '26966356054def9572634c6237ec90e69eaac615c3ec40db4f7a8cfe4801d4c7'),
        (0x76860, 15, '24a4c85f0a627c25852f592dfad31fcaf22cf74184c7f8c3940a4c20053e6e4a'),
        (0x17282f, 22, 'ecca9d17791b8b98063d0ec49d512551f59ec2bb481daec4e74500a385135506'),
        (0x73790, 320, 'f764c442d01e431bea7a4735171d0965691a0dcba6e83c18d09c28894ee01d69'),
        (0x73aa0, 11, '69755aea4ac89db056f26c0af2f1d87a52e7a908fb954ca90a67eaec9497b342'),
        (0x7afe0, 371, '517e9dca46baffbb224e329285e4e1995aedec6dbebcbf9d6b9b1ab4470195c1'),
        (0xbac10, 836, '82e28ff7598be95e3df73f3eb8656fda5760b933767e8ea43b4349cf8598602a'),
        (0xc8cfd, 226, '4eb6854b09988e1eb8982c7a2bdee850533952eb9471c8dfc7eb61d23d3c7ea9'),
        (0x214e90, 7, 'c9989f2d2a92d4bfc946a71ab0394c9eb988d39f411a9ffc0cf66a08b20ccc62'),
        (0x73700, 93, 'a4823d420ec95a36f7be385b9c2110627960ba8e4d816232e03809801a1faa2d'),
        (0x75b90, 846, 'e7efeeb9f7302c775932b80232ecd3aec2ece592474657ad1d8e791d37b12a5f'),
        (0x470a0, 296, '0a9d863db132f03c13b03d8b39df5b8c3edf8700e117acbf6f37e581c16c3b71'),
        (0x47300, 773, '93a556959eaba855c748d672c47460d602939e578eb870341b18b939d78fd2cb'),
        (0x47610, 960, 'e0940fb3d30dd20c96ea8fe011d9090e0aa0a19aa5c655776f6529e24cdaf011'),
    ]:
        section=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4])
        offset=section[3]+va-section[1]
        assert hashlib.sha256(xbe[offset:offset+size]).hexdigest()==digest,hex(va)
    for name in ('xbe_view_layout.h','xbe_view_layout.cpp'):
        (area/'src/common'/name).write_text((local/name).read_text())
    hooks={
        37:[('\t// 0017283A cmp eax, 0xFFFFFFFF','\tTrialPlayerCameraCreated(ctx,base);\n')],
        46:[('X86_FUNC(__imp__sub_001CDEE0) {\n','\tconst float trial_camera_target=TrialCameraTarget(ctx,base);\n')],
        11:[('X86_FUNC(__imp__sub_00073D80) {\n','\tTrialHudLayoutScope trial_hud_layout(ctx,base);\n'),
            ('X86_FUNC(__imp__sub_00073AA0) {\n','\tTrialReticleScope trial_reticle(ctx,base);\n'),
            ('X86_FUNC(__imp__sub_00075B90) {\n','\tTrialProjectedHudScope trial_projected_hud(ctx,base);\n'),
            ('\t// 00073E32 call 0x0022D360','\tTrialHudLayout(ctx,base);\n')],
        16:[('X86_FUNC(__imp__sub_0009D9B0) {\n','\tTrialHudQuadScope trial_hud_quad(ctx,base);\n')],
        19:[('\t// 000BADED call [eax+0x28]','\tTrialWeaponProjection(ctx,base);\n')],
    }
    include='#include "xbe_view_layout.h"\n'
    for chunk,seams in hooks.items():
        p=area/f'generated/game/x86_recomp.{chunk}.cpp';s=p.read_text().replace(include,'')
        for anchor,hook in seams:
            s=s.replace(hook,'');assert s.count(anchor)==1,anchor
            s=s.replace(anchor,anchor+hook if anchor.startswith('X86_FUNC') else hook+anchor,1)
        if chunk==46:
            s=s.replace('\tTrialCameraFovScope trial_camera_fov(ctx,base);\n','')
            first=s.index('X86_FUNC(__imp__sub_001CDEE0) {')
            last=s.index('\nX86_WEAK_FUNC(sub_001CDEE0',first)
            body=s[first:last]
            compare='{ uint32_t ea = (uint32_t)(ctx.esi + 0x228u); float a = ctx.xmm[0].f[0], b = X86_MEMF32(ea);'
            changed=compare.replace('X86_MEMF32(ea)','trial_camera_target')
            body=body.replace(changed,compare);assert body.count(compare)==1;body=body.replace(compare,changed,1)
            load='{ uint32_t ea = (uint32_t)(ctx.esi + 0x228u); ctx.xmm[0].u[0] = X86_MEM32(ea);'
            changed=load.replace('ctx.xmm[0].u[0] = X86_MEM32(ea)','ctx.xmm[0].f[0] = trial_camera_target')
            body=body.replace(changed,load);assert body.count(load)==1;body=body.replace(load,changed,1)
            s=s[:first]+body+s[last:]
        if chunk==11:
            original='\t// 00076862 push 0x43A00000\n\t{ uint32_t v = (uint32_t)(0x43A00000u); X86_PUSH32(v); }'
            changed='\t// 00076862 push 0x43A00000\n\t{ uint32_t v = TrialHudCenterBits(); X86_PUSH32(v); }'
            s=s.replace(changed,original);assert s.count(original)==1;s=s.replace(original,changed,1)
        s=s.replace('#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+include,1)
        p.write_text(s)
    source=(area/'generated/game/x86_recomp.19.cpp').read_text()
    first=source.index('X86_FUNC(__imp__sub_000BAC10) {')
    last=source.index('\nX86_WEAK_FUNC(sub_000BAC10',first)
    test=(local/'test_xbe_view_layout.cpp').read_text()
    assert test.count('// GENERATED_MODEL_OPERATION')==1
    test=test.replace('// GENERATED_MODEL_OPERATION',source[first:last])
    source=(area/'generated/game/x86_recomp.6.cpp').read_text()
    first=source.index('X86_FUNC(__imp__sub_000470A0) {')
    last=source.index('\nX86_WEAK_FUNC(sub_000470A0',first)
    assert test.count('// GENERATED_POINT_PROJECTION')==1
    test=test.replace('// GENERATED_POINT_PROJECTION',source[first:last])
    (area/'src/compat/test_view_layout.cpp').write_text(test)

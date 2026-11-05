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
        (0xa5c50,85,'1d32eaf934ad8376c68bfd9f231236ca6ae2280b126a3edabec8c9b318e74993'),
        (0x8bf4e,19,'8acedae21d632b25135a7835cdcb0a225fcbc83047adcb9b74c9751023c32a28'),
        (0xa5e80,273,'b498e0b214e5f1554201bee857ff6d178beac8c4524207f8cb178c1094aa3562'),
        (0x272870,317,'d4e626b5839ae231a22f9c129becf403586dbeae36ae06f910a5a5d18088acc6'),
        (0x272bb0,1027,'6f33cf9392fdf78e692cafcaa47169817c1d5fcec37b20fa22eb2fc5d738021c'),
        (0xa5cd0,179,'8635225542d840952d85bb00227e9f75cc336f45f2fa33183e9d1513628891ea'),
        (0x738d0,384,'b597664a720e41d60024d10f799fac58ab278a09980a1428d440363fbb9b2837'),
        (0x70d90,455,'879111defca52dd723604dadc820a063cf5705351be989ed3f38d58867be6671'),
        (0x72b50,60,'afadb1353b883d95b024a9df1eab40feb131e45afdf63c01826869ce55784691'),
        (0x4e868c,24,'17230f1c400f9d998fcc6aac9d66bb6c733ab04249dde4d579d05c56ebaa1048'),
        (0x77450,15,'d2f7435b7190543986656f45b0e59ae873f48e1450f56217d30dc2425f229fa6'),
        (0x72200,36,'0b4813430d8d6f91d3585289115f336e2956ea9f0dbda3e66493a44ccb326a73'),
        (0x77570,46,'b00f306ce8dbed63cf84144706f2ae3b9cbc20ad25e0a22350963e349828e763'),
        (0x273860,281,'5e6001ceef33b5965ba6b0459efbd1ab10b54fcbf8d3bd60863516f5ee1ded72'),
        (0x274320,134,'54627a82dcda6d3f3e2aec0283309cf09d8ec7594999f1b015d5020038d8ecde'),
        (0x453580,12,'02507f4402ade8eb2201636ede9a4718e71e563065550b2b8b520534699d556c'),
        (0x48905c,8,'f684cd358df4f91ebbcca8a73a795b6c13f3d446bb8c3f951c40b8000ffaec8e'),
        (0x2739a0,343,'81d5e3ce97cffd7ba695c5da6c17ce9cde0153d7dc3baa48a95c57ec90d27884'),
        (0x7aae0,124,'b2df8d95b517070f414c9bc567bf5b5191ca4092f15f91ff9a52d1f2f7aecb0a'),
        (0xd26dc,252,'e8414ecb2ff092478eb0b5099036dbb6b32321f85fe5ca0ec466893fafd09bbe'),
        (0xcdac0,2952,'ef172d0e4d0787ce2c6f88b267621a9eab47e68ddb13a4d44f250494ea83a38c'),
        (0x731e0,136,'0fc7ea519e666a17aa7391c9729b7b3d848d5e239c9615ff80a73398f008f26c'),
        (0x65170,152,'065c75529af885b1496619ccb3d328b405fb562dc7bf321db649260df5a2f553'),
        (0x65550,137,'aab96479d66b4c8839b15e7563beadd6ed280c69ce36ebfd5c59f276fda68c5f'),
        (0x656d0,129,'d0bbcf01f129b1ff53e28e215f9ab9a09377407b96a2472d7940605a37c0037c'),
        (0x65650,124,'beb93a3c17cf3d5739402d3fb345077df15a69005afb9ef76a852c62412160d9'),
        (0x658c0,77,'a4dd74ea9507df734084e55c779e2f29fccdecf72f4f216ba1901dc7f8a1bf3a'),
        (0x871a0,193,'6f03ecdb070f35df1e281676d6668634a3f2ce22f723a4f41c3ac2edf723e452'),
        (0x718a0,87,'939c58895e89c7cb466570e62e84178461b147aaa4b11fb85cb79d285846efd7'),
        (0x7c1e0,173,'9bc53f0d2b974328dad134710efd10f5536d7517cba79ea63c19e5ac61621bb5'),
        (0x1cdee0, 2188, '3e663bffac54a5898cdd612b5ed257a2ad66f0d6616ff052a1e1abcf9de9da5e'),
        (0x73d80, 395, '2580ad629ab2db40193f06f710a4f7bbeebc1b4920efa86c20a8c88f4fdc02c3'),
        (0x9d9b0, 242, '26966356054def9572634c6237ec90e69eaac615c3ec40db4f7a8cfe4801d4c7'),
        (0x76860, 15, '24a4c85f0a627c25852f592dfad31fcaf22cf74184c7f8c3940a4c20053e6e4a'),
        (0x17282f, 22, 'ecca9d17791b8b98063d0ec49d512551f59ec2bb481daec4e74500a385135506'),
        (0x73790, 320, 'f764c442d01e431bea7a4735171d0965691a0dcba6e83c18d09c28894ee01d69'),
        (0x73aa0, 11, '69755aea4ac89db056f26c0af2f1d87a52e7a908fb954ca90a67eaec9497b342'),
        (0x7afe0, 371, '517e9dca46baffbb224e329285e4e1995aedec6dbebcbf9d6b9b1ab4470195c1'),
        (0x9d7e0, 43, 'cb99388a8b0a8473975114b301d43cb32e09b03ab259493d4313d659b6924bb8'),
        (0x7b340, 18, '32d7b61190624b4bdc34f730974edbfd8b92aef2782a50f9ed1811a739eec315'),
        (0x7a690, 223, '28680f763bc8c3ff3418dd9ea79925ce6eb17c6958ad3c5b0df62f3e1e197778'),
        (0x442d0, 16, '3d7245716e4be8e627b5de93ab9fe45899b9060cf2303a95c4c416a7842b437f'),
        (0x4e55d0, 6480, '4bae31a042b3806974a3374d99fe3167e3585e07777d172932ee3a9065b44fb5'),
        (0x77ec0, 5364, '8cb125d3a1d50ebf9f0d00b579326950486e725ea73a1ec13df7e920647a6ee2'),
        (0xbac10, 836, '82e28ff7598be95e3df73f3eb8656fda5760b933767e8ea43b4349cf8598602a'),
        (0x175820, 985, '86e1a44738234d4780eca40a498427fc55370048bebe9319ebb567102f31ab87'),
        (0xc8cfd, 226, '4eb6854b09988e1eb8982c7a2bdee850533952eb9471c8dfc7eb61d23d3c7ea9'),
        (0x214e90, 7, 'c9989f2d2a92d4bfc946a71ab0394c9eb988d39f411a9ffc0cf66a08b20ccc62'),
        (0x73700, 93, 'a4823d420ec95a36f7be385b9c2110627960ba8e4d816232e03809801a1faa2d'),
        (0x75b90, 846, 'e7efeeb9f7302c775932b80232ecd3aec2ece592474657ad1d8e791d37b12a5f'),
        (0x470a0, 296, '0a9d863db132f03c13b03d8b39df5b8c3edf8700e117acbf6f37e581c16c3b71'),
        (0x47300, 773, '93a556959eaba855c748d672c47460d602939e578eb870341b18b939d78fd2cb'),
        (0x47610, 960, 'e0940fb3d30dd20c96ea8fe011d9090e0aa0a19aa5c655776f6529e24cdaf011'),
        (0x215060, 794, 'a3710b17e87403e3138092af5081882c84c6af71c03b249ee4122961d611167d'),
        (0xd4804, 6, '92a53488b5553b4b1eed596a34ebb8b57a00374deda019427b8e68acb5ebf3a0'),
        (0xcb3c8,5,'771c91743733499569b23b81b7ef7e6d709fb0fa836991c36c0d73410674ed88'),
        (0xcab20,3829,'3c0dd657f8d424b1a3ec478d42922bc0f149fe3985b26d965e7f9b93ac79c4a7'),
        (0xa5c30,30,'69622ece3ece926676dfa7fe6355ae5a83fc98a4141ee47d8eab3c22326e7021'),
        (0x21e260,387,'85172910b4bfb17d15716e3825c3af773009572efff63f6cd5e4babe8138f1d6'),
        (0x21e3f0,117,'7a76c30e24977cfa92c8003e7b0c7d1b3bf53e6db44d9c6418c75d56c532daa5'),
    ]:
        section=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4])
        offset=section[3]+va-section[1]
        assert hashlib.sha256(xbe[offset:offset+size]).hexdigest()==digest,hex(va)
    for name in ('xbe_view_layout.h','xbe_view_layout.cpp'):
        (area/'src/common'/name).write_text((local/name).read_text())
    hooks={
        9:[('X86_FUNC(__imp__sub_000658C0) {\n','\tTrialProjectedHudScope trial_scope_tint(ctx,base);\n')],
        21:[('\t// 000CDCA8 mov edx, [eax]','\tTrialLadderHands(ctx,base);\n')],
        37:[('\t// 0017283A cmp eax, 0xFFFFFFFF','\tTrialPlayerCameraCreated(ctx,base);\n'),
            ('X86_FUNC(__imp__sub_00175820) {\n','\tTrialTaserVertices trial_taser_vertices(ctx,base);\n')],
        46:[('X86_FUNC(__imp__sub_001CDEE0) {\n','\tconst float trial_camera_target=TrialCameraTarget(ctx,base);\n')],
        11:[('X86_FUNC(__imp__sub_00073D80) {\n','\tTrialHudLayoutScope trial_hud_layout(ctx,base);\n'),
            ('X86_FUNC(__imp__sub_000731E0) {\n','\tTrialProjectedHudScope trial_split_scope(ctx,base);\n'),
            ('\t// 0007325E push edi','\tTrialScopeBorders(ctx,base);\n'),
            ('X86_FUNC(__imp__sub_00073AA0) {\n','\tTrialReticleScope trial_reticle(ctx,base);\n'),
            ('X86_FUNC(__imp__sub_00077570) {\n','\tTrialHudHelpScope trial_hud_help(ctx,base);\n'),
            ('X86_FUNC(__imp__sub_00075B90) {\n','\tTrialProjectedHudScope trial_projected_hud(ctx,base);\n'),
            ('\t// 00073E32 call 0x0022D360','\tTrialHudLayout(ctx,base);\n')],
        12:[('X86_FUNC(__imp__sub_0007C1E0) {\n','\tTrialHudInsetScope trial_hud_inset(ctx,base);\n')],
        16:[('X86_FUNC(__imp__sub_0009D9B0) {\n','\tTrialHudQuadScope trial_hud_quad(ctx,base);\n')],
        17:[],
        14:[],
        65:[('X86_FUNC(__imp__sub_00272870) {\n','\tTrialMenuLayoutScope trial_menu_layout;\n'),
            # The news clock belongs to its uncropped 4:3 film, not the 16:9 menus.
            ('X86_FUNC(__imp__sub_00272BB0) {\n','\tTrialMenuLayoutScope trial_movie_overlay(4.f/3.f);\n')],
        19:[('\t// 000BADED call [eax+0x28]','\tTrialWeaponProjection(ctx,base);\n')],
        54:[('X86_FUNC(__imp__sub_00215060) {\n','\tTrialPlayerViewScope trial_player_view;\n'),
            ('\t// 0021525A call [eax+0x4C]','\ttrial_player_view.Select(ctx,base);\n'),
            ('\t// 0021532D inc esi','\ttrial_player_view.Clear();\n')],
    }
    include='#include "xbe_view_layout.h"\n'
    for chunk,seams in hooks.items():
        p=area/f'generated/game/x86_recomp.{chunk}.cpp';s=p.read_text().replace(include,'')
        for anchor,hook in seams:
            s=s.replace(hook,'');assert s.count(anchor)==1,anchor
            s=s.replace(anchor,anchor+hook if anchor.startswith('X86_FUNC') else hook+anchor,1)
            assert s.count(hook)==1,hook
        if chunk==17:
            original='{ uint32_t ea = (uint32_t)(ctx.esp + 0x18u); float a = ctx.xmm[0].f[0], b = X86_MEMF32(ea); (void)a; ctx.xmm[0].f[0] = a * b; }'
            changed=original.replace('X86_MEMF32(ea)','TrialRootAspect(ctx,base)')
            first=s.index('X86_FUNC(__imp__sub_000A5C50) {')
            last=s.index('\nX86_WEAK_FUNC(sub_000A5C50',first)
            body=s[first:last].replace(changed,original)
            assert body.count(original)==1
            s=s[:first]+body.replace(original,changed,1)+s[last:]
        if chunk==14:
            # The logical view must use the root's aspect, or retail refits it to 16:9.
            original='\t// 0008BF4E mov ecx, [esp+0x1C]\n\t{ uint32_t ea = (uint32_t)(ctx.esp + 0x1Cu); ctx.ecx = (uint32_t)(X86_MEM32(ea)); }'
            changed='\t// 0008BF4E mov ecx, [esp+0x1C]\n\tctx.ecx = TrialDisplayAspectBits();'
            s=s.replace(changed,original);assert s.count(original)==1
            s=s.replace(original,changed,1)
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
            # Apply color after the native target/spin decision, before the fade tree update.
            hook='\tTrialReticleColor(ctx,base);\n'
            s=s.replace(hook,'')
            anchor='\t// 000739F9 lea edi, [esi+0x20]\n'
            assert s.count(anchor)==1
            s=s.replace(anchor,anchor+hook,1)
            original='\t// 00076862 push 0x43A00000\n\t{ uint32_t v = (uint32_t)(0x43A00000u); X86_PUSH32(v); }'
            changed='\t// 00076862 push 0x43A00000\n\t{ uint32_t v = TrialHudCenterBits(); X86_PUSH32(v); }'
            s=s.replace(changed,original);assert s.count(original)==1;s=s.replace(original,changed,1)
        s=s.replace('#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+include,1)
        p.write_text(s)
    # Retire the old compensating bullet bias; alignment belongs to the reticle.
    p=area/'generated/game/x86_recomp.20.cpp'
    hook='\tTrialCoopShotOffset(ctx,base);\n'
    s=p.read_text()
    assert s.count(hook) in (0,2)
    s=s.replace(include,'').replace(hook,'')
    anchor='\t// 000CB3C8 call 0x000EBF10'
    assert s.count(anchor)==2
    assert hook not in s
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
    for chunk,address,marker in ((37,'00175820','// GENERATED_TASER_DRAW'),(13,'000871A0','// GENERATED_SCOPE_COLOR'),(11,'000731E0','// GENERATED_SCOPE_DRAW'),
                                (9,'00065170','// GENERATED_RECT_QUEUE'),(9,'00065550','// GENERATED_RECT_ROUND'),(9,'000656D0','// GENERATED_RECT_DRAW'),
                                (9,'00065650','// GENERATED_FULLSCREEN_RECT'),(9,'000658C0','// GENERATED_SCOPE_TINT'),
                                (11,'000718A0','// GENERATED_DESCRIPTOR_COPY'),(12,'0007C1E0','// GENERATED_HUD_INSET'),
                                (11,'00077450','// GENERATED_REWARD_DRAW'),(11,'00072200','// GENERATED_OBJECTIVE_DRAW'),
                                (11,'00077570','// GENERATED_HELP_DRAW'),(65,'00273860','// GENERATED_HELP_TEXT_DRAW'),
                                (65,'002739A0','// GENERATED_NOTICE_TWEEN_DRAW'),
                                (12,'0007AFE0','// GENERATED_RETICLE_SPRITE'),
                                (12,'0007B340','// GENERATED_RETICLE_SIZE'),
                                (16,'0009D7E0','// GENERATED_RETICLE_QUEUE'),
                                (11,'0007AAE0','// GENERATED_CENTERED_SCOPE_CHILD'),
                                (17,'000A5C10','// GENERATED_VIEWPORT_HEIGHT'),(17,'000A5C20','// GENERATED_VIEWPORT_WIDTH'),
                                (17,'000A5C30','// GENERATED_VIEWPORT_ASPECT'),(17,'000A5C50','// GENERATED_ROOT_VIEWPORT'),
                                (17,'000A5E80','// GENERATED_FITTED_VIEWPORT'),
                                (17,'000A5CD0','// GENERATED_DERIVED_VIEWPORT'),(55,'0021E260','// GENERATED_PERSPECTIVE')):
        source=(area/f'generated/game/x86_recomp.{chunk}.cpp').read_text()
        first=source.index('X86_FUNC(__imp__sub_'+address+') {')
        last=source.index('\nX86_WEAK_FUNC(sub_'+address,first)
        assert test.count(marker)==1
        test=test.replace(marker,source[first:last].replace('__imp__sub_'+address,'sub_'+address))
    source=(area/'generated/game/x86_recomp.14.cpp').read_text()
    first=source.index('\t// 0008BF4E mov ecx, [esp+0x1C]')
    last=source.index('\t// 0008BF61 mov ecx, [esp+0x20]',first)
    test=test.replace('// GENERATED_DISPLAY_FIT',
        'X86_FUNC(NativeDisplayFit) {\n'+source[first:last]+'\n}\n')
    original=source[first:last].replace('ctx.ecx = TrialDisplayAspectBits();',
        'ctx.ecx = X86_MEM32(ctx.esp + 0x1Cu);')
    test=test.replace('// GENERATED_ORIGINAL_DISPLAY_FIT',
        'X86_FUNC(OriginalDisplayFit) {\n'+original+'\n}\n')
    source=(area/'generated/game/x86_recomp.11.cpp').read_text()
    first=source.index('X86_FUNC(__imp__sub_00077570) {')
    last=source.index('\nX86_WEAK_FUNC(sub_00077570',first)
    original=source[first:last].replace('\tTrialHudHelpScope trial_hud_help(ctx,base);\n','')
    test=test.replace('// GENERATED_ORIGINAL_HELP_DRAW',original.replace('__imp__sub_00077570','OriginalHelpDraw'))
    source=(area/'generated/game/x86_recomp.65.cpp').read_text()
    first=source.index('X86_FUNC(__imp__sub_00274320) {')
    last=source.index('\nX86_WEAK_FUNC(sub_00274320',first)
    queue=source[first:last].replace('__imp__sub_00274320','NativeHelpQueue')
    # A previous full preparation may contain the separate level-select menu
    # callback. This fixture executes the pinned retail help-list dispatch.
    queue=queue.replace('TrialLevelDetailTarget(ctx, base, X86_MEM32((uint32_t)(ctx.edx + 0x4u)))',
                        'X86_MEM32((uint32_t)(ctx.edx + 0x4u))')
    test=test.replace('// GENERATED_HELP_QUEUE_DRAW',queue)
    a=(area/'generated/game/x86_recomp.11.cpp').read_text()
    start=a.index('\t// 00073925 mov eax, [0x0051C4DC]')
    end=a.index('\t// 000739FC mov ebx, 0x1B',start)
    part=a[start:end].replace('ctx.eax=(ctx.eax&0xffffff00u)|TrialCoopSpinHud(base,ctx.esi);',
        '{ uint32_t ea = 0x51BDF4u; ctx.eax = (ctx.eax & 0xFFFFFF00u) | X86_MEM8(ea); }')
    test=test.replace('// GENERATED_RETICLE_COLOR',
        'X86_FUNC(NativeReticleColor) { X86_LOAD_FLAGS(); uint32_t ca=0,cb=0;\n'+part+'\n}\n')
    constants=[]
    for va in (0x456fbc,0x456fc0,0x456fc4,0x44d948,0x44bf94,0x44c1f8):
        section=next(s for s in sections if s[1]<=va and va+4<=s[1]+s[4])
        word=struct.unpack_from('<I',xbe,section[3]+va-section[1])[0]
        constants.append(f'W(b,0x{va:x},0x{word:x});')
    test=test.replace('// GENERATED_VIEWPORT_CONSTANTS',''.join(constants))
    (area/'src/compat/test_view_layout.cpp').write_text(test)

"""Adapt accepted frontend contracts to the current host; never mutate save policy."""
from pathlib import Path
import hashlib
import struct
import re


def prepare(area):
    """Run after prepare_xbe_input; no additional production translation unit."""
    area=Path(area);local=Path(__file__).parent;out=area/'src/common'
    image=(area/'assets/Default.xbe').read_bytes()
    base,=struct.unpack_from('<I',image,0x104)
    count,table=struct.unpack_from('<II',image,0x11c)
    sections=[struct.unpack_from('<6I',image,table-base+i*56) for i in range(count)]
    def read(va,n):
        s=next(s for s in sections if s[1]<=va and va+n<=s[1]+s[4])
        return image[s[3]+va-s[1]:s[3]+va-s[1]+n]
    # Private co-op reuses this exact retail campaign row and its original callback.
    assert read(0x272124,5)==bytes.fromhex('b8f48d4800')
    assert read(0x27212f,11)==bytes.fromhex('683017270050e8e613ffff')
    campaign_key=b'NewFrontend-Singleplayer-1\0'
    assert read(0x488df4,len(campaign_key))==campaign_key
    for va,data in [(0x270927,'e884e8ffff'),(0x270949,'e862e7ffff'),
                    (0x26f1b0,'558bec83e4f0'),(0x17b39,'8bf1'),(0x25c71f,'8bf157'),(0x25c89e,'e87d7a0100')]:
        assert read(va,len(bytes.fromhex(data)))==bytes.fromhex(data),hex(va)
    # Profile I/O warnings use these two original localized presentation paths.
    # Retain original string lookup, glyph layout, callbacks and text-bank data.
    for va,n,digest in [
        (0x17e40,0x9d,'5fa2ea1e2369123d03837056f6ba592793f641a9efadad9a11ad3c5ad79a3c7b'),
        (0x18ce0,0xfa,'8126a311d1f3da600487db6b319cec1be5d22a4aead4c733b43dc6d9e3d63836')]:
        assert hashlib.sha256(read(va,n)).hexdigest()==digest,hex(va)
    assert read(0x48cd84,18)==b'XBoxTRC-MemCard-1\0'
    assert read(0x48a850,18)==b'XBoxTRC-MemCard-2\0'
    warning_functions=[]
    for chunk,address,instruction,scratch,capacity in [
        (0,'00017E40','00017E87',0x10,0x800),
        (1,'00018CE0','00018D6F',0x24,0x200)]:
        path=area/f'generated/game/x86_recomp.{chunk}.cpp';source=path.read_text()
        declaration='extern "C" uint32_t TrialFrontendWarningText(uint8_t*,uint32_t,uint32_t,uint32_t,uint32_t);'
        if declaration not in source:
            marker='#include "x86_recomp_shared.h"'
            assert source.count(marker)==1;source=source.replace(marker,marker+'\n'+declaration,1)
        match=re.search(r'X86_FUNC\(__imp__sub_'+address+r'\) \{.*?^\}',source,re.S|re.M)
        assert match,address
        original=body=match[0]
        hook=f'\tctx.eax = TrialFrontendWarningText(base,ctx.esi,ctx.eax,ctx.esp+0x{scratch:X}u,0x{capacity:X}u);\n'
        if hook not in body:
            marker=f'\t// {instruction} push eax\n'
            assert body.count(marker)==1;body=body.replace(marker,hook+marker,1)
        source=source.replace(original,body,1);path.write_text(source)
        warning_functions.append(body)
    assert set(struct.unpack('<4I',read(0x26f688,16)))=={0x26f621,0x26f653,0x26f5ee}
    for name in ('xbe_frontend.h','xbe_frontend_guest.inc','xbe_frontend_text.h','xbe_coop_menu.h'):
        (out/name).write_text((local/name).read_text())
    (area/'src/compat/test_coop_menu.cpp').write_text((local/'test_xbe_coop_menu.cpp').read_text())
    # Drop only list-owned sprite submissions, retaining original row updates,
    # hit bounds and callbacks. The parent background is outside this scope.
    assert hashlib.sha256(read(0x9d9b0,242)).hexdigest()=='26966356054def9572634c6237ec90e69eaac615c3ec40db4f7a8cfe4801d4c7'
    queue=area/'generated/game/x86_recomp.16.cpp'
    text=queue.read_text()
    declaration='extern bool TrialFrontendHideListSprites();\n'
    hook='\tif (TrialFrontendHideListSprites()) { ctx.esp += 36; return; }\n'
    text=text.replace(declaration,'').replace(hook,'')
    text=text.replace('#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+declaration,1)
    marker='X86_FUNC(__imp__sub_0009D9B0) {\n'
    assert text.count(marker)==1
    queue.write_text(text.replace(marker,marker+hook,1))
    # Selection bars/arrows use the immediate rectangle path, not the glyph queue.
    assert hashlib.sha256(read(0x65550,0x89)).hexdigest()=='aab96479d66b4c8839b15e7563beadd6ed280c69ce36ebfd5c59f276fda68c5f'
    rectangle=area/'generated/game/x86_recomp.9.cpp';text=rectangle.read_text()
    hook='\tif (TrialFrontendHideListSprites()) { ctx.esp += 4; return; }\n'
    text=text.replace(declaration,'').replace(hook,'')
    text=text.replace('#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+declaration,1)
    marker='X86_FUNC(__imp__sub_00065550) {\n'
    assert text.count(marker)==1
    text=text.replace(marker,marker+hook,1)
    # Glyph quads at188CA/18A00 use65170 directly, bypassing65550 and9D9B0.
    assert hashlib.sha256(read(0x65170,0x98)).hexdigest()=='065c75529af885b1496619ccb3d328b405fb562dc7bf321db649260df5a2f553'
    marker='X86_FUNC(__imp__sub_00065170) {\n'
    assert text.count(marker)==1
    rectangle.write_text(text.replace(marker,marker+hook,1))
    assert hashlib.sha256(read(0x262A00,0x828)).hexdigest()=='fd2e08018be4779761129d8f6b582517d7c0b6ba368312b62ff40e0cea39f05c'
    # Qualify the actual pause Quit row, its shared yes/no factory and the
    # message/choice/separator calls. Do not cover or skip its scene backdrop.
    for va,n,digest in [
        (0x263520,0x4C,'8350b50059a6b20f4e2dbcee568b2c7f902e239644e78f70dc9dc95927cb1a45'),
        (0x265D9E,0x5B,'5e3b6ed5258a3e5104dc322a4c8cd48f611abcf05eec5be5d57ee5533a25e6ff'),
        (0x2677C2,0x18,'c84abe247a4b78bea601fb0a11715f861ea5e0b2b2f006618cfb6351c5e9766d')]:
        assert hashlib.sha256(read(va,n)).hexdigest()==digest,hex(va)
    # Exact checkpoint and mission restart row factories, sharing the Quit modal.
    assert hashlib.sha256(read(0x2676ec,0x17)).hexdigest()=='5e285ea10e48ef0754451eb8f1ad30260c78c5d07a825830c18d1794795b7e8a'
    assert hashlib.sha256(read(0x26770f,0x18)).hexdigest()=='ece4800760afe4c8b334b582a9110ba0a20c8256178d951d316718e033a27829'
    for va,label in [(0x473368,b'NewFrontEnd-LooseProgress-1\0'),
                     (0x48573C,b'NewFrontEnd-Yes-1\0'),(0x485750,b'NewFrontEnd-No-1\0')]:
        assert read(va,len(label))==label,hex(va)
    modal=area/'generated/game/x86_recomp.62.cpp';text=modal.read_text()
    declaration='extern bool TrialFrontendHidePauseDialogText(uint32_t);\n'
    if declaration not in text:
        marker='#include "x86_recomp_shared.h"\n'
        assert text.count(marker)==1;text=text.replace(marker,marker+declaration,1)
    match=re.search(r'X86_FUNC\(__imp__sub_0025A310\) \{.*?^\}',text,re.S|re.M)
    assert match;original=body=match[0]
    for caller,function,pop in [('25A5D5','00018CE0',32),('25A754','00018CE0',32),('25A7CE','00018DE0',24)]:
        marker=f'X86_PUSH32(0x{caller}u); sub_{function}(ctx, base);'
        replacement=(f'X86_PUSH32(0x{caller}u); if (TrialFrontendHidePauseDialogText(ctx.ebx)) '
                     f'ctx.esp += {pop}; else sub_{function}(ctx, base);')
        if replacement not in body:
            assert body.count(marker)==1;body=body.replace(marker,replacement,1)
    modal.write_text(text.replace(original,body,1))
    guest=out/'hybrid_frontend_guest.inc';text=guest.read_text()
    marker='if(!ucrr_native_frontend_enabled()||!hf_settled()||hf_view.kind!=kind'
    replacement='if(!FrontendGuestOwns(owner,kind)||!ucrr_native_frontend_enabled()||!hf_settled()||hf_view.kind!=kind'
    if replacement not in text:
        assert text.count(marker)==1;text=text.replace(marker,replacement,1)
    guest.write_text(text)
    special=out/'menu_dialog_guest.inc';text=special.read_text()
    declarations=('static bool FrontendProfileDelete(uint32_t,unsigned);\n'
                  'static bool FrontendProfileDeleteQuery();\n'
                  'static bool FrontendDeleteChoice(uint32_t,unsigned);\n'
                  'static bool FrontendDeleteParent();\n')
    if declarations not in text:text=declarations+text
    declarations='static void menu_dialog_capture();\nstatic void FrontendPauseDialogEnter();\n'
    if declarations not in text:text=declarations+text
    marker='    dialog_running=1;menu_special_reveal();\n'
    replacement=marker+'    FrontendPauseDialogEnter();\n'
    if replacement not in text:
        assert text.count(marker)==1;text=text.replace(marker,replacement,1)
    # Share the exact existing capture with early native-presentation handoff.
    # The original leave still owns all running/click/rectangle cleanup.
    if 'static void menu_dialog_capture(){' not in text:
        marker='void ucrr_menu_dialog_leave(void){\n'
        assert text.count(marker)==1
        start=text.index(marker);cleanup=text.index('    dialog_running=0;dialog_click=0;if(!dialog_drawn)dialog_count=0;',start)
        capture=text[start+len(marker):cleanup]
        text=text[:start]+'static void menu_dialog_capture(){\n'+capture+'}\n'+marker+'    menu_dialog_capture();\n'+text[cleanup:]
    marker='if(hf_view.kind&&!(dialog_running'
    replacement='if(hf_view.kind&&!FrontendDeleteParent()&&!(dialog_running'
    if replacement not in text:
        assert text.count(marker)==1;text=text.replace(marker,replacement,1)
    marker='(a.kind!=UC_HF_SELECT&&a.kind!=UC_HF_ACTIVATE))return 0;'
    replacement=('(a.kind!=UC_HF_SELECT&&a.kind!=UC_HF_ACTIVATE&&a.kind!=TRIAL_HF_DELETE))return 0;\n'
                 '        if(a.kind==TRIAL_HF_DELETE&&!FrontendProfileDelete(profile_owner,a.index))return 0;')
    if replacement not in text:
        assert text.count(marker)==1;text=text.replace(marker,replacement,1)
    marker='if(a.kind==UC_HF_SELECT||a.kind==UC_HF_ACTIVATE){MEM32(dialog_owner+8)=a.index;'
    replacement=('if(!FrontendDeleteChoice(dialog_owner,a.index))return;\n        '+marker)
    if replacement not in text:
        assert text.count(marker)==1;text=text.replace(marker,replacement,1)
    marker='static int menu_special_query(void){\n'
    replacement=marker+'    if(FrontendProfileDeleteQuery())return 1;\n'
    if replacement not in text:
        assert text.count(marker)==1;text=text.replace(marker,replacement,1)
    for kind,owner in [('DIALOG','dialog_owner'),('PROFILES','profile_owner')]:
        marker=f'if(ucrr_native_frontend_enabled()&&hf_view.kind==UC_HF_{kind}'
        replacement=f'if(FrontendGuestOwns({owner},UC_HF_{kind})&&ucrr_native_frontend_enabled()&&hf_view.kind==UC_HF_{kind}'
        if replacement not in text:
            assert text.count(marker)==1;text=text.replace(marker,replacement,1)
    marker='float y=MEMF(g_esp),bottom=MEMF(g_esp+4),right=MEMF(g_esp+8),x=MEMF(g_esp+12);'
    replacement=('/*259690 draws x=0..right; its fourth argument is fade width,\n'
                 '     * overwritten with right-fade before2843CE, not a left edge. */\n'
                 '    float y=MEMF(g_esp),bottom=MEMF(g_esp+4),right=MEMF(g_esp+8),x=0;')
    if replacement not in text:
        assert text.count(marker)==1;text=text.replace(marker,replacement,1)
    special.write_text(text)
    path=area/'generated/game/x86_recomp.65.cpp';text=path.read_text()
    # 270320 draws the campaign details before updating/drawing its +10C modal.
    assert read(0x270328,6)==bytes.fromhex('8b8e0c010000')
    assert read(0x270BB8,6)==bytes.fromhex('8bb60c010000')
    assert read(0x25A2F3,6)==bytes.fromhex('c700845c4800')
    assert read(0x25A260,3)==bytes.fromhex('8b5104')
    assert read(0x2703F5,6)==bytes.fromhex('8b86ec000000')
    # Difficulty factory258D40 adds253FF0's shade wrapper;2546B0 draws its +4 child.
    assert read(0x258DD2,5)==bytes.fromhex('e819b2ffff')
    assert read(0x254006,3)==bytes.fromhex('894804')
    assert read(0x254010,6)==bytes.fromhex('c700a4464800')
    assert read(0x2546C1,5)==bytes.fromhex('8b4e048b01')
    text=text.replace('extern void TrialFrontendLevelDrawEnd();\n','')
    declaration=('extern thread_local bool TrialFrontendLevelDrawHidden;\n'
                 'extern bool TrialFrontendLevelDrawBegin(uint32_t);\n')
    if declaration not in text:
        text=text.replace('#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+declaration,1)
    marker='\tTrialGuestHook(ctx, base, 10);\n'
    text=text.replace('TrialFrontendLevelDrawEnd(); TrialFrontendLevelDrawHidden=previous;',
                      'TrialFrontendLevelDrawHidden=previous;')
    hook=('\tstruct LevelDrawScope { bool previous=TrialFrontendLevelDrawHidden; '
          '~LevelDrawScope() { TrialFrontendLevelDrawHidden=previous; } } level_draw_scope;\n')
    if hook not in text:
        assert text.count(marker)==1;text=text.replace(marker,marker+hook,1)
    hook='\tTrialFrontendLevelDrawHidden=TrialFrontendLevelDrawBegin(ctx.esi);\n'
    marker='loc_002703F5:\n'
    if hook not in text:
        assert text.count(marker)==1;text=text.replace(marker,marker+hook,1)
    for address,op in [('0026F1B0',27),('0026F0B0',28)]:
        marker=f'X86_FUNC(__imp__sub_{address}) {{\n'
        hook=f'\tif (TrialGuestHook(ctx, base, {op})) {{ ctx.esp += 4; return; }}\n'
        if hook not in text:
            assert text.count(marker)==1;text=text.replace(marker,marker+hook,1)
    path.write_text(text)
    path=area/'generated/game/x86_recomp.67.cpp';text=path.read_text()
    # Only profile row drawing is scoped out; preview/medals and callbacks remain.
    assert hashlib.sha256(read(0x284330,0x15e)).hexdigest()=='2313ec48ec27d8e6f118939a70298e27482cd73eeeb999669da3fbdaa21bbbb4'
    declaration='extern thread_local uint32_t TrialFrontendProfileDrawOwner;\n'
    text=text.replace(declaration,'')
    text=text.replace('#include \"x86_recomp_shared.h\"\n','#include \"x86_recomp_shared.h\"\n'+declaration,1)
    marker='X86_FUNC(__imp__sub_00284330) {\n'
    hook=('\tstruct ProfileDrawScope { uint32_t previous; ProfileDrawScope(uint32_t owner) : previous(TrialFrontendProfileDrawOwner) { TrialFrontendProfileDrawOwner=owner; } '
          '~ProfileDrawScope() { TrialFrontendProfileDrawOwner=previous; } } profile_draw_scope(ctx.ecx);\n')
    if hook not in text:
        assert text.count(marker)==1;text=text.replace(marker,marker+hook,1)
    # Recompiled interior entry points repeat the tail; patch only the actual
    # keyboard update owner, not every generated copy of its return address.
    match=re.search(r'X86_FUNC\(__imp__sub_00284A50\) \{.*?^\}',text,re.S|re.M)
    assert match
    original=body=match[0]
    marker='X86_FUNC(__imp__sub_00284A50) {\n'
    hook='\tTrialGuestHook(ctx, base, 29);\n'
    if hook not in body:
        assert body.count(marker)==1;body=body.replace(marker,marker+hook,1)
        marker='\t// 00284AA0 push 0x1E'
        assert body.count(marker)==1
        body=body.replace(marker,'\tif (TrialGuestHook(ctx, base, 30)) goto loc_00284EF6;\n'+marker,1)
        marker='\t// 00285690 ret 0x08\n\tctx.esp += 12; return;'
        assert body.count(marker)==1
        body=body.replace(marker,'\tTrialGuestHook(ctx, base, 31);\n'+marker,1)
    text=text.replace(original,body,1)
    path.write_text(text)
    test=(local/'test_xbe_frontend.cpp').read_text()
    assert test.count('// ORIGINAL_PROFILE_WARNING_PRESENTATION')==1
    test=test.replace('// ORIGINAL_PROFILE_WARNING_PRESENTATION','\n'.join(warning_functions))
    # The private fixture gets the user's actual four localized warning pairs.
    # Neither text-bank contents nor extracted game strings enter release assets.
    from analyze_frontend import parse_archive,unpack_entry
    archive,entries=parse_archive(area/'assets/99-01.ent')
    bank=unpack_entry(archive,next(e for e in entries if e['name']=='99-01.exttxt'))
    languages,version=struct.unpack_from('<HH',bank)
    keys,lookups,strings,audio,count=struct.unpack_from('<5I',bank,4)
    block_size=keys+lookups+strings+audio
    assert languages==4 and 24+languages*block_size==len(bank) and keys==count*56
    cases=[]
    def cpp_bytes(value):return '"'+''.join(f'\\{c:03o}' for c in value)+'"'
    for language in range(languages):
        block=bank[24+language*block_size:24+(language+1)*block_size]
        for i in range(count):
            record=block[i*56:(i+1)*56];key=record[:40].split(b'\0')[0]
            if key not in (b'xboxtrc-memcard-1',b'xboxtrc-memcard-2'):continue
            variants=struct.unpack_from('<I',record,52)[0]
            offset=struct.unpack_from('<I',block,keys+variants)[0]
            value=block[keys+lookups+offset:].split(b'\0')[0]
            before,after=[(b'Xbox console',b'PC'),(b'la console Xbox',b'le PC'),
                          (b'Xbox',b'il PC'),(b'die Xbox',b'den PC')][language]
            assert value.count(before)==1
            expected=value.replace(before,after)
            cases.append('{'+','.join(map(cpp_bytes,(key,value,expected)))+'}')
    assert len(cases)==8 and test.count('// ORIGINAL_PROFILE_WARNING_CASES')==1
    test=test.replace('// ORIGINAL_PROFILE_WARNING_CASES',
        'const WarningCase warning_cases[]={'+','.join(cases)+'};')
    host=(local/'xbe_trial_graphics_ui.cpp').read_text()
    signature='void TrialFrontendPad(uint32_t buttons,uint8_t confirm,uint8_t back,int16_t x,int16_t y){'
    assert host.count(signature)==1
    start=host.index(signature)
    pad=host[start:host.index('\n}',start)+2]
    assert test.count('// PRODUCTION_FRONTEND_PAD')==1
    test=test.replace('// PRODUCTION_FRONTEND_PAD',pad)
    table=','.join(hex(x) for x in struct.unpack('<4I',read(0x26f688,16)))
    test=test.replace('// ORIGINAL_COMPLETION_TABLE',f'const uint32_t completion_table[4]={{{table}}};')
    # Original name constructor, validator, confirmation and parent Back owner.
    pins=[(0x2848D0,0x174),(0x2858A0,0x98),(0x2830A0,0xF8),(0x283500,0xDA),(0x276690,0xFA)]
    expected=['4277d12ad428f445e0d797f9d4339328d2ecfdec8ef736d8bae3a9819f3aaa11',
              '8599e8100a041ae50f502cd0ba1d4e653bb43fb3b0ecaadd121e82a987cd4c7d',
              '3110cd26edcf739c66020179db8aa6945fbf453719ba1d26e26584b63e4a8203',
              'c3582a555bb88bc8706bb1128c25e8d230ab486f7a60dbf5abedddf437df553e',
              '64d83be46aee5a8a6af349e264161854c27a3e0898614f7ac885c8fe3d278c2b']
    for (va,n),digest in zip(pins,expected):
        assert hashlib.sha256(read(va,n)).hexdigest()==digest,hex(va)
    functions=[]
    for address in ('002830A0','002858A0'):
        match=re.search(r'X86_FUNC\(__imp__sub_'+address+r'\) \{.*?^\}',text,re.S|re.M)
        assert match,address;functions.append(match[0])
    test=test.replace('// ORIGINAL_NAME_VALIDATION','\n'.join(functions))
    profile_pins=[(0x2840C0,0x250,'9defe235a595ab9948d14b797284b41957ffc01a776d753fcc619acc8fca6f14'),
        (0x284330,0x160,'bac062dc7a13564d3a9cac9e81d84f7f4f1917d66d411cc45c250584fcf8dd14'),
        (0x259690,0x100,'bbeef11023601bd2e6e419f30359e330f39069b86d11045451385b4ffc1087e5'),
        (0x503224,0x28,'b0ea5953390a716d9d99e06330e7d45917ee35a5d19b2ae720d280f53f866c4a')]
    for va,n,digest in profile_pins:
        assert hashlib.sha256(read(va,n)).hexdigest()==digest,hex(va)
    functions=[]
    for chunk,address in [(62,'00259690'),(67,'00284330'),(67,'002840C0')]:
        source=(area/f'generated/game/x86_recomp.{chunk}.cpp').read_text()
        match=re.search(r'X86_FUNC\(__imp__sub_'+address+r'\) \{.*?^\}',source,re.S|re.M)
        assert match,address;functions.append(match[0])
    test=test.replace('// ORIGINAL_PROFILE_PATH','\n'.join(functions))
    delete_functions=[]
    for chunk,va,n,digest in [
        (62,0x258BC0,0x173,'beefa76f7e8bdc4a9d502a6bc5b8a760d5a1d18c7f3507c5823898566dc886e4'),
        (62,0x258D40,0xAC,'ee9bc8c2fddc80818ad65bfeb1700d321c7a8a75647016af009fa7d40b6817ed'),
        (62,0x258DF0,0x69,'69bd3f2d6decf5b4b812d2e8846312c7836a931b582f11df669f8b9bec02e338'),
        (62,0x25A310,0x572,'d0b4077165f3fa49fc57d54574e267433d46ffa02cddeedfe0028f3a5ff98965'),
        (67,0x284580,0x24,'c2c021c7febc1473ffcf93a2e46e7f0baa0b10cd1003887d805e2e3a7a11053d')]:
        assert hashlib.sha256(read(va,n)).hexdigest()==digest,hex(va)
        source=(area/f'generated/game/x86_recomp.{chunk}.cpp').read_text()
        match=re.search(r'X86_FUNC\(__imp__sub_'+f'{va:08X}'+r'\) \{.*?^\}',source,re.S|re.M)
        assert match,hex(va);delete_functions.append(match[0])
    assert test.count('// ORIGINAL_PROFILE_DELETE_PATH')==1
    test=test.replace('// ORIGINAL_PROFILE_DELETE_PATH','\n'.join(delete_functions))
    constants=','.join(hex(v) for v in struct.unpack('<10I',read(0x503224,0x28)))
    test=test.replace('// ORIGINAL_PROFILE_CONSTANTS',f'const uint32_t profile_constants[10]={{{constants}}};')
    # Execute the backing's actual material/rectangle/half-pixel path, not just
    # the host helper's untransformed input coordinates.
    for va,n,digest in [
        (0x656D0,0x81,'d0bbcf01f129b1ff53e28e215f9ab9a09377407b96a2472d7940605a37c0037c'),
        (0x65550,0x89,'aab96479d66b4c8839b15e7563beadd6ed280c69ce36ebfd5c59f276fda68c5f'),
        (0x65170,0x98,'065c75529af885b1496619ccb3d328b405fb562dc7bf321db649260df5a2f553')]:
        assert hashlib.sha256(read(va,n)).hexdigest()==digest,hex(va)
    assert read(0x44BF94,4)==struct.pack('<f',1.)
    assert read(0x44C1F8,4)==struct.pack('<f',.5)
    source=(area/'generated/game/x86_recomp.9.cpp').read_text()
    functions=[]
    for address in ('00065170','00065550','000656D0'):
        match=re.search(r'X86_FUNC\(__imp__sub_'+address+r'\) \{.*?^\}',source,re.S|re.M)
        assert match,address;functions.append(match[0])
    assert test.count('// ORIGINAL_SIDEBAR_DRAW')==1
    test=test.replace('// ORIGINAL_SIDEBAR_DRAW','\n'.join(functions))
    (area/'src/compat/test_frontend.cpp').write_text(test)

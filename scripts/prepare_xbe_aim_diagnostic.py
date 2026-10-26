"""Bounded shot/camera/impact evidence; apply only to an opt-in private diagnostic."""
from pathlib import Path
import hashlib
import struct

INCLUDE='#include "xbe_aim_diagnostic.h"\n'
PINS=(
    (0xcab20,48,'48695a86fb1b23b20417d5ac3355f1a676234804f6ca925532b890d206d73fb2'),
    (0xcaf25,11,'37ca90ada85cdbea91ceb81677e9aae198256a6dd9cc746ddb56e5baf92cecdd'),
    (0xcb3c8,8,'920f1c2b0b6327fcf4c4f6d635c29966eafe1eaced527ab4d2f643b87bfc81d2'),
    (0xeb870,9,'b4a4e7f70a13b230482362f022ffad5334e73b198c1946084b1457271403a42e'),
    (0xebefe,7,'011ac6de7d58ba590b98fb2a1ad53db55e59f884e141b8ed0629d30978d52319'),
    (0x21527e,11,'4f9d6033b9a30fd6df723b91bee7a0fa9e3b9dc70419deb80ea0b5fc12e3f167'),
    (0x73aa0,11,'69755aea4ac89db056f26c0af2f1d87a52e7a908fb954ca90a67eaec9497b342'),
    (0x9d9b0,11,'72d7da1c92e42cfa3e1142ab7f6ddacdb2c77f672e298521364683163a4a7861'),
)
HOOKS={
    20:(('X86_FUNC(__imp__sub_000CAB20) {',
         'X86_FUNC(__imp__sub_000CAB20) {\n\tTrialAimFireScope trial_aim_fire(ctx,base);',1),
        ('X86_FUNC(__imp__sub_000CAF25) {',
         'X86_FUNC(__imp__sub_000CAF25) {\n\tTrialAimFireScope trial_aim_fire(ctx,base,true);',1),
        ('\tX86_PUSH32(0xCB3CDu); sub_000EBF10(ctx, base);',
         '\t{ TrialAimShotScope trial_aim_shot(ctx,base); X86_PUSH32(0xCB3CDu); sub_000EBF10(ctx, base); }',2),),
    23:(('X86_FUNC(__imp__sub_000EB870) {',
         'X86_FUNC(__imp__sub_000EB870) {\n\tTrialAimImpact(ctx,base);',1),
        ('\t// 000EBF00 call 0x000EBA90',
         '\tTrialAimBullet(ctx,base);\n\t// 000EBF00 call 0x000EBA90',1)),
    54:(('\t// 00215283 mov ecx, [0x005150B0]',
         '\tTrialAimRender(ctx,base);\n\t// 00215283 mov ecx, [0x005150B0]',1),),
    11:(('X86_FUNC(__imp__sub_00073AA0) {',
         'X86_FUNC(__imp__sub_00073AA0) {\n\tTrialAimReticleScope trial_aim_reticle(ctx,base);',1),),
    16:(('\tTrialHudQuadScope trial_hud_quad(ctx,base);',
         '\tTrialHudQuadScope trial_hud_quad(ctx,base);\n\tTrialAimQuad(ctx,base);',1),),
}

def clean(source,unit):
    source=source.replace(INCLUDE,'')
    for old,new,count in HOOKS[unit]:
        if new in source:
            assert source.count(new)==count,(unit,old)
            source=source.replace(new,old)
    return source

def unprepare(area):
    for unit in HOOKS:
        p=Path(area)/f'generated/game/x86_recomp.{unit}.cpp'
        if p.exists():
            before=p.read_text();after=clean(before,unit)
            if after!=before:p.write_text(after)

def prepare(area):
    area=Path(area);local=Path(__file__).parent
    xbe=(area/'assets/Default.xbe').read_bytes();base=struct.unpack_from('<I',xbe,0x104)[0]
    count,table=struct.unpack_from('<II',xbe,0x11c)
    sections=[struct.unpack_from('<6I',xbe,table-base+i*56)for i in range(count)]
    for address,size,digest in PINS:
        section=next(s for s in sections if s[1]<=address and address+size<=s[1]+s[4])
        start=section[3]+address-section[1]
        assert hashlib.sha256(xbe[start:start+size]).hexdigest()==digest,hex(address)
    prefix=None
    for unit,hooks in HOOKS.items():
        p=area/f'generated/game/x86_recomp.{unit}.cpp';source=clean(p.read_text(),unit)
        if unit==20:
            start=source.index('\t// 000CAB20 push ebp')
            end=source.index('\t// 000CAB50 call',start)
            prefix=source[start:end]
        for old,new,count in hooks:
            assert source.count(old)==count,(unit,old,source.count(old))
            source=source.replace(old,new)
        p.write_text(INCLUDE+source)
    for name in ('xbe_aim_diagnostic.cpp','xbe_aim_diagnostic.h'):
        (area/'src/common'/name).write_text((local/name).read_text())
    fixture=(local/'test_xbe_aim_diagnostic.cpp').read_text()
    assert prefix and fixture.count('// ORIGINAL_FIRE_PREFIX')==1
    fixture=fixture.replace('// ORIGINAL_FIRE_PREFIX',prefix)
    (area/'src/compat/test_aim_diagnostic.cpp').write_text(fixture)
    (area/'src/compat/test_aim_release.cpp').write_text((local/'test_xbe_aim_release.cpp').read_text())

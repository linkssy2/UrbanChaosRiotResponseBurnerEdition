"""Bounded shot/camera/impact evidence; apply only to an opt-in private diagnostic."""
from pathlib import Path
import hashlib
import struct
import re

INCLUDE='#include "xbe_aim_diagnostic.h"\n'
TARGET_SOURCE_SHA256='b825323f5092a034627affb083742c20ebd0978b0114cd9ef6439640ecc2de05'
PINS=(
    (0xc77d0,0x283,'793582295edb5f8d2e4544fb78a1b5f835ec58c07ad04ccc1912012b5f87cab7'),
    (0xcab20,48,'48695a86fb1b23b20417d5ac3355f1a676234804f6ca925532b890d206d73fb2'),
    (0xcaf25,11,'37ca90ada85cdbea91ceb81677e9aae198256a6dd9cc746ddb56e5baf92cecdd'),
    (0xcb3c8,8,'920f1c2b0b6327fcf4c4f6d635c29966eafe1eaced527ab4d2f643b87bfc81d2'),
    (0xeb870,9,'b4a4e7f70a13b230482362f022ffad5334e73b198c1946084b1457271403a42e'),
    (0xebefe,7,'011ac6de7d58ba590b98fb2a1ad53db55e59f884e141b8ed0629d30978d52319'),
    (0x21527e,11,'4f9d6033b9a30fd6df723b91bee7a0fa9e3b9dc70419deb80ea0b5fc12e3f167'),
    (0x73aa0,11,'69755aea4ac89db056f26c0af2f1d87a52e7a908fb954ca90a67eaec9497b342'),
    (0x9d9b0,11,'72d7da1c92e42cfa3e1142ab7f6ddacdb2c77f672e298521364683163a4a7861'),
    (0x223cde,48,'2319d651ce3d5920202583b18a6779c374cac3f5b0ea1d17f7cec9ab8acbea0d'),
    (0x456ae0,8,'3ac69e926885669ca55ba4c3086216bcf61d1071bfd284a08694f12e1ae30ff1'),
)
HOOKS={
    20:(('X86_FUNC(__imp__sub_000C77D0) {',
         'X86_FUNC(__imp__sub_000C77D0) {\n\tTrialAimTargetScope trial_aim_target(ctx,base);',1),
        ('\t// 000C77F5 movaps xmm0, [eax+0x20]',
         '\ttrial_aim_target.Shake(ctx,base);\n\t// 000C77F5 movaps xmm0, [eax+0x20]',1),
        ('\t// 000C7A4D pop edi',
         '\ttrial_aim_target.After(base);\n\t// 000C7A4D pop edi',1),
        ('X86_FUNC(__imp__sub_000CAB20) {',
         'X86_FUNC(__imp__sub_000CAB20) {\n\tTrialAimFireScope trial_aim_fire(ctx,base);',1),
        ('X86_FUNC(__imp__sub_000CAF25) {',
         'X86_FUNC(__imp__sub_000CAF25) {\n\tTrialAimFireScope trial_aim_fire(ctx,base,true);',1),
        ('\tX86_PUSH32(0xCB3CDu); sub_000EBF10(ctx, base);',
         '\t{ TrialAimShotScope trial_aim_shot(ctx,base); X86_PUSH32(0xCB3CDu); sub_000EBF10(ctx, base); }',2),),
    23:(('X86_FUNC(__imp__sub_000EB870) {',
         'X86_FUNC(__imp__sub_000EB870) {\n\tTrialAimImpactScope trial_aim_impact(ctx,base);',1),
        ('\t// 000EBF00 call 0x000EBA90',
         '\tTrialAimBullet(ctx,base);\n\t// 000EBF00 call 0x000EBA90',1)),
    54:(('\t// 00215283 mov ecx, [0x005150B0]',
         '\tTrialAimRender(ctx,base);\n\t// 00215283 mov ecx, [0x005150B0]',1),),
    11:(('X86_FUNC(__imp__sub_00073AA0) {',
         'X86_FUNC(__imp__sub_00073AA0) {\n\tTrialAimReticleScope trial_aim_reticle(ctx,base);',1),),
    16:(('\tTrialHudQuadScope trial_hud_quad(ctx,base);',
         '\tTrialHudQuadScope trial_hud_quad(ctx,base);\n\tTrialAimQuad(ctx,base);',1),),
    56:(('\t// 00223D0B call [ebx+0x04]',
         '\tTrialAimDecalPublish(ctx,base);\n\t// 00223D0B call [ebx+0x04]',1),),
}

def clean(source,unit):
    source=source.replace(INCLUDE,'')
    if unit==23:source=source.replace('\tTrialAimImpact(ctx,base);\n','')
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
    target=None
    instrumented_target=None
    decal=None
    instrumented_decal=None
    for unit,hooks in HOOKS.items():
        p=area/f'generated/game/x86_recomp.{unit}.cpp';source=clean(p.read_text(),unit)
        if unit==20:
            target=re.search(r'X86_FUNC\(__imp__sub_000C77D0\) \{.*?^\}',source,re.M|re.S).group()
            assert hashlib.sha256(target.encode()).hexdigest()==TARGET_SOURCE_SHA256,'C77D0 lifted body drift'
            start=source.index('\t// 000CAB20 push ebp')
            end=source.index('\t// 000CAB50 call',start)
            prefix=source[start:end]
        if unit==56:
            start=source.index('\t// 00223D08 push eax')
            end=source.index('loc_00223D0E:',start)
            decal=source[start:end]
        for old,new,count in hooks:
            assert source.count(old)==count,(unit,old,source.count(old))
            source=source.replace(old,new)
        if unit==20:
            instrumented_target=re.search(r'X86_FUNC\(__imp__sub_000C77D0\) \{.*?^\}',source,re.M|re.S).group()
        if unit==56:
            start=source.index('\t// 00223D08 push eax')
            end=source.index('loc_00223D0E:',start)
            instrumented_decal=source[start:end]
        p.write_text(INCLUDE+source)
    for name in ('xbe_aim_diagnostic.cpp','xbe_aim_diagnostic.h'):
        (area/'src/common'/name).write_text((local/name).read_text())
    fixture=(local/'test_xbe_aim_diagnostic.cpp').read_text()
    assert prefix and fixture.count('// ORIGINAL_FIRE_PREFIX')==1
    fixture=fixture.replace('// ORIGINAL_FIRE_PREFIX',prefix)
    # Run the complete original update with and without the actual injected hooks.
    # Candidate selection and the shake getter remain explicit fixture boundaries.
    for marker,function in (('// ORIGINAL_TARGET_UPDATE',target),('// INSTRUMENTED_TARGET_UPDATE',instrumented_target)):
        assert function and fixture.count(marker)==1
        body=function[function.index('{')+1:function.rindex('}')]
        body=body.replace('x86_icall(ctx, base, t)','AimTargetCall(ctx, base, t)').replace('sub_000C1A10(ctx, base)','AimTargetAutoaim(ctx, base)')
        fixture=fixture.replace(marker,body)
    # Exact final original CALL fragment: observer must preserve its arguments,
    # full CPU state and guest memory while recording the accepted quad.
    for marker,body in (('// ORIGINAL_DECAL_PUBLISH',decal),('// INSTRUMENTED_DECAL_PUBLISH',instrumented_decal)):
        assert body and fixture.count(marker)==1
        fixture=fixture.replace(marker,body.replace('x86_icall(ctx, base, t)','AimDecalCall(ctx, base, t)'))
    (area/'src/compat/test_aim_diagnostic.cpp').write_text(fixture)
    (area/'src/compat/test_aim_release.cpp').write_text((local/'test_xbe_aim_release.cpp').read_text())

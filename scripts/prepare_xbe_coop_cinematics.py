"""Present admitted campaign cinematics once, retaining their original lifecycle."""
from pathlib import Path
import hashlib
import struct
import re

PINS=(
    (0x215060,794,'a3710b17e87403e3138092af5081882c84c6af71c03b249ee4122961d611167d'),
    (0x215380,214,'12f02f0a14aa4baefa093cca221de9fe72d3bb71879ae04aab5e167b0c1f6a4c'),
    (0x21e0a0,39,'c8e5bd83be36c76cf9165bc1361a7d6e2b6bc7add87b8c48619ecbd008065e1d'),
    (0x173cf0,128,'6610e4e87e987b2290c4212f4600bbd19dd6838038871ec02af5a4c26d61b7bc'),
    (0x1ada40,64,'55626470cfefde7d4fbe88bebb83f5f1272e0c93993b0113e6fe9d5c16e23895'),
    (0x1abdf0,54,'926fcf3868f412203cf5460408baaf1729a72b4d7b3e910ed5d8b08188d82fb7'),
    (0x115914,27,'3c4adc3f27ee59d2e70173899ae112df4e27ec72a84c6e2fbefd88a9276ade5b'),
    (0x115000,384,'e5e7e3550f3d6747066b89b698b1ea9f5c67da69c6b116963603b015e519cd08'),
    (0xdfdf7,45,'64ff9160ddb1b817b3a301bf3b915d5fd29f40c062a13644ec6dfda82955dd2d'),
    (0x387aa,21,'77da974b70f5c0e7b303f8a0771d142bba8704665a0666782bc84f40b5a2afae'),
    (0x37228,41,'67037201570635ad7680d88afe2baac892aa78ab8f4008b60d2ad68839e7a335'),
    (0x37578,101,'7663e4b0d1b3ba9a7828b474f406d7933f40af75f3a63ce6df846af61b38de3a'),
    (0x375dd,260,'d3ee448aea234f22774c78a03931b8a8929cbb315d5ba5b5c929156efb4b665f'),
    (0x36760,101,'08fbb87f984ed59db6bfde520dac89aba9ecd640773b4e7f6bb249c8fb9b611c'),
    (0x338b0,20,'72965d30936f3b482f4a8c9b2b0f8f3846052d4283907c0669207cc19dea4b5f'),
    (0x4a6e0,35,'7bab26dac634b0ebce35adf5527719234dba176c77af4f0c2ea189deca027240'),
    (0x4a840,6,'e51598aae247076e5665f4a60dfcf3e4ed9a470afbcf020165528094ba6f6c58'),
    (0x4acd0,9,'3d88229ba8f81117aa293b673da37176d12ed5fdadb9eeb56bfda6da97e01c38'),
    (0x128f0,49,'9f8502beab04e0ba35c2ac9c9b385488271ffa2debceb39b619d540db76cd680'),
    (0x4a780,42,'7ed0295eacfc935c7d62322f41d5632b89ef70ce8f7bade3dc378a31f734922a'),
    (0x1726c4,16,'f043b1d5708fde1dd368adabb7ebeb12c9360740f65485e31c1c4636b9186671'),
    (0x44bf2c,4,'929ed883892128192e0c44e99fcd80778a6e40e4b75e07d49a0d21e5900b92f1'),
)
INCLUDE='#include "xbe_coop_cinematics.h"\n'
SCENE_SCOPE='\tTrialCoopCinematicView trial_coop_cinematic(ctx,base,true);\n'
HUD_SCOPE='\tTrialCoopCinematicView trial_coop_cinematic(ctx,base,false);\n'
SCENE_HOOK='\tif(trial_coop_cinematic.Scene()){ctx.esi=ctx.edi;goto loc_0021532D;}\n'
HUD_HOOK='\tif(trial_coop_cinematic.Hud())goto loc_00215429;\n'
DAMAGE_HOOK='\tTrialCoopDamagePlayer(ctx,base);\n'
KILLER_HOOK='\tTrialCoopKillcamKiller(ctx,base);\n'
HOOKS=(INCLUDE,SCENE_SCOPE,HUD_SCOPE,SCENE_HOOK,HUD_HOOK,DAMAGE_HOOK,KILLER_HOOK)

def unprepare(area):
    """Remove only our hooks before earlier preparers pin original source."""
    for unit in (4,22,54):
        path=Path(area)/f'generated/game/x86_recomp.{unit}.cpp'
        if not path.exists():continue
        old=path.read_text();source=old
        for hook in HOOKS:source=source.replace(hook,'')
        if source!=old:path.write_text(source)

def prepare(area):
    area=Path(area);local=Path(__file__).parent
    xbe=(area/'assets/Default.xbe').read_bytes()
    base=struct.unpack_from('<I',xbe,0x104)[0]
    count,table=struct.unpack_from('<II',xbe,0x11c)
    sections=[struct.unpack_from('<6I',xbe,table-base+i*56) for i in range(count)]
    for va,size,digest in PINS:
        section=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4])
        offset=section[3]+va-section[1]
        assert hashlib.sha256(xbe[offset:offset+size]).hexdigest()==digest,hex(va)
    path=area/'generated/game/x86_recomp.54.cpp';source=path.read_text()
    for hook in HOOKS:source=source.replace(hook,'')
    original=source.replace('\tif(TrialCoopSkipUnreadyView(ctx,base)){ctx.esi=ctx.edi;goto loc_0021532D;}\n','')
    original=re.sub(r'^.*// uc_prompt_context\n','',original,flags=re.M)
    witnesses=[]
    for name,first,last,digest,hook in (
        ('Scene',0x2151e0,0x21523a,'d55c7044c8db511d2e3b026d3843d2dfbd690b74c922d3d7e1233c1ac345949e',SCENE_HOOK),
        ('Hud',0x2153a2,0x215429,'afaa2521592b85a4a30ab2a863f68d76784e314bfb8d67121c0959cf54f28e29',HUD_HOOK),
    ):
        start=original.index(f'\t// {first:08X} ');stop=original.index(f'\t// {last:08X} ',start)
        part=original[start:stop]
        assert hashlib.sha256(part.encode()).hexdigest()==digest,name
        marker='\t// 00215222 ' if name=='Scene' else '\t// 002153EB '
        assert part.count(marker)==1
        for fixed in (False,True):
            body=part.replace(marker,hook+marker,1) if fixed else part
            # The actual loop continuation is represented by a return to the fixture.
            finish='\treturn;\nloc_0021532D: ctx.eax=0xffffffffu; return;\n' if name=='Scene' else ';\n'
            args='X86Context& ctx,uint8_t* base,TrialCoopCinematicView& trial_coop_cinematic'
            witnesses.append(f'void {"Coop" if fixed else "Original"}{name}({args}) {{\n\tX86_LOAD_FLAGS();\n\tuint32_t ca=0,cb=0;\n'+body+finish+'}\n')
    setter=(area/'generated/game/x86_recomp.55.cpp').read_text()
    first=setter.index('X86_FUNC(__imp__sub_0021E0A0)');last=setter.index('\nX86_WEAK_FUNC',first)
    setter=setter[first:last]
    assert hashlib.sha256(setter.encode()).hexdigest()=='106968e27052d50680829f46fc2efa5862f260bd86893a39a228e4b9aef5e767'
    setter=setter.replace('__imp__sub_0021E0A0','sub_0021E0A0',1)
    rewards=[]
    for unit,va,digest in (
        (6,0x4a840,'77e02a84134be3bac2bfcea70562957f6be493f348198ff2ef8a30e72d891f4b'),
        (6,0x4acd0,'5e658c29e77f331c045498dba19928e7629042aec91a0cc55482940bbc4e1a05'),
        (6,0x4a6e0,'09e8ad46976fd4c12882200c60a4bdbcc0c022c70d52efb1ec576496018e2df6'),
        (3,0x338b0,'d5184d97e14432bef4b5fe7b71c4e101a9b20cdaf244af295343b3bcc012c694'),
        (4,0x36760,'52db1909aa903af7495a96e541c1a2861f82e41980d38ab0a0570ca82a5a02e0'),
        (6,0x4a780,'68f5aefbda65aeea9fd3d9db3518b810f2fcf1cd5ee689b930bc7249bbefaa0e'),
        (0,0x128f0,'c6823c239ade3b2527ea4cbbc6e2716f33fffcbca3a75a46ca0f9606ee1c14cb'),
    ):
        s=(area/f'generated/game/x86_recomp.{unit}.cpp').read_text()
        a=s.index(f'X86_FUNC(__imp__sub_{va:08X})');z=s.index('\nX86_WEAK_FUNC',a);part=s[a:z]
        assert hashlib.sha256(part.encode()).hexdigest()==digest,hex(va)
        rewards.append(part.replace(f'__imp__sub_{va:08X}',f'sub_{va:08X}',1))
    # The real startup registration is last-writer-wins, not a fixed P1 ID.
    s=(area/'generated/game/x86_recomp.37.cpp').read_text()
    a=s.index('\t// 001726C4 ');z=s.index('\t// 001726D4 ',a);part=s[a:z]
    assert hashlib.sha256(part.encode()).hexdigest()=='ba8fbd67a3fef0c52a71a1b69867d5c79d224f37d8d08214fa9d26cbb1283400'
    rewards.append('void OriginalPlayerRegistration(X86Context& ctx,uint8_t* base) {\n'+part+'}\n')
    reward_units={}
    for name,unit,first,last,digest,anchor,hook,finish in (
        ('DamagePlayer',22,0xdfdf7,0xdfe24,'a4822e2fe58731fae93a47da8ce4e46502b2643e6e1a79f3b114b7025ecb4e3c',0xdfe07,DAMAGE_HOOK,''),
        ('KillcamKiller',4,0x387aa,0x387bf,'98428ab1010a96b7ff7ad3324d0d8fb4969b82ed25940c1caa1cfe4b791fb469',0x387b7,KILLER_HOOK,'ctx.eax=1;return;\nloc_00038A46:ctx.eax=0;return;\n'),
        ('HeadshotEligibility',4,0x37228,0x37251,'ed900392327a4305fc20de7917d56a80fc0080d9871fdac0346e03195dcce89a',0,None,''),
        ('HeadshotCount',4,0x37578,0x375dd,'4b81745606f1057805c62144a4350c1846d051ddd48660fce4d42d5423170950',0,None,'return;\nloc_00037802:;loc_000377EA:;loc_000377C3:return;\n'),
    ):
        if unit not in reward_units:
            s=(area/f'generated/game/x86_recomp.{unit}.cpp').read_text()
            for h in HOOKS:s=s.replace(h,'')
            reward_units[unit]=s
        s=reward_units[unit];a=s.index(f'\t// {first:08X} ');z=s.index(f'\t// {last:08X} ',a);part=s[a:z]
        assert hashlib.sha256(part.encode()).hexdigest()==digest,name
        if name=='HeadshotCount':
            # Retain the actual publication branches through the shared 73060
            # popup boundary, including the goal threshold and localized labels.
            stop=s.index('\t// 000376E1 ',z);popup=s[z:stop]
            assert hashlib.sha256(popup.encode()).hexdigest()=='cacaadad1a08f6a1314e7456ba21a9c7dc88337a1bea26379f448f92b14e868e'
            part+=popup+';\n'
        for fixed in ((False,True) if hook else (False,)):
            body=part.replace(f'\t// {anchor:08X} ',hook+f'\t// {anchor:08X} ',1) if fixed else part
            rewards.append(f'void {"Coop" if fixed else "Original"}{name}(X86Context& ctx,uint8_t* base) {{\n\tX86_LOAD_FLAGS();\n\tuint32_t ca=0,cb=0;\n'+body+finish+'}\n')
        if hook:
            marker=f'\t// {anchor:08X} ';assert s.count(marker)==1
            reward_units[unit]=s.replace(marker,hook+marker,1)
    for function,scope in (('00215060',SCENE_SCOPE),('00215380',HUD_SCOPE)):
        anchor=f'X86_FUNC(__imp__sub_{function}) {{\n';assert source.count(anchor)==1
        source=source.replace(anchor,anchor+scope,1)
    for anchor,hook in (('\t// 00215222 ',SCENE_HOOK),('\t// 002153EB ',HUD_HOOK)):
        assert source.count(anchor)==1;source=source.replace(anchor,hook+anchor,1)
    for hook in (SCENE_SCOPE,HUD_SCOPE,SCENE_HOOK,HUD_HOOK):assert source.count(hook)==1
    source=source.replace('#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+INCLUDE,1)
    for name in ('xbe_coop_cinematics.cpp','xbe_coop_cinematics.h'):
        (area/'src/common'/name).write_text((local/name).read_text())
    test=(local/'test_xbe_coop_cinematics.cpp').read_text()
    marker='// ORIGINAL_COOP_CINEMATIC_CALLERS';assert test.count(marker)==1
    test=test.replace(marker,setter+'\n'+'\n'.join(witnesses))
    marker='// ORIGINAL_COOP_REWARD_CALLERS';assert test.count(marker)==1
    test=test.replace(marker,'\n'.join(rewards))
    (area/'src/compat/test_coop_cinematics.cpp').write_text(test)
    path.write_text(source)
    for unit,s in reward_units.items():
        for hook in (DAMAGE_HOOK,KILLER_HOOK):assert s.count(hook)==int((unit==22)==(hook==DAMAGE_HOOK))
        s=s.replace('#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+INCLUDE,1)
        (area/f'generated/game/x86_recomp.{unit}.cpp').write_text(s)

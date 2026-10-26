"""Keep the retail delayed incapacitation award on its causal player's HUD."""
from pathlib import Path
import hashlib
import re
import struct

INCLUDE='#include "xbe_coop_ko_rewards.h"\n'
HOOKS={
    4:[('\t// 00038FA9 mov dword ptr [esi+0x9C], 0x00','\tTrialCoopKOCycleFinished(ctx,base);\n')],
    11:[('\t// 00073066 test ecx, ecx','\tTrialCoopKORecipient(ctx,base);\n')],
    22:[('\t// 000DDCD6 mov [ecx+0xB8], eax','\tTrialCoopKODamage(ctx,base);\n'),
        ('\t// 000DF43B mov dword ptr [esi+0x110], 0x02','\tTrialCoopKOCommitted(ctx,base);\n'),
        ('\t// 000E17D9 cmp eax, ebx','\tTrialCoopKODirectPlayer(ctx,base);\n'),
        ('\t// 000E18BF cmp eax, ebx','\tTrialCoopKODirectPlayer(ctx,base);\n'),
        ('X86_FUNC(__imp__sub_000E2FC0) {\n','\tTrialCoopKOForget(ctx);\n'),
        ('X86_FUNC(__imp__sub_000E2100) {\n','\tTrialCoopKOUpdateScope trial_coop_ko_update(ctx,base);\n')],
    41:[('X86_FUNC(__imp__sub_001998B0) {\n','\tTrialCoopKOEventScope trial_coop_ko_event(ctx,base);\n')],
}
BODIES=(
    (4,'00036800',178,'1f906ead035e8998bbf053478394208e60437e609029d5d586535476167bbb21','fd49a20dcc6f5dfa9abb16e97d0c3457e762db0b2a4cfaed69986ae61dffe268'),
    (6,'0004ACD0',9,'3d88229ba8f81117aa293b673da37176d12ed5fdadb9eeb56bfda6da97e01c38','5e658c29e77f331c045498dba19928e7629042aec91a0cc55482940bbc4e1a05'),
    (22,'000DDCA0',63,'6747108d8d4f39ea73301a7220a2317c68cd95bf4f21558ff39e9a6c4e026186','2e6d85706df168e035a87196cb6001bc92841ccc8ed22f4f7df43df462f7073c'),
    (22,'000DF380',254,'8ffd8a39fc7ab2dbad28b2890c8ffe75618291b6d875919905ce2c1fc6cd0036','7ba44f99705b2ed15328a25ef42863a4a81c3f6c415affcad514fecd1eeb8f9c'),
    (41,'001998B0',47,'caf4986ff6cafca5fe0324e9352650e6baa4c4b1aa9a1838ba6d182a1494fa20','a4df95e7b6a10bbbeb2e782e075e3594e6943d070199406b9ca97699ec77cc30'),
    (50,'001F90A0',56,'f7aa31f3736f27a11b0e94c7672dbb0a20a26401e7758b3140eb82cddbbef866','00c794ac196fd497ca8f5b3bb7492d420000e5a180b2357ba5b53b946905be16'),
    (11,'00073060',41,'9dbcda37bba9ebadf6a238e3bddec335b78a5be94c96261a39a0a21b2ea38789','087445535e851af02b0452af77100d84a5a0180accb5cd2bd474bd4f3567d44c'),
)
PINS=(
    (0x38fa6,21,'0475443203eaa19336be0f39593450690126dcef7f404fcce01c135c62797cc1'),
    (0xe17c2,27,'35d63481fcbafa0b43305f87a4636ee2041583f1427f08e25b9b4748633728aa'),
    (0xe253c,325,'64fb2ebd0d3471a4a2dfba84f522a5be58ab045e9650398abd0f0746ce687c1d'),
    (0x38221,69,'73188724546228a00cb733bdbf63bd970bbc0c0d3c10ce27f11dc83e8c4bcaf7'),
    (0xe18ae,94,'f85eded77e6b8da2a581cc7e5b41d34ab472288df5435d6c992bee6babd4a7ed'),
    (0xe2fc0,41,'5e7b48b8f9580bd6d85d89a70ae1020e9254733468bc871e40b8ecbd1e9ec5bd'),
)

def clean(source,unit):
    source=source.replace(INCLUDE,'')
    for _,hook in HOOKS.get(unit,[]):source=source.replace(hook,'')
    return source

def unprepare(area):
    for unit in HOOKS:
        path=Path(area)/f'generated/game/x86_recomp.{unit}.cpp'
        if path.exists():
            before=path.read_text();after=clean(before,unit)
            if before!=after:path.write_text(after)

def prepare(area):
    from prepare_xbe_coop_rewards import canonical_source
    from prepare_xbe_coop_turrets import strip as strip_turret
    area=Path(area);local=Path(__file__).parent
    xbe=(area/'assets/Default.xbe').read_bytes();base=struct.unpack_from('<I',xbe,0x104)[0]
    count,table=struct.unpack_from('<II',xbe,0x11c)
    sections=[struct.unpack_from('<6I',xbe,table-base+i*56)for i in range(count)]
    for va,size,digest in PINS+tuple((int(a,16),size,raw)for _,a,size,raw,_ in BODIES):
        s=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4]);offset=s[3]+va-s[1]
        assert hashlib.sha256(xbe[offset:offset+size]).hexdigest()==digest,hex(va)
    sources={u:clean((area/f'generated/game/x86_recomp.{u}.cpp').read_text(),u)
             for u in set(HOOKS)|{u for u,_,_,_,_ in BODIES}}
    test=(local/'test_xbe_coop_ko_rewards.cpp').read_text()
    for unit,address,_,_,digest in BODIES:
        source=canonical_source(sources[unit],unit)
        if unit==6:source=source.replace('\tTrialHealthPackMedicPlayer(ctx,base,false);\n','')
        if unit==41:source=strip_turret(source) # Original witness only; retain runtime wrappers.
        body=re.search(r'X86_FUNC\(__imp__sub_'+address+r'\) \{.*?^\}',source,re.M|re.S).group()
        assert hashlib.sha256(body.encode()).hexdigest()==digest,address
        for anchor,hook in HOOKS.get(unit,[]):
            if anchor in body:body=body.replace(anchor,anchor+hook if anchor.startswith('X86_FUNC')else hook+anchor,1)
        marker='// ORIGINAL_'+address;assert test.count(marker)==1
        test=test.replace(marker,body)
    source=sources[22];start=source.index('\t// 000E253C ');stop=source.index('\t// 000E2681 ',start)
    tail=source[start:stop]
    assert hashlib.sha256(tail.encode()).hexdigest()=='a9fe2dc09f27996c381d357a604452645c51ff4b708b808a71b6a65876544b59'
    tail+='\tctx.edi=X86_MEM32(ctx.esp);ctx.esi=X86_MEM32(ctx.esp+4);ctx.esp+=0x4c;\n'
    test=test.replace('// ORIGINAL_DELAYED_AWARD','X86_FUNC(DelayedAward) {\n\tTrialCoopKOUpdateScope trial_coop_ko_update(ctx,base);\n\tX86_LOAD_FLAGS();\n\tuint32_t ca=0,cb=0;\n'+tail+'}\n')
    gates=[]
    for name,first,last,branch,digest in (
        ('DirectKO',0xe18ae,0xe18c3,0xe192b,'971c51a17a488ed278ecae9a868e5b56dbc5c6293a42cc2913ad1649682c3b60'),
        ('ImpactPlayer',0xe17c2,0xe17dd,0xe1838,'d03c1125d4013e8474c1020a498df6a9c3790bf221a66c0b920e979d610dd7af'),
    ):
        part=source[source.index(f'\t// {first:08X} '):source.index(f'\t// {last:08X} ')]
        assert hashlib.sha256(part.encode()).hexdigest()==digest,name
        for fixed in (False,True):
            body=part
            if fixed:
                anchor='\t// 000E18BF ' if name=='DirectKO' else '\t// 000E17D9 '
                body=body.replace(anchor,'\tTrialCoopKODirectPlayer(ctx,base);\n'+anchor,1)
            gates.append(f'X86_FUNC({"Coop" if fixed else "Original"}{name}) {{\n\tX86_LOAD_FLAGS();\n\tuint32_t ca=0,cb=0;\n'+body+
                         f'ctx.eax=1;return;\nloc_{branch:08X}:ctx.eax=0;\n}}\n')
    test=test.replace('// ORIGINAL_IMPACT_GATES','\n'.join(gates))
    source=canonical_source(sources[4],4)
    a=source.index('\t// 00038FA6 ');z=source.index('\t// 00038FBB ',a);body=source[a:z]
    assert hashlib.sha256(body.encode()).hexdigest()=='12e38a3ccb884dc01b5b97250540b6d932b303edde7a5afd5a4df36ebf512af0'
    anchor,hook=HOOKS[4][0];body=body.replace(anchor,hook+anchor,1)
    test=test.replace('// COOP_DAMAGE_END','void CoopDamageEnd(X86Context& ctx,uint8_t* base) {\n'+body+'}\n')
    for unit,seams in HOOKS.items():
        source=sources[unit]
        for anchor,hook in seams:
            assert source.count(anchor)==(2 if unit==4 else 1),(unit,anchor)
            source=source.replace(anchor,anchor+hook if anchor.startswith('X86_FUNC')else hook+anchor)
        source=source.replace('#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+INCLUDE,1)
        (area/f'generated/game/x86_recomp.{unit}.cpp').write_text(source)
    for name in ('xbe_coop_ko_rewards.h','xbe_coop_ko_rewards.cpp'):
        (area/'src/common'/name).write_text((local/name).read_text())
    (area/'src/compat/test_coop_ko_rewards.cpp').write_text(test)

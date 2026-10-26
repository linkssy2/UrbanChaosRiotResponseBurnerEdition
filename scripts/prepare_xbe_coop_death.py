"""Co-op downed players, original ragdolls and original actor-reset revival."""
from pathlib import Path
import hashlib
import re
import struct

PINS = (
    (0xc7f40,1541,'13e200fb09a2086e02c0f949db37a820ee41b1e9f8b309989453f2622517106d'),
    (0xe5310,688,'cf689f11153d8eb060b53bf04c0770a596d89314f49eab9c8af2d60a0ab74085'),
    (0xe7830,926,'c6dd34e7bb7cd3efa560a793878896b5015cd527efa02822cef228eec922dc8d'),
    (0xd35a0,818,'a2dddc25813498dac2ae9b17b7350e2784bd5c864654e9820d63c4c9bd87e9a1'),
    (0xbdbe4,14,'9d52b19829992cb6843880b73fc4f51990ff76cc106a5ed63d6cdb224155b0dd'),
    (0xcd04f,332,'744be5980aef98afae4a37cf961c930c08c02977552123e537ab67cdc414523c'),
    (0xd4f19,14,'bf165986f30a4bf7d189986c965faa652569da8d2b31c6afbbb382fc5a96fb1b'),
    (0x49660,34,'d19cf41e47a30f81f16d137f0cc0c5d94632fd351905718709fb179b0c3f5e31'),
    (0xb5e80,10,'f830869d60400819128314aafe38f1f20b865adca35ba1f829d168e22afca9ae'),
    (0xbdab0,77,'45ece1835ce41f21433c855890584d7a17ae0130c27d1238e41bd06ff0bfff78'),
    (0x2180a0,34,'424f6d55ac90e45c2d9491ae63804bd7142dea228a27fd7b86ca8ad55251ec6a'),
    (0x218b30,29,'8e9dca95bcff218688ecd45faf07a7bc615c0d134676f78a5aa7156dc73b2936'),
    (0xbd150,150,'de4b93b6d36e6c42e732305f07c146c4d05e6c1cb82b888f1896ee64812fa0f8'),
    (0xd38e0,34,'871e38ce88f800ab68ad43c0e1ef0928097700eacd32095837c747034c9cca19'),
    (0x2a42b0,45,'bc061c5d684f5e915b7b8fcefa1da8e2cda00473b56c132a7ca0482cec810204'),
    (0x2a2910,86,'cf9fd3c66af155ff003a9f2167c28669771553c6e41b4dfd6a9217439d49fed9'),
    (0x44d024,4,'0d66bd808e9247acb560fe33efaa62ad6f11ee6f85cc4f61bd17673263de04ee'),
    (0x208f80,809,'c67006b0cf0384dbd2d9079168c647d38d00a9fd808da1dc6fe70d703ac863af'),
    (0x55610,99,'a1cfbffb668c81ad991efa66ca9270e7620c8b7d70ae3561bc8ca0060eee6c3e'),
    (0x1f9010,7,'9b2fef42d4d310ac37a9037bf5e4b6d0da15b5c165d135674de5c7a57ab6b0c6'),
    (0x69280,363,'7001a3ab419d8f50fd42ca89f98ec09308ff69cf232a6a05d9e8f1142b77035e'),
    (0x45270c,4,'159cb4c742e3029725d946fcb24e8262e0ff12a3755a28e518f5f3e712227dc5'),
    (0x4e6b0,153,'fa2545e3122323323e665e9dc26a7b5336524c569aeb3d2c330101f05fdecb61'),
    (0x4e641,11,'c6f9d709d395d73e0df94c58c41a6f0f7fb14a0da2254a5c92aada29032b8abe'),
    (0x4504d0,11,'e8ab1c700427a613dd1ceb1bc15ae6324f398334b988ebecbd2ce4c8c394d93f'),
    (0x487acc,26,'7b1e05fbeb994b47d75938be2f75e0e18574b01d5003a1a5541ff7e3f3c14fbc'),
    (0x4e620,33,'a6876c65b38586de93ebe56e7604fd9cb1cc2b8f7bab78dcfec8ac77cb2f6753'),
    (0x143193,28,'a45356066e4d0e397da171cdce2651188ede1dc4ffe93b2071a3c61694a1c57a'),
    (0x4c9da,26,'1b42afc51c4cc2e574feab32f95f6ab696d452c3f4393e1cebc9c861ca043785'),
)
POLICY='\t{const unsigned death=TrialCoopDeathDisposition(base,ctx.esi);if(death==1)goto loc_000C84EB;if(death==2)goto loc_000C8461;}\n'
HOOKS={
    20:[('\t// 000C8030 test eax, eax','\tTrialCoopDeathBody(ctx,base);\n'),
        ('\t// 000C809D mov ecx, eax','\tTrialCoopDeathBody(ctx,base);\n'),
        ('\t// 000C81E4 push 0x4531B0',POLICY)],
    23:[('\t// 000E5339 jnz 0x000E5379','\tif(TrialCoopImmediateRagdoll(base,ctx.esi))goto loc_000E5379;\n'),
        ('\t// 000E7960 mov edx, [esi]','\tif(TrialCoopKeepDownedBody(base,ctx.esi))goto loc_000E796F;\n')],
    52:[('\t// 00209258 mov ecx, [ebp+0x04]','\tTrialCoopDeathUpdate(ctx,base);\n')],
}
REGIONS=(
    (20,'000C8026','000C80A4','f65ad78eee8f32a3f83b41a6f9649243b19d444dcdb735105f12f1f1917fc4ad','DeathBodyCaller'),
    (23,'000E532E','000E533B','153a8ce63f82cc64bc63589f431d7f499c23759dcdd51258374dac90f1123ca9','RagdollBranch'),
    (23,'000E7953','000E7967','866b9c9a8eab41fbe98c2b4840f079ff215ecd88ff1c80ca541d36a84634ada1','BodyExpiry'),
    (52,'00209258','00209261','007a91288813d21bb2ab20c4722afc68fab238abdfcbc9f52e614753c57b4e47','DeathTick'),
)
def section(source,first,last):
    a=source.index('\t// '+first+' ');z=source.index('\t// '+last+' ',a)
    return source[a:z]

def fixture(name,text):
    tail=''
    if name.endswith('RagdollBranch'):
        tail='\t++animation_branches;return;\nloc_000E5379:\n\t++ragdoll_branches;\n'
    elif name.endswith('BodyExpiry'):
        tail='loc_000E796F:\n\t;\n'
    elif name.endswith('DeathBodyCaller') and 'loc_000C80A4:' not in text:
        tail='loc_000C80A4:\n\t;\n'
    if name=='CoopDeathTick':text='\tTrialCoopDeathUpdate(ctx,base);\n'+text
    return 'X86_FUNC('+name+') {\n\tX86_LOAD_FLAGS();\n\tuint32_t ca=0,cb=0;\n'+text+tail+'}\n'

def unprepare(area):
    """Remove only this module's seams before earlier original-source pins run."""
    area=Path(area)
    include='#include "xbe_coop_death.h"\n'
    for chunk,seams in HOOKS.items():
        path=area/f'generated/game/x86_recomp.{chunk}.cpp'
        if not path.exists():continue
        before=path.read_text();source=before.replace(include,'')
        for hook in dict.fromkeys(hook for _,hook in seams):source=source.replace(hook,'')
        if source!=before:path.write_text(source)

def prepare(area):
    area=Path(area);local=Path(__file__).parent
    xbe=(area/'assets/Default.xbe').read_bytes();base=struct.unpack_from('<I',xbe,0x104)[0]
    count,table=struct.unpack_from('<II',xbe,0x11c)
    sections=[struct.unpack_from('<6I',xbe,table-base+i*56) for i in range(count)]
    for va,size,digest in PINS:
        s=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4])
        start=s[3]+va-s[1]
        assert hashlib.sha256(xbe[start:start+size]).hexdigest()==digest,hex(va)
    for name in ('xbe_coop_death.h','xbe_coop_death.cpp'):
        (area/'src/common'/name).write_text((local/name).read_text())
    include='#include "xbe_coop_death.h"\n';chunks={};extracted=''
    for chunk,seams in HOOKS.items():
        path=area/f'generated/game/x86_recomp.{chunk}.cpp'
        source=path.read_text().replace(include,'')
        for hook in dict.fromkeys(hook for _,hook in seams):source=source.replace(hook,'')
        for unit,first,last,digest,name in REGIONS:
            if unit!=chunk:continue
            original=section(source,first,last)
            assert hashlib.sha256(original.encode()).hexdigest()==digest,first
            extracted+=fixture('Original'+name,original)
        for anchor,hook in seams:
            assert source.count(anchor)==1,anchor
            source=source.replace(anchor,hook+anchor,1)
        for hook in dict.fromkeys(h for _,h in seams):
            assert source.count(hook)==sum(h==hook for _,h in seams)
        for unit,first,last,_,name in REGIONS:
            if unit==chunk:extracted+=fixture('Coop'+name,section(source,first,last))
        source=source.replace('#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+include,1)
        path.write_text(source);chunks[chunk]=source
    test=(local/'test_xbe_coop_death.cpp').read_text()
    assert test.count('// ORIGINAL_DEATH_REGIONS')==1
    test=test.replace('// ORIGINAL_DEATH_REGIONS',extracted)
    for unit,address,digest in (
        (10,'00069280','0948871151a68a27863d5acab09a485c49dfa813d52912e7176538d47d44f6a0'),
        (21,'000D35A0','92d3177b31eb036cc1d363eecb5a09f162c305923e950bf6fa966c2c3251d739'),
        (19,'000BDAB0','9c508f4d4592e6233f97c5051efa1298678d01fd0b9a51f6bba706501803fa7f'),
        (54,'002180A0','934c1d1bd89f0665eda05195aa1bf8499cb202e98ee7e71798924aca9d600037'),
        (19,'000BFE00','44e64295aecf422b0b4cf423dc7465690bb3533e8e8f61e2b126e87f1dc577e1'),
        (19,'000BD150','95ed5cf2a54b64185379686e64c6bf0bda40f288d5b36bc1dba95f14b453dd6f'),
        (6,'00049660','db98a55cc360b1956e611b4c3b55e50396c44d246c5bfc99538f7e3eec690954'),
        (18,'000B5E80','0bf211fd2ae337f43ae73e75cd2fcc133b040f089822fb9568eced25f5acbccd'),
        (71,'002A42B0','02124b12fd4f28d07a58fb41cedaf9fa3a406635c295e59b1264b26c52662a36'),
        (70,'002A2910','c58bf9863e1de991f97ba2585544e530cc9b5505e39b2569803a39b89c07c287')):
        source=(area/f'generated/game/x86_recomp.{unit}.cpp').read_text()
        f=re.search(r'X86_FUNC\(__imp__sub_'+address+r'\) \{.*?^\}',source,re.M|re.S).group()
        assert hashlib.sha256(f.encode()).hexdigest()==digest,address
        marker='// ORIGINAL_'+address;assert test.count(marker)==1
        test=test.replace(marker,f)
    visibility=''
    for unit,first,last,digest,name in (
        (19,'000BDBE4','000BDBF2','2174c95bee8abcde59d5815b8e08e385910d67c524b04a7b98af65fcce4a0b45','DeadVisibility'),
        (21,'000D4F19','000D4F27','e15c589cfb66d3d69a567135955e671483722396eeea3f2385c8ca615de717a6','SpawnVisibility'),
        (20,'000CD04F','000CD19B','5645fbb2b708e0171864b9c1906b1666b7b774e103bb646807af429acdab1794','FirstPersonVisibility')):
        source=(area/f'generated/game/x86_recomp.{unit}.cpp').read_text()
        part=section(source,first,last)
        assert hashlib.sha256(part.encode()).hexdigest()==digest,first
        if name=='FirstPersonVisibility':part+='\t;\n'
        visibility+=fixture('Original'+name,part)
    assert test.count('// ORIGINAL_VISIBILITY_REGIONS')==1
    test=test.replace('// ORIGINAL_VISIBILITY_REGIONS',visibility)
    source=(area/'generated/game/x86_recomp.6.cpp').read_text()
    consumer=''
    part=section(source,'0004E6B0','0004E749')
    assert hashlib.sha256(part.encode()).hexdigest()=='737f2e137e38a20239642fdeb584983dbad0f1568a85f0a51fa6f0b97b628ed7'
    consumer+=part
    part=section(source,'0004E641','0004E64C')
    assert hashlib.sha256(part.encode()).hexdigest()=='29b988f2fc3530f98b33f0a0e698ff76771d202cfc2499351b72aeacfb3642f6'
    consumer+='\tctx.eax=0;return;\nloc_0004E641:\n'
    consumer+=part
    consumer+='\t;\nloc_0004E871:ctx.eax=0;return;\n'
    test=test.replace('// ORIGINAL_FAILURE_CONSUMER',fixture('OriginalFailureConsumer',consumer))
    dead=section(source,'0004E620','0004E641')
    assert hashlib.sha256(dead.encode()).hexdigest()=='04e9afd164a768597ee7404c54b0aa32ad6e9d4b95cbfd538d522534ebe3a49e'
    dead+=section(source,'0004E641','0004E64C')+'\t;\nloc_0004E871:ctx.eax=0;return;\n'
    test=test.replace('// ORIGINAL_DEAD_FAILURE_CONSUMER',fixture('OriginalDeadFailureConsumer',dead))
    # Execute the exact maintained routing hook, with the original destinations
    # instrumented by the fixture. Original state/health/reset run separately.
    test=test.replace('// DEATH_DISPOSITION_HOOK',POLICY)
    (area/'src/compat/test_coop_death.cpp').write_text(test)

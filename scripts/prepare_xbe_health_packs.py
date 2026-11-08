"""Stored medic supplies using the original handoff, stock and player healing."""
from pathlib import Path
import hashlib
import struct

PINS=(
    (0x5aab0,6,"82a6f08404d9c41eb2d1437c1fe102f4c25209e68bea2833795ddaf6641ca0c7"),
    (0x5b140,7,"2783f77e759c990df3d7ae85040ccc1b0885a0285cf1d92254fab5af72f754e8"),
    (0x64400,697,"d860b3feedd2d3897305962f2b90cf65a22e021d03784c7a601f6834f90e0f33"),
    (0xbdab0,77,"45ece1835ce41f21433c855890584d7a17ae0130c27d1238e41bd06ff0bfff78"),
    (0x646bc,56,"00ef2b348fce0edbd6f22915c689753f03d4000f68c59513ef9d97ff32aba674"),
    (0x643e0,6,"2f50be2683201136880faa73c01094369ebb3428808d2188c528381e7a624d6a"),
    (0x167f3,8,"cf544b24ed2963a18e4c6355659272bff6c3e5e52fc53a702444a933313493ae"),
    (0x1688b0,0x84,"dc01f2253b9ee220865f62e4065b24edcef98ac5dd66857d9b8d73fe404df09b"),
    (0x168d58,0x1b,"342c97c11a9e89f0b6d830d75533c67e653ce46d30f5037a4a572cfdc6612df4"),
    (0x168f20,0xac,"8d00899f2fea392b9f1c49cd4526abaa3037fe6dbd330a1fae19948abe24486a"),
    (0x20a230,0x2e,"46ed0f6bb09ef6c4111f864d06722430432e556ee4661018ac7a54c8c2ef7f9c"),
    (0x70d90,455,'879111defca52dd723604dadc820a063cf5705351be989ed3f38d58867be6671'),
    (0x76a50,736,'5d703977efb27bb1283f614464c7a46dcf7269c5af635b34aa5cc119268d030e'),
    (0x4af40,0x58,"a882f5e7b60a51f78d55c9a30af24bb6c5e86204086bb031fdf67460c382af6a"),
    (0x73de6,0x7c,"c2cdc041da11aacca6fdc3f954e778412faf1e4343abb65a322c35cfe326f73a"),
    (0x22d360,7,"0b1e1d37d7cbdbd10d4c974017dd477e62ee9c4ebbd5bb19cd91d832621c5606"),
    (0x48154c,32,"36dd16ee1078fe42e0ed1fbdf457680dcda32e73308ece420fa1ee6bbddac0db"),
    (0x2223d0,120,"8c3e7d6f4f8b35dc6a418f3aeba18cb2e2c2af28ba802a141b19878efeea4af5"),
    (0x6bac0,87,"d101e68c97c4a12196f932013f380e1e10af4738b21ba039c26045cee64b8869"),
    (0x4531a0,0xc,"191b35b27aa8ba753c68868de9de52a3cd944940b3d1e810a6fe1693468b42f9"),
    (0x450588,0x38,"8c0bd52d6656740e68af63306e989e90876303d3d90d2b1b955fcc1d5ed6bab6"),
    (0x44bf20,0x38,"40ee4d7d4fe0099b53ac58d654cf88ea4485483ff9d61bab2d83f3d88a8c80e4"),
    (0x4526f0,0x38,"3d180eba33b6c359e7054024c3ee2b04ccd726902310e7c325fd8b2bc19c87ce"),
    (0xbd650,0xa,"cfd366f5698bced6dbd162e2dbe38a32551f7c29d9686a8756dc3c5dab3b70f3"),
    (0x169570,0x35,"e6d4a08bc38db05c86e135cdf601ff29216563741c74c89290172c18098ebc9a"),
    (0x182b60,0xb4,"fe3151a9bae795bc7eb7ae62d4c4cfc9d1003a0688db41f7e133caed82522b94"),
    (0x4acd0,0x9,"3d88229ba8f81117aa293b673da37176d12ed5fdadb9eeb56bfda6da97e01c38"),
    (0x4ace0,0x15,"a245d89233c0962e974fa9d392d9d163b08b1a6a9fb9f9c7ee02a92a75aed044"),
    (0x1c5b96,0xf,"35e49cb702c849591ec4014b578178a73dc45a8c55bcf4c2a1f3908b4e17f1f5"),
    (0x1c59a0,0x13,"11ff11500b8472e8ed7f7c1bc783ebfd38ad331e8e12ff65a28e660f0a6ee9cd"),
    (0x168b80,0x13,"78bddcf72b4736b97c48e04ff4008de192815e09b3aced424a11fa93fc775823"),
    (0x182c20,0x13,"c8e20c52c494d3b1e07682353adfe001368cdec12d3ed63cd14292ce7d2b096f"),
    (0xd2ab0,0xf,"2c0a32f728d14663320296e782d1791e322bcbdd020db1223b276cf68f2e5c4d"),
    (0xd2bb9,0x82,"4662e6c1fa57cab742aa31937080fae9768338164a603326398b1e61ad8809db"),
    (0x2180a0,0x22,"424f6d55ac90e45c2d9491ae63804bd7142dea228a27fd7b86ca8ad55251ec6a"),
    (0x218120,7,"c9af73197615758bcbfce6464123bda69bcd94dae6cc0bde4066de331201cee1"),
    (0x44c6c4,4,"5d5d32c7cf8e2ca77d9af6bf35a0ba5cd09074ccb4f3871aba6f67420b324a57"),
    (0x46fc4c,4,"259cb45bfb2f4ed5b1813eeec0c13cefe3d9530c448199c2c2dd14b6f5296d33"),
    (0x172f49,0x9,"6580b55f8fed49d2120c507152beb572e615f9c2e4e92615c806a09a8c18952d"),
    (0x172e80,0x13,"65156691a5a6ea0b16cc7e3912ae4983e23d28779210d20ed6b147b8943de27d"),
    (0x168a40,0x13,"0c06c9de8a82df5264c170136c6209faf8c972b4878d338409f7fd6ce6c17312"),
    (0x4c330,0x1e,"bd9426179f5df7f3a920b61dbf768102366002320082a454d2ece195dd70e24d"),
    (0xcca4c,0x28,"b307f00a607d87d45154c161cbfaa86ba8eca316f4246bb2fe9786d16b484020"),
    (0x64230,0x47,"a212462cad730cd91b48edc61275faba9b6005fdc51da7ee76e0cfd56d36aecf"),
    (0x2431b0,0x4,"8acf161f52368e8c95efb689a150a798b080039ec8193d4aa411a87496346c6a"),
    (0xcd980,0x13c,"6bf16c0c4e10817bf98f94ba93906a2c61ebf3f13e85d415158df2b37361ef61"),
    (0xd2f20,0x2c,"51a286e732187c8bfba2f1b40c80f8e65550b1d75b1fe99ea60fc61304cf9217"),
    (0xd3e89,0x17,"7d7b420c7c0bd803c83ab9420257eb7d60d9935de0f5ad1a4c1e1f1ee63042c4"),
    (0xbfe00,0xac,"6d891588e23ad6a53276aa736bbbc5073dfabdfdd03ce66153b0fd1d482d627e"),
    (0x458a48,0x50,"dbc156c7485b61f55014575cd1004bdeb02d66d6f351f842f7155823dd3be547"),
    (0x459048,0x5,"0c4a836afe1bb889afed5386c62ea46650a245dbfdbe2e964ff1684aa0822149"),
    (0x22b930,0x2c,"478e9388de01344ce247fdd2eb1ad46759592cbddf5b9deb3e2da59a67e00d2a"),
    (0x22b8d0,0x5b,"f39c53d82b1649a9225a86d70e00bdbebaaa68cc52b2176e5ef82d48beb9f9a4"),
    (0xc77d0,0x283,"793582295edb5f8d2e4544fb78a1b5f835ec58c07ad04ccc1912012b5f87cab7"),
    (0x4500e4,4,"df809649a8ba650dc4e549430e7139763b9e266a889d35e7c2fb8a40032a82c8"),
    (0x44bf94,4,"e00e5eb9444182f352323374ef4e08ebcb784725fdd4fd612d7730540b3e0c8c"),
)
ENTRY={
    19:{'000BD650':'\tif(TrialHealthPackMedicReceipt(ctx,base)){ctx.esp+=12;return;}\n'},
    21:{'000D2AB0':'\tTrialHealthPackUpdate(ctx,base);\n'},
    36:{'00168B80':'\tTrialHealthPackMedicScope trial_health_medic(ctx,base);\n',
        '00168A40':'\tTrialHealthPackMedicScope trial_health_medic(ctx,base);\n',
        '00168F20':'\tTrialHealthPackMedicScope trial_health_medic(ctx,base);\n\tTrialHealthPackMedicEnd(ctx,base);\n',
        '00169570':'\tif(TrialHealthPackMedicStockBlocked(ctx,base)){ctx.esp+=12;return;}\n'},
    37:{'00172E80':'\tTrialHealthPackDestroyed(ctx,base);\n'},
    38:{'00182C20':'\tTrialHealthPackMedicScope trial_health_medic(ctx,base);\n',
        '00182B60':'\tTrialHealthPackDeliveryScope trial_health_delivery(ctx,base);\n\tif(trial_health_delivery.intercept&&!trial_health_delivery.accepted){ctx.esp+=4;return;}\n'},
    45:{'001C59A0':'\tTrialHealthPackMedicScope trial_health_medic(ctx,base,true);\n'},
}
SEAMS={
    6:((0x4acd6,'\tTrialHealthPackMedicPlayer(ctx,base,false);\n'),
       (0x4ace6,'\tTrialHealthPackMedicPlayer(ctx,base,true);\n'),
       (0x4c344,'\tTrialHealthPackCheckpointSaved(ctx,base);\n')),
    20:((0xcca6c,'\tTrialHealthPackCheckpointLoaded(ctx,base);\n'),),
    21:((0xcd98e,'\tif(TrialHealthPackInjectionContact(ctx,base))goto loc_000CD9DE;\n'),
        (0xcda75,'\tTrialHealthPackInjectionClip(ctx,base);\n')),
    37:((0x172f4e,'\tTrialHealthPackCreated(ctx,base);\n'),),
}
OLD='\tif ((cf | zf)) { goto loc_001C5C1E; }'
NEW='\tif (!TrialHealthPackMedicNeeds(base,!(cf | zf))) { goto loc_001C5C1E; }'
EXTRACTED_HASHES={
    '000BD650':'91f707e28dc390172201312edc9fceccf2a98bb696f1217ea9d0459a75487450',
    '00169570':'55df822ceb81a2657a2d7395a9b92413ac733cb1113610f957d380f1ecfe1dce',
    '00182B60':'f67bf687417649149b87146a74c8b222ec87f91059c2dd09292b64d9d35df8ad',
    '0004ACD0':'5e658c29e77f331c045498dba19928e7629042aec91a0cc55482940bbc4e1a05',
}

def clean(source,unit):
    source=source.replace('#include "xbe_health_packs.h"\n','').replace(NEW,OLD)
    for hook in sorted(ENTRY.get(unit,{}).values(),key=len,reverse=True):source=source.replace(hook,'')
    for _,hook in SEAMS.get(unit,()):source=source.replace(hook,'')
    return source

def unprepare(area):
    """Run before earlier preparers whose exact function pins own these seams."""
    for unit in sorted(set(ENTRY)|set(SEAMS)):
        path=Path(area)/f'generated/game/x86_recomp.{unit}.cpp'
        if path.exists():
            before=path.read_text();after=clean(before,unit)
            if before!=after:path.write_text(after)

def prepare(area):
    area=Path(area);local=Path(__file__).parent
    bridge=(local.parent/'build/upstream-review-xboxxbe-20261014/runtime/cpu/dispatch.cpp').read_text()
    a=bridge.index('uint32_t CallGuest(');z=bridge.index('\n}\n',a)+2
    bridge=bridge[a:z]
    assert hashlib.sha256(bridge.encode()).hexdigest()=='1e3a800968e6deb99ddc92052680c56a77c359aba6d7e0471b343fb0b4e0f16b'
    x=(area/'assets/Default.xbe').read_bytes();base=struct.unpack_from('<I',x,0x104)[0]
    count,table=struct.unpack_from('<II',x,0x11c)
    sections=[struct.unpack_from('<6I',x,table-base+i*56) for i in range(count)]
    for va,size,digest in PINS:
        s=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4])
        offset=s[3]+va-s[1]
        assert hashlib.sha256(x[offset:offset+size]).hexdigest()==digest,hex(va)
    originals={};patched={};include='#include "xbe_health_packs.h"\n'
    for unit in sorted(set(ENTRY)|set(SEAMS)):
        path=area/f'generated/game/x86_recomp.{unit}.cpp'
        s=clean(path.read_text(),unit)
        originals[unit]=s
        for name,hook in ENTRY.get(unit,{}).items():
            anchor=f'X86_FUNC(__imp__sub_{name}) {{\n';assert s.count(anchor)==1,name
            s=s.replace(anchor,anchor+hook,1)
        for va,hook in SEAMS.get(unit,()):
            anchor=f'\t// {va:08X} ';assert s.count(anchor)==1,hex(va)
            s=s.replace(anchor,hook+anchor,1)
        if unit==45:
            anchor='\t// 001C5BA3 jbe 0x001C5C1E\n'+OLD
            assert s.count(anchor)==1;s=s.replace(anchor,anchor.replace(OLD,NEW),1)
        patched[unit]=s
        s=s.replace('#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+include,1)
        path.write_text(s)
    for name in ('xbe_health_packs.cpp','xbe_health_packs.h','xbe_health_settings.h'):
        (area/'src/common'/name).write_text((local/name).read_text())
    (area/'src/compat/xbe_health_settings.cpp').write_text((local/'xbe_health_settings.cpp').read_text())
    s=(area/'generated/game/x86_recomp.9.cpp').read_text()
    a=s.index('X86_FUNC(__imp__sub_000643E0) {');z=s.index('\nX86_WEAK_FUNC',a)
    assert hashlib.sha256(s[a:z].encode()).hexdigest()=='65b95e293f8c3716565c1374654c761393adda622e455e39302fc425780aa031'
    pieces=[s[a:z].replace('__imp__sub_000643E0','OriginalProfile',1)]
    s=originals[20];a=s.index('X86_FUNC(__imp__sub_000C77D0)');z=s.index('\nX86_WEAK_FUNC',a)
    assert hashlib.sha256(s[a:z].encode()).hexdigest()=='b825323f5092a034627affb083742c20ebd0978b0114cd9ef6439640ecc2de05'
    pieces.append(s[a:z].replace('__imp__sub_000C77D0','OriginalAimRay',1))
    s=originals[19];a=s.index('X86_FUNC(__imp__sub_000BFE00)');z=s.index('\nX86_WEAK_FUNC',a)
    assert hashlib.sha256(s[a:z].encode()).hexdigest()=='44e64295aecf422b0b4cf423dc7465690bb3533e8e8f61e2b126e87f1dc577e1'
    pieces.append(s[a:z].replace('__imp__sub_000BFE00','OriginalInjectionState',1))
    for variant,sources in (('Original',originals),('Partner',patched)):
        s=sources[21];a=s.index('\t// 000CD980 ');z=s.index('loc_000CDAC0:',a)
        if variant=='Original':assert hashlib.sha256(s[a:z].encode()).hexdigest()=='6387e21703acbe7d5d5ea38def8762a99a78c4ece4adf4e8a334e3cf7db0f35a'
        pieces.append(f'X86_FUNC({variant}InjectionTick) {{\n\tX86_LOAD_FLAGS();\n\tuint32_t ca=0,cb=0;\n'+s[a:z]+'}\n')
    s=originals[21];a=s.index('\t// 000D2F20 ');z=s.index('loc_000D2F4C:',a)
    assert hashlib.sha256(s[a:z].encode()).hexdigest()=='c1592cdb850eb963d9a4f2e695c21868e610e0fe159a7882f05da1d05f27880f'
    pieces.append('X86_FUNC(OriginalInjectionQueue) {\n\tX86_LOAD_FLAGS();\n\tuint32_t ca=0,cb=0;\n'+s[a:z]+'loc_000D2F4C:\n\treturn;\n}\n')
    s=originals[21];a=s.index('\t// 000D2BB9 ');z=s.index('loc_000D2C3B:',a)
    assert hashlib.sha256(s[a:z].encode()).hexdigest()=='fd70604730898b2d082faa7e678dcfb0f3912ef429b2223e508d8637fe65aeac'
    pieces.append('X86_FUNC(OriginalRefillStep) {\n\tX86_LOAD_FLAGS();\n\tuint32_t ca=0,cb=0;\n'+s[a:z]+'loc_000D2C3B:\n\treturn;\n}\n')
    for unit,name,label in ((19,'000BD650','Receipt'),(36,'00169570','Stock'),(38,'00182B60','Delivery'),(6,'0004ACD0','PlayerId')):
        for variant,sources in (('Original',originals),('Stored',patched)):
            s=sources[unit];anchor=f'X86_FUNC(__imp__sub_{name}) {{'
            a=s.index(anchor);z=s.index('\nX86_WEAK_FUNC',a)
            if variant=='Original':assert hashlib.sha256(s[a:z].encode()).hexdigest()==EXTRACTED_HASHES[name],name
            pieces.append(s[a:z].replace('__imp__sub_'+name,variant+label,1))
    for variant,sources in (('Original',originals),('Stored',patched)):
        s=sources[45];a=s.index('\t// 001C5B96 ');z=s.index('\t// 001C5BA5 ',a)
        if variant=='Original':assert hashlib.sha256(s[a:z].encode()).hexdigest()=='c9b4411addd76e9c207abea0715cd17ccd4e827a1170144f591f6f54186e3f1a'
        pieces.append(f'X86_FUNC({variant}Eligibility) {{\n\tX86_LOAD_FLAGS();\n'+s[a:z]+'\tctx.eax=1;return;\nloc_001C5C1E:\n\tctx.eax=0;\n}\n')
        s=sources[6];a=s.index('\t// 0004C330 ');z=s.index('\t// 0004C344 ',a)
        if variant=='Original':assert hashlib.sha256(s[a:z].encode()).hexdigest()=='e524542566692338e16852a198e95cc06c7379263736b2d47bbed5fb98d73a6a'
        pieces.append(f'X86_FUNC({variant}CheckpointTail) {{\n\tX86_LOAD_FLAGS();\n\tuint32_t ca=0,cb=0;\n'+s[a:z]+'}\n')
        s=sources[20];a=s.index('\t// 000CCA4C ');z=s.index('\t// 000CCA6C ',a)
        if variant=='Original':assert hashlib.sha256(s[a:z].encode()).hexdigest()=='6e8631bfc45cd59a07b532f00657262a70d41b7c70d968b1df05c257fc481f8f'
        pieces.append(f'X86_FUNC({variant}CheckpointRestore) {{\n\tX86_LOAD_FLAGS();\n\tuint32_t ca=0,cb=0;\n'+s[a:z]+'\treturn;\nloc_000CCBBC:\n\treturn;\n}\n')
    s=originals[6];a=s.index('X86_FUNC(__imp__sub_0004AF40) {');z=s.index('\nX86_WEAK_FUNC',a)
    assert hashlib.sha256(s[a:z].encode()).hexdigest()=='6cc239fda29236f7e04372b36ffa90f845448c05160f43de0932ebe34697da97'
    pieces.append(s[a:z].replace('__imp__sub_0004AF40','OriginalHudAllowed',1))
    for unit,name,label,digest in (
        (54,'002180A0','OriginalSetHealth','934c1d1bd89f0665eda05195aa1bf8499cb202e98ee7e71798924aca9d600037'),
        (54,'00218120','sub_00218120','98986e1f27a70a0afca0b839da9e97e17a45ba34c7c6fbd96a7f1f6ddf5d64b0'),
        (10,'0006BAC0','sub_0006BAC0','e50c500a76dd1ddd951293be1172b4dd5f2689293406540dfcecf12613a06d5a'),
        (55,'00222400','sub_00222400','09c13a1e23d706cb811e6af7bbb18ca18f5fb01c54ae195a3002f493ac6f732e'),
        (55,'002223D0','OriginalFadeAlpha','a9a5ea237d54c9038ae50977cd597103ae28484a7f8e316db297c603938b795d')):
        s=(area/f'generated/game/x86_recomp.{unit}.cpp').read_text()
        a=s.index(f'X86_FUNC(__imp__sub_{name}) {{');z=s.index('\nX86_WEAK_FUNC',a)
        assert hashlib.sha256(s[a:z].encode()).hexdigest()==digest,name
        pieces.append(s[a:z].replace('__imp__sub_'+name,label,1))
    s=(area/'generated/game/x86_recomp.11.cpp').read_text()
    a=s.index('\t// 00073E37 ');z=s.index('\t// 00073E5B ',a)
    assert hashlib.sha256(s[a:z].encode()).hexdigest()=='14e3d65d24ac77f7a2305ebc8f5482632af262e4b0474b63b0927372fbe222e7'
    pieces.append('X86_FUNC(OriginalHudFade) {\n'+s[a:z]+'}\n')
    # Original maximum-health query includes its settings-table stack frame.
    s=(area/'generated/game/x86_recomp.8.cpp').read_text()
    a=s.index('X86_FUNC(__imp__sub_0005AAB0) {');z=s.index('\nX86_WEAK_FUNC',a)
    assert hashlib.sha256(s[a:z].encode()).hexdigest()=='a2545e4f4dd2e46a08e6eaaa35cb92ab4e0a90a6796cbcc2c68833a59af6c04d'
    pieces.append(s[a:z].replace('__imp__sub_0005AAB0','sub_0005AAB0',1))
    s=(area/'generated/game/x86_recomp.8.cpp').read_text()
    a=s.index('X86_FUNC(__imp__sub_0005B140) {');z=s.index('\nX86_WEAK_FUNC',a)
    assert hashlib.sha256(s[a:z].encode()).hexdigest()=='fca5208e06a850825d64fd0296f8c8c3f6f38bd6f86c8fb81d58ada92016bd0a'
    pieces.append(s[a:z].replace('__imp__sub_0005B140','sub_0005B140',1))
    s=(area/'generated/game/x86_recomp.9.cpp').read_text()
    a=s.index('X86_FUNC(__imp__sub_000643E0) {');z=s.index('\nX86_WEAK_FUNC',a)
    assert hashlib.sha256(s[a:z].encode()).hexdigest()=='65b95e293f8c3716565c1374654c761393adda622e455e39302fc425780aa031'
    pieces.append(s[a:z].replace('__imp__sub_000643E0','sub_000643E0',1))
    s=(area/'generated/game/x86_recomp.9.cpp').read_text()
    a=s.index('X86_FUNC(__imp__sub_00064400) {');z=s.index('\nX86_WEAK_FUNC',a)
    assert hashlib.sha256(s[a:z].encode()).hexdigest()=='27a1fd87731e8f497b65731890008267ce6c3440f375a2b967fe020734fd10ff'
    pieces.append(s[a:z].replace('__imp__sub_00064400','sub_00064400',1))
    s=(area/'generated/game/x86_recomp.19.cpp').read_text()
    a=s.index('X86_FUNC(__imp__sub_000BDAB0) {');z=s.index('\nX86_WEAK_FUNC',a)
    assert hashlib.sha256(s[a:z].encode()).hexdigest()=='9c508f4d4592e6233f97c5051efa1298678d01fd0b9a51f6bba706501803fa7f'
    pieces.append(s[a:z].replace('__imp__sub_000BDAB0','OriginalMaximum',1))
    test=(local/'test_xbe_health_packs.cpp').read_text();marker='// ORIGINAL_HEALTH_PACK_CONTRACTS'
    assert test.count(marker)==1
    # A cancelled injection retries from native neutral state 8. BFE00 uses
    # this table when leaving states 4..11, including that normal retry path.
    state_table=[]
    for va in range(0xbfeac,0xbfecc,4):
        section=next(s for s in sections if s[1]<=va and va+4<=s[1]+s[4])
        value=struct.unpack_from('<I',x,section[3]+va-section[1])[0]
        state_table.append(value)
    assert state_table==[0xbfe1d,0xbfe39,0xbfe55,0xbfe94,0xbfe94,0xbfe71,0xbfe94,0xbfe8d]
    assert test.count('// ORIGINAL_INJECTION_STATE_TABLE')==1
    test=test.replace('// ORIGINAL_INJECTION_STATE_TABLE','\n'.join(
        f'Put(0x{0xbfeac+i*4:x},0x{value:x});' for i,value in enumerate(state_table)))
    constants=(*range(0x646bc,0x646f4,4),0x46c150,0x450b94,0x457cec,0x452840,0x44c1e0,0x44c174,0x44c6c4,0x46fc4c)
    values=[]
    for va in constants:
        section=next(s for s in sections if s[1]<=va and va+4<=s[1]+s[4])
        value=struct.unpack_from('<I',x,section[3]+va-section[1])[0]
        values.append(f'Put(0x{va:x},0x{value:x});')
    assert test.count('// ORIGINAL_MAXIMUM_CONSTANTS')==2
    test=test.replace('// ORIGINAL_MAXIMUM_CONSTANTS','\n'.join(values))
    bridge_marker='// ORIGINAL_CALL_GUEST_CONTRACT';assert test.count(bridge_marker)==1
    (area/'src/compat/test_health_packs.cpp').write_text(test.replace(marker,'\n'.join(pieces)).replace(bridge_marker,bridge))

"""Stored medic supplies using the original handoff, stock and player healing."""
from pathlib import Path
import hashlib
import struct

PINS=(
    (0x70d90,455,'879111defca52dd723604dadc820a063cf5705351be989ed3f38d58867be6671'),
    (0x76a50,736,'5d703977efb27bb1283f614464c7a46dcf7269c5af635b34aa5cc119268d030e'),
    (0x4af40,0x58,"a882f5e7b60a51f78d55c9a30af24bb6c5e86204086bb031fdf67460c382af6a"),
    (0x73de6,0x7c,"c2cdc041da11aacca6fdc3f954e778412faf1e4343abb65a322c35cfe326f73a"),
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
    for hook in ENTRY.get(unit,{}).values():source=source.replace(hook,'')
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
    for name in ('xbe_health_packs.cpp','xbe_health_packs.h'):
        (area/'src/common'/name).write_text((local/name).read_text())
    pieces=[]
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
    test=(local/'test_xbe_health_packs.cpp').read_text();marker='// ORIGINAL_HEALTH_PACK_CONTRACTS'
    assert test.count(marker)==1
    bridge_marker='// ORIGINAL_CALL_GUEST_CONTRACT';assert test.count(bridge_marker)==1
    (area/'src/compat/test_health_packs.cpp').write_text(test.replace(marker,'\n'.join(pieces)).replace(bridge_marker,bridge))

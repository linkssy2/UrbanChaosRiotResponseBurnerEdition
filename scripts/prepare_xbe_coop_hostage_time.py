"""Share authored hostage slow-motion admission and view feedback in local co-op."""
from pathlib import Path
import hashlib,re,struct

INCLUDE='#include "xbe_coop_hostage_time.h"\n'
HOOKS=(
    (0x160318,'\tTrialCoopHostageSlowmoAdmission(ctx,base);\n'),
    (0x1600ff,'\tTrialCoopHostageSlowmoQueryPlayer(ctx,base);\n'),
    (0x160123,'\tTrialCoopHostageSlowmoQueryPlayer(ctx,base);\n'),
    (0x160345,'\tTrialCoopHostageSlowmoLivingPlayer(ctx,base);\n'),
    (0x160b9c,'\tTrialCoopHostageSlowmoLivingPlayer(ctx,base);\n'),
    (0x160bfb,'\tTrialCoopHostageSlowmoFeedback(ctx,base);\n'),
)
BODIES=(
 (28,0x11bdc0,84,'5aacd065702e1bf73dbfd7d36dacde5cdf9f0bb256c79f54a19d810f1c037676','39691df163164cfdc3ae535e2d31271033574d9abe2959b005bc17f291d7faae'),
 (35,0x1600d0,102,'d1ff63d3c5992aea79be3fcf9367c3f343674bb24386840e5bda37503510c10c','bad56af163c7a87ea415b333c83a8304566819cd6c3d4f87ad0979f128a6c32b'),
 (35,0x160310,175,'351e3d8aa87cf2cc557557ee1cb657c963b09bc09c14b981a17057bd4a1b3565','108a9c29c13726e84db929379a1e1fd049b3e6d7701e76c8136d1eede7140b24'),
 (35,0x160810,47,'6d6033bec57782508f518d93a9fc87910fd55d6a3343e261a5bd2c06af32f924','d111681f24caf7cbb5b4a8a48f57e51b730e65d5954fee7f0fac2cd1ec736f7d'),
 (35,0x160840,32,'3e39596ef376ef6d50145e029049b819b9dcab9f3dfb29cb702583058f708d77','dc84acd3dc9b0a6e8c9cc9fb9384f581ca2b93f964e34a613b26bb7c4f186654'),
 (35,0x160860,8,'541cc225c41dd4213c245f208ebd20c6a2b5374fd8f1df5c9c442b05f9e058ca','bead579e1323ad73c2bfdc26b68777e03d184b93d4620951414c793d48c0bc53'),
 (35,0x1603c0,117,'36f6833c123722e9411813c6fe8d97d05be5fb6f8a8b1538d25fccde6ffae100','7d641f6b9ec1ea3fc61d8fc1a3e172d112fedb1b6a6e1dc70c0abda3ed78a0b1'),
 (35,0x160440,132,'efde68037c2f20c1a17bd0a7213e10fe70445ef632a29d1d7ff11c577484052e','ffc522bb94f7c63615e0dcb2ece94b1870915a1b3efca74b65ce136ac8ff1b02'),
 (40,0x191890,106,'08b1f8eb8addd84f2c0ef7b3065b105c2b825c676b0b47b77823a566e1d0eb29','aeea2a26d66b96387da8805a85dfea7598b37b26dea8b83824872c1410a3393c'),
 (50,0x1f9010,9,'899c718a5333cfa40cdaa23a42713aa616de87fbb074f1150bb0bfaa7873d339','8f5786d86fd1a77504011d1037b9c29cd58d632d252b0c6fdc767eab134f8abb'),
 (3,0x338b0,20,'72965d30936f3b482f4a8c9b2b0f8f3846052d4283907c0669207cc19dea4b5f','d5184d97e14432bef4b5fe7b71c4e101a9b20cdaf244af295343b3bcc012c694'),
 (19,0xbd640,15,'486b3fc18d189b9e7cc3246264eb458bd1a35e56d54232698cef42fb2c30fed0','f26f475955e5b2d1716b2c198939e099d1ffe95b57b57253a8d1bc97ec70fe92'),
 (6,0x4a840,6,'e51598aae247076e5665f4a60dfcf3e4ed9a470afbcf020165528094ba6f6c58','77e02a84134be3bac2bfcea70562957f6be493f348198ff2ef8a30e72d891f4b'),
 (6,0x4acd0,9,'3d88229ba8f81117aa293b673da37176d12ed5fdadb9eeb56bfda6da97e01c38','5e658c29e77f331c045498dba19928e7629042aec91a0cc55482940bbc4e1a05'),
)
SLICES=(
 ('DeathRequest',0x11a005,0x11a053,'7cff0a23d9bf7e0df2caa616ddb9158a742e1418f92bef236002bc72bfbd883e','1b6673d9c0d5db654900dc2e445d0bb0f6a80eda93745b64fb8c177b91e1c489'),
 ('Ramp',0x160a69,0x160b05,'0bea0c0a591a9acee117863b9c1b5cc00ed39847891866eff50144441b707d13','c23b31ee9796ce69699bfc8d885e80f3ef86864f0fda80c0fb112168683a2239'),
 ('Feedback',0x160b78,0x160c01,'a5b89cd55da44e0424ca7fc088ae5bdbec0378dc580e812830038a159aee2c12','3418bc1875290531487aab8a759e5fb321589c368c69062b0708521e1968fdab'),
 ('Teardown',0x16076d,0x160779,'fe2cd91b95f53656bd8ba692333c3f95529eb4f9be996557dab8db99dc000d51','b11b58a71961f71b5168ca3f702235b9048bf2de9ff832825150627f599507ce'),
)
def clean(source):
    source=source.replace(INCLUDE,'')
    for _,hook in HOOKS:source=source.replace(hook,'')
    return source
def unprepare(area):
    path=Path(area)/'generated/game/x86_recomp.35.cpp'
    if path.exists():
        before=path.read_text();after=clean(before)
        if after!=before:path.write_text(after)
def patched(body):
    for va,hook in HOOKS:body=body.replace(f'\t// {va:08X} ',hook+f'\t// {va:08X} ')
    return body
def clock_fixture(area,raw,test):
    def clock_body(unit,address):
        source=(area/f'generated/game/x86_recomp.{unit}.cpp').read_text()
        return re.search(rf'X86_FUNC\(__imp__sub_{address:08X}\) \{{.*?^\}}',source,re.M|re.S)[0]
    for va,size,digest in (
        (0x5d3bc,24,'795cde1416ec1fbf287cc22479c6dd2022e02e0ead056c276aeee5c697284e74'),
        (0x208f80,911,'5d74f89e9de9bc3bb3d935054ee02599f9282fb6309313642d4b57df38875fc7'),
        (0x1f9010,41,'8e6c0f9ad90635fce0407c7a5c2f1db05be16d47dcb776d2d315fa62e21cc202'),
    ):assert hashlib.sha256(raw(va,size)).hexdigest()==digest,hex(va)
    world=clock_body(52,0x208f80)
    for hook in ('\tTrialCoopBodiesRetire(ctx,base);\n','\tTrialCoopDeathUpdate(ctx,base);\n'):
        world=world.replace(hook,'')
    assert hashlib.sha256(world.encode()).hexdigest()=='4de70826c50b1a8ca9541bd2d64751006abf9da19db358b220972f956a56ba10'
    names=sorted(set(re.findall(r'\bsub_([0-9A-F]{8})(?=\(ctx)',world)))
    world=world.replace('__imp__sub_00208F80','ClockOriginalWorld',1).replace('x86_icall(','ClockIcall(')
    for name in names:world=world.replace('sub_'+name+'(','ClockStub_'+name+'(')
    bodies=[world]
    getters=(
        (0x1f9010,'8f5786d86fd1a77504011d1037b9c29cd58d632d252b0c6fdc767eab134f8abb'),
        (0x1f9020,'864bb64bb47f8329ce36c4a209123dd0adca9b9c72beded827d6a192ead6df32'),
    )
    for va,digest in getters:
        original=clock_body(50,va);assert hashlib.sha256(original.encode()).hexdigest()==digest
        bodies.append(original.replace(f'__imp__sub_{va:08X}',f'ClockGetter_{va:08X}',1))
    source=(area/'generated/game/x86_recomp.8.cpp').read_text()
    first=source.index('\t// 0005D3BC ');last=source.index('\t// 0005D3D4 ',first)
    scale=source[first:last]
    assert hashlib.sha256(scale.encode()).hexdigest()=='34edec60794ae974911cc3461c0c66af0ade9f988ac3b5d752f2322b9c819310'
    bodies.append('X86_FUNC(ClockScale){\n'+scale+'}\n')
    declarations=['X86_FUNC(ClockStub_'+name+');'for name in names]
    declarations.extend(f'X86_FUNC(ClockGetter_{va:08X});'for va,_ in getters)
    return test.replace('// CLOCK_DECLARATIONS','\n'.join(declarations)).replace('// CLOCK_NATIVE_BODIES','\n'.join(bodies))

def prepare(area):
    area=Path(area);local=Path(__file__).parent
    image=(area/'assets/Default.xbe').read_bytes();base=struct.unpack_from('<I',image,0x104)[0]
    count,table=struct.unpack_from('<II',image,0x11c)
    sections=[struct.unpack_from('<6I',image,table-base+i*56) for i in range(count)]
    def raw(va,size):
        section=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4]);offset=section[3]+va-section[1]
        return image[offset:offset+size]
    sources={unit:clean((area/f'generated/game/x86_recomp.{unit}.cpp').read_text()) for unit in {row[0] for row in BODIES}}
    sources[6]=sources[6].replace('\tTrialCoopHazardPlayer(ctx,base);\n','').replace('\tTrialHealthPackMedicPlayer(ctx,base,false);\n','')
    fixtures=[]
    for unit,va,size,raw_hash,source_hash in BODIES:
        assert hashlib.sha256(raw(va,size)).hexdigest()==raw_hash,hex(va)
        source=sources[unit];start=source.index(f'X86_FUNC(__imp__sub_{va:08X})');end=source.index('\nX86_WEAK_FUNC',start)
        body=source[start:end];assert hashlib.sha256(body.encode()).hexdigest()==source_hash,hex(va)
        if va in (0x1600d0,0x160310,0x160810):
            names={0x1600d0:'Query',0x160310:'Request',0x160810:'Event'}
            for fixed in (False,True):
                result=patched(body) if fixed else body
                result=result.replace(f'__imp__sub_{va:08X}',f'{"Coop" if fixed else "Original"}{names[va]}',1)
                result=result.replace('sub_001600D0(ctx','CoopQuery(ctx' if fixed else 'OriginalQuery(ctx')
                result=result.replace('sub_00160310(ctx','CoopRequest(ctx' if fixed else 'OriginalRequest(ctx')
                fixtures.append(result)
        else:fixtures.append(body.replace(f'__imp__sub_{va:08X}',f'sub_{va:08X}',1))
    for name,first,last,raw_hash,source_hash in SLICES:
        source=sources[28 if name=='DeathRequest' else 35];start=source.index(f'\t// {first:08X} ');end=source.index(f'\t// {last:08X} ',start)
        body=source[start:end];assert hashlib.sha256(raw(first,last-first)).hexdigest()==raw_hash,hex(first)
        assert hashlib.sha256(body.encode()).hexdigest()==source_hash,hex(first)
        for fixed in (False,True) if name=='Feedback' else (False,):
            result=patched(body) if fixed else body
            external=set(re.findall(r'goto (loc_[0-9A-F]+)',result))-set(re.findall(r'^(loc_[0-9A-F]+):',result,re.M))
            result+='return;\n'+''.join(f'{label}:\n' for label in sorted(external))+'return;\n'
            label=('Coop' if fixed else 'Original')+name
            fixtures.append(f'X86_FUNC({label}) {{\nX86_LOAD_FLAGS();uint32_t ca=0,cb=0;\n'+result+'}\n')
    test=(local/'test_xbe_coop_hostage_time.cpp').read_text()
    declarations=sorted(set(re.findall(r'\b(?:sub_[0-9A-F]{8}|(?:Coop|Original)(?:Query|Request|Event))(?=\(ctx)','\n'.join(fixtures))))
    test=test.replace('// ORIGINAL_HOSTAGE_TIME_DECLARATIONS','\n'.join('X86_FUNC('+name+');' for name in declarations))
    test=test.replace('// ORIGINAL_HOSTAGE_TIME_BODIES','\n'.join(fixtures))
    constants=(0x44bf94,0x44e150,0x44c174,0x44f674,0x44e1d8,0x4f9f6c,0x4f9e4c,0x466738,0x44c1f8,0x44c538)
    test=test.replace('// ORIGINAL_HOSTAGE_TIME_CONSTANTS','\n'.join(f'Put(0x{va:x},0x{struct.unpack("<I",raw(va,4))[0]:x});' for va in constants))
    test=clock_fixture(area,raw,test)
    source=sources[35]
    for va,_ in HOOKS:assert source.count(f'\t// {va:08X} ')==1,hex(va)
    changed=patched(source);assert clean(changed)==source
    changed=changed.replace('#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+INCLUDE,1)
    (area/'generated/game/x86_recomp.35.cpp').write_text(changed)
    for name in ('xbe_coop_hostage_time.h','xbe_coop_hostage_time.cpp'):(area/'src/common'/name).write_text((local/name).read_text())
    (area/'src/compat/test_coop_hostage_time.cpp').write_text(test)

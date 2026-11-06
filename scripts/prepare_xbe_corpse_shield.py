"""Scoped equipment aliases driven by an owned native chest-pinned ragdoll."""
from pathlib import Path
import hashlib
import json
import re
import struct
import prepare_xbe_coop_turret_assets as turret

INCLUDE='#include "xbe_corpse_shield.h"\n'
HOOKS={
 7: (('\t// 0005505B ', '\tTrialCorpseShieldReset();\n'),),
 14:(('\t// 0008DB16 ', '\tTrialCorpseShieldResourceLoaded(ctx,base);\n'),),
 19:(('X86_FUNC(__imp__sub_000B9800) {\n', '\tTrialCorpseShieldPose(base,ctx.ecx);\n'),
     ('\t// 000B9853 ', '\tTrialCorpseShieldPose(base,ctx.ebp-8);\n'),
     ('\t// 000B98CC ', '\tTrialCorpseShieldPose(base,ctx.edi);\n')),
 53:(('X86_FUNC(__imp__sub_0020BC40) {\n','\tTrialCorpseShieldScope corpse_shield(ctx,base);\n'),
     ('\t// 0020BC7C ', '\tcorpse_shield.Select(ctx);\n')),
 71:(('\t// 002A8906 ', '\tif(TrialCorpseShieldIgnoreQuery(base,ctx.esi,X86_MEM32(ctx.ebx+0x24u))) goto loc_002A8913;\n'),),
}
# Retire the rejected flashlight hooks from already prepared trees.
LEGACY=('\tTrialCorpseShieldScope corpse_shield(ctx,base,true);\n',
        '\tTrialCorpseShieldScope corpse_shield(ctx,base,false);\n')
PINS=(
(52,0x20a1d0,36,'624973c7a23e9b1f462b73835f48e5ce428a08085e0ec6d352ddf0ce922fd8ab','73f885c6fc231bac7f5442eb0ead4187352c43f6dff1d747146aadcb9d3c54e0'),
(53,0x210910,121,'8eeee3416fadfb85ed9107acb18d81da98bcf15d04b777aad1eac1b9790ec1b5','dc73161715493a3a7054046cd80e74bb161fc87cb64206b67c699d0bf358bf65'),
(71,0x2a8860,192,'18c40bb20dcfc1d19c2f202000a77a369c38181affb6eadef3436aa706356612','003d731dc2b7635c68858a528ddf0f801559fc828870823945d6dcbc0cd89dfb'),
(55,0x21d0e0,167,'fbea20ad1cbd5df273d831c1c30542a008d9014588426a847a23193e85475b33','c617eeb551027c6d7fb91609d169e8d45f4c492bd13bdd9f2d6448f2a6e47b4b'),
(55,0x21b1a0,115,'f1c62d257247edc8aaa36df4d10e25854690887d068f6d206ea1986c04b4145f','4be28d44a96d162e3e343da123be478de65a56fec18f092b81ea931d2170f079'),
(18,0xb5570,47,'e393bde54e3b8abbeb4ddf2f008fb1704d6a2d0692d9dff63655f908c7ebd163','38bdd08920ba41a22c92742210a53179ce22aeeb4bf86aca263394a49d5ca592'),
(18,0xb55d0,81,'625588bd4e7b6b38818be2124cb1e407ad002d9c1f7d4f829361b8db67a0a1bf','bb56f0cde73c7b9f2e38dd1ec57211d5e9f67595915f897282c38202b5e0779f'),
(18,0xb5c90,38,'166f3352f6077c8ad16827e4be4adc43cb85f9bc46fb955d69304a840471b8eb','dc1e8116b78056304c4805b164c3d4109f8581b82c7f5452d364dea7757002f6'),
(19,0xbe050,162,'4423efbcb68f4d7c2448b42120d993cad199482c4ec8ad65eb2bab8eae35eea1','f3ebcca779e02277adc0380cb440fee4fbe4f3cbc24d3c53d9121c8088470a83'),
(25,0xfe170,200,'e9c9e4c208050898896367637ff5dca289f7bd67ef4334d94e490e0dfce0d650','329933b2f19bd81b5c8c6ca89318192b4b6db4b4b2fc4fb18b69cad0721d7d92'),
(19,0xbd940,355,'4ed43c4acba901d9f4ddb10e8ddacb0db28167d9b3571dab6523264de0ab2d58','d975b9d545612c2360b3e2515af89fcbb7d8305a80568ec5b42a9db538bc3aa1'),
(19,0xb9800,33,'155500ed85f7109056f75305c46ecde14fae2a4ef6e09873ad52b80aefb28119','00266d319c18a2489c67fb2bc55ce5a904e66bf9f551415efdb0ab62b84373b3'),
(19,0xb9830,228,'8d040b17298fe1e8e3b30e81af783eb92d136207cc2cc9910712d01333e8d4f5','ba1ff988b46291593d897abf2fc0ac180ae14294c0c70ee93207b9a4de0e024d'),
(25,0xfcff0,63,'92984b678bcc165c65f7e32738790e5655413ce18cb48340d9e73e0738c68469','c1750a290927a5b033191a7603c9b7528ff504d4b1d4779dc275f597ec2af1ad'),
(7,0x54fe0,1579,'175eab758296750d132f539a54479bc60de104f002d79e4594fd39e0e07ea7af','cdc6b231c84464c0f29cf80c442d0c48586cc1a47dc167f9a7a1106b627ceffe'),
)
SHARED=('\tTrialCoopResourceLoaded(ctx,base);\n','\tTrialCoopPreload(ctx,base);\n')

def function(source,va):
    return re.search(rf'X86_FUNC\(__imp__sub_{va:08X}\) \{{.*?^\}}',source,re.M|re.S)[0]

def clean(source):
    source=source.replace(INCLUDE,'')
    for hook in LEGACY:source=source.replace(hook,'')
    for hooks in HOOKS.values():
        for _,hook in hooks:source=source.replace(hook,'')
    return source

def patch(source,unit):
    source=clean(source);result=source
    for anchor,hook in HOOKS[unit]:
        assert result.count(anchor)==1,anchor
        result=result.replace(anchor,anchor+hook if anchor.startswith('X86_FUNC') else hook+anchor,1)
    result=INCLUDE+result
    assert clean(result)==source
    return result

def unprepare(area):
    for unit in HOOKS:
        path=Path(area)/f'generated/game/x86_recomp.{unit}.cpp'
        if path.exists():
            before=path.read_text();after=clean(before)
            if before!=after:path.write_text(after)

def authored_pickups(area):
    # Source indices are level-local. Census every shipped ENT so admitting
    # this class/index/model combination cannot replace an unrelated prop.
    import sys
    sys.path.insert(0,str(Path(__file__).parent/'native_level_checks'))
    from inspect_level import load
    levels=sorted((area/'assets').glob('*.ent'));assert len(levels)==27
    found=[]
    for path in levels:
        reader=load(path);reader.level()
        for index in (578,585):
            if index>=len(reader.entities):continue
            prop=reader.entities[index]
            if prop['kind']=='BasicProp' and prop['asset'].lower()=='assets_playerweaponsandequipment\\eqp_shield\\eqp_shield.amf':
                found.append((path.name,index,hashlib.sha256(reader.data[prop['offset']:prop['end']]).hexdigest()))
    assert found==[
        ('12-01.ent',578,'e6c5d774ae9281c5c8a38ef7a019be33b5afca6f4fe93b5027ba7e4e1bd7b4f0'),
        ('12-01.ent',585,'a11faecd4881065b926d283963225c18d178fc29edfc0c14497ad211e620be73')],found

def prepare(area):
    area=Path(area);local=Path(__file__).parent
    authored_pickups(area)
    image=(area/'assets/Default.xbe').read_bytes();base=struct.unpack_from('<I',image,0x104)[0]
    count,table=struct.unpack_from('<II',image,0x11c)
    sections=[struct.unpack_from('<6I',image,table-base+i*56)for i in range(count)]
    def raw(va,size):
        section=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4])
        offset=section[3]+va-section[1];return image[offset:offset+size]
    sources={u:clean((area/f'generated/game/x86_recomp.{u}.cpp').read_text())for u in set(HOOKS)|{u for u,_,_,_,_ in PINS}|{u for u,_,_ in turret.SOURCES}}
    for unit,va,size,digest,source_digest in PINS:
        assert hashlib.sha256(raw(va,size)).hexdigest()==digest,hex(va)
        body=function(sources[unit],va)
        for hook in SHARED:body=body.replace(hook,'')
        assert hashlib.sha256(body.encode()).hexdigest()==source_digest,hex(va)
    assert struct.unpack('<I',raw(0x457830+8,4))[0]==0xb5570
    assert struct.unpack('<I',raw(0x457830+12,4))[0]==0xb55d0
    assert struct.unpack('<I',raw(0x462658+0x28,4))[0]==0xfe170
    assert struct.unpack('<I',raw(0x46fbe8+0x28,4))[0]==0xbe050
    assert struct.unpack('<I',raw(0x47e278+0x28,4))[0]==0x20bc40
    assert raw(0x1e27c5,6).hex()=='c70678e24700'
    assert raw(0x20be7d,8).hex()=='e8cec2ffff894654'
    assert raw(0x20bea2,12).hex()=='8b0de0c351008b5114895650'
    # BasicProp uses 20A1D0's descriptor model; live provider ID assignment
    # happens afterwards. The native model wrapper loads both companions from
    # that same name; the pickup asset therefore retains the original skeleton,
    # mdlhk and entsupp. Neither pickup trigger nor collision setup is patched.
    assert raw(0x21094e,49).hex()=='8b4e0c518bcfe87798fffff6460820750c68000000108bcfe8d597ffff8b46088b575483e0e883c8088956048946088b76'
    assert raw(0x21d14e,13).hex()=='e83dfbffff578bcee815feffff'
    # Entity virtual+34 (MainPlayer D0080/CE670) updates equipment world before
    # 20929C advances physics. Read owner+6F0, not its later C0180 render copy.
    assert raw(0xd09bd,5).hex()=='e8aedcffff'
    assert raw(0x209270,49).hex()=='85f674288b470485c08b0f740c8b49048b118bf8ff5234ebe78b760485f674028b3e8b49048b11ff5234ebd4e82fb30000'
    # Bash remains the retail MeleeHit -> camera cast -> actor damage route.
    # Only an owned body's accepted ray candidate is excluded, before the
    # nearest-hit collector can let its actor-zero result absorb the strike.
    assert raw(0xd0fae,7).hex()=='8bcee80b48ffff'
    assert raw(0xc580f,5).hex()=='e81cea1400'
    assert raw(0xc5d4c,8).hex()=='8b07528bcfff506c'
    assert raw(0x2a886c,5).hex()=='8b7e1003fe'  # collidable-relative body pointer
    assert raw(0x2f5cb8,3).hex()=='8d5614'  # native rigid-body collidable at +14
    assert raw(0x2f5d00,4).hex()=='8bc62bc2'
    assert raw(0x2f5d0c,3).hex()=='894210'  # stores body-collidable at collidable+10
    # Pin the real item-registry/caller operands, not a fixture-invented name.
    assert raw(0x551b5,10).hex()=='c785a0000000500d4500'
    assert raw(0xc8a4e,41).hex()=='8bcde82b95f7ff3bc3750433c0eb0c8b400450e86ab8f7ff83c4048b165305a0000000508bceff5228'
    assert raw(0xcc6b7,13).hex()=='c744241c98894500e87c12ffff'
    names=[raw(va,100).split(b'\0')[0].decode('ascii') for va in (0x450d50,0x450ce8,0x450da0,0x458998)]
    assert all('EQP_Shield' in name for name in names[:3]) and 'EQP_Flashlight' in names[3]
    for va,size,digest in turret.PINS:
        if va in (0x199810,0x4753a0):continue
        assert hashlib.sha256(raw(va,size)).hexdigest()==digest,hex(va)
    for unit,va,digest in turret.SOURCES:
        if va==0x199810:continue
        body=function(turret.strip(sources[unit]),va)
        for hook in SHARED:body=body.replace(hook,'')
        assert hashlib.sha256(body.encode()).hexdigest()==digest,hex(va)
    # Contract fixtures exercise these exact native loader bodies. Other model
    # observers remain installed in production and are isolated at test boundary.
    witnesses=['const char* const native_model_names[]={'+','.join(json.dumps(name) for name in names)+'};']
    for unit,va,name in ((53,0x20bc40,'sub_0020BC40'),(52,0x20a1d0,'NativePickup'),(19,0xbe050,'NativeShield'),(19,0xbd940,'NativeFlashlight'),(19,0xb9830,'NativePose'),(25,0xfe170,'NativeThirdShield')):
        source=turret.strip(sources[unit])
        if unit in HOOKS:source=patch(source,unit)
        witnesses.append(function(source,va).replace(f'__imp__sub_{va:08X}',name))
    for unit in HOOKS:(area/f'generated/game/x86_recomp.{unit}.cpp').write_text(patch(sources[unit],unit))
    for name in ('xbe_corpse_shield.h','xbe_corpse_shield.cpp'):(area/'src/common'/name).write_text((local/name).read_text())
    test=(local/'test_xbe_corpse_shield.cpp').read_text();marker='// NATIVE_CORPSE_SHIELD_LOADERS'
    (area/'src/compat/test_xbe_corpse_shield_motion.h').write_text((local/'test_xbe_corpse_shield_motion.h').read_text())
    assert test.count(marker)==1
    (area/'src/compat/test_corpse_shield.cpp').write_text(test.replace(marker,'\n'.join(witnesses)))
    query=(local/'test_xbe_corpse_shield_query.cpp').read_text();marker='// NATIVE_CORPSE_SHIELD_QUERY'
    assert query.count(marker)==1
    original_filter=function(sources[71],0x2a8860).replace('__imp__sub_002A8860','NativeShieldQuery')
    filtered=function(patch(sources[71],71),0x2a8860).replace('__imp__sub_002A8860','HeldShieldQuery')
    (area/'src/compat/test_corpse_shield_query.cpp').write_text(query.replace(marker,original_filter+'\n'+filtered))

if __name__=='__main__':prepare(Path(__file__).resolve().parents[1]/'build/xbe-full-integration-20261014')

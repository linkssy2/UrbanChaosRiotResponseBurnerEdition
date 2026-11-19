"""Actor-local campaign conversations, preserving original authored admission."""
from pathlib import Path
import hashlib
import re
import struct

def runtime_call_guest():
    source=(Path(__file__).resolve().parents[1]/'build/upstream-review-xboxxbe-20261014/runtime/cpu/dispatch.cpp').read_text()
    start=source.index('uint32_t CallGuest(')
    body=source[start:source.index('\n}\n',start)+2]
    assert hashlib.sha256(body.encode()).hexdigest()=='1e3a800968e6deb99ddc92052680c56a77c359aba6d7e0471b343fb0b4e0f16b'
    return body

INCLUDE='#include "xbe_coop_interactions.h"\n'
ENTRY='\tTrialCoopInteractionScope coop_interaction(ctx,base);\n\tif(!coop_interaction.Update()){ctx.esp+=8;return;}\n'
WEAPON_ENTRY='\tTrialCoopInteractionWeaponScope coop_weapons(ctx,base);\n'
MANUAL_ENTRY='\tif(TrialCoopInteractionWeaponInputBlocked(ctx.ecx)){ctx.esp+=4;return;}\n'
HOOKS={
    3:[('00033B7E','\tTrialCoopInteractionLookup(ctx,base);\n'),
       ('000328F0','\tTrialCoopInteractionClose(ctx,base,ctx.ecx);\n')],
    21:[],
    38:[('0017BD96','\tTrialCoopInteractionLookup(ctx,base);\n'),
        ('0017BEEC','\tTrialCoopInteractionLookup(ctx,base);\n'),
        ('0017C1AB','\tTrialCoopInteractionLookup(ctx,base);\n'),
        ('0017C35D','\tTrialCoopInteractionLookup(ctx,base);\n'),
        ('0017BC99','\tTrialCoopInteractionLookup(ctx,base);\n'),
        ('0017BEA9','\tTrialCoopInteractionInput(ctx,base);\n'),
        ('0017BB70','\tTrialCoopInteractionInput(ctx,base);\n'),
        ('0017BE68','\tif(!coop_interaction.Begin())goto loc_0017C3DA;\n'),
        ('0017BFD0','\tif(!coop_interaction.Commit())goto loc_0017C3DA;\n'),
        ('0017C12D','\tif(!coop_interaction.Commit())goto loc_0017C3DA;\n'),
        ('0017C20F','\tif(!coop_interaction.Commit())goto loc_0017C3DA;\n'),
        ('0017C3D2','\tif(!coop_interaction.Commit())goto loc_0017C3DA;\n'),
        ('0017BAC0','\tTrialCoopInteractionClose(ctx,base,ctx.ecx);\n'),
        ('0017BC10','\tTrialCoopInteractionClose(ctx,base,ctx.ecx);\n'),
        ('0017C430','\tTrialCoopInteractionClose(ctx,base,ctx.ecx);\n')]
}
FADES=(('0017BF45','0017BF48',28,False),('0017C203','0017C206',28,False),
       ('0017C3B5','0017C3B8',28,False),('0017C3FC','0017C3FF',4,True))
PINS=(
 (0x17bd80,1681,'665016a86df82f93cd599f56be769fd501d0546244f32ae88218821885af5a22'),
 (0x33b60,673,'7c44ba2ae7f97b1e6afefe2c6f5804bd74c0f7dc2ed6839e5b4e2bb5c90612de'),
 (0x328f0,208,'5c796e5cc3109c1c35937490fdf2369dfcfee4a561494cb5c301ca77e48d2209'),
 (0x17bc70,259,'23f539f2f89d2de6e5b3eb91eacbdb6c452bae717d3cd535941f9d09885a901e'),
 (0x17bb40,73,'b3e16da586c02f4caac367e3a088eca32574a84c08e58f2e9d0e14b13772148c'),
 (0x17c430,200,'46b43001e9c733a4b571a81a1833ed8a55ad3dce93674694f8232ebbf2bb8941'),
 (0xd4b4e,33,'1e15ada3a3f1545a09d4dfcf1e0e3bba87c20ec4f71ffb679b186950e2e25695'))
FUNCTIONS=((3,0x33b60,'Range','8da2e513c9aa4fa79d0766c2e205830eabd342808b7ccefb38fcc7e137ab21c3'),
 (38,0x17bd80,'Conversation','45867d5370752208437cb7fdcaf9e82db4de5ab5555e2d4a3dbc68516661e706'),
 (38,0x17bb40,'Skip','7b3d3ff3a292dcb44e24e63517abcb1a2e0e64ffee04fd66f1d94198f6afcafa'))
WEAPONS=(
 (0xd25b0,600,'31f4f4e833fe5b77bd0d0782d437d76f26a5b7c52d5f07d658f5b50ef6089926','a1a7d5238242ae18931cf69bdf0912861bc58894c04d6640917b6d3146786e26','WeaponDispatch'),
 (0xd2510,154,'d86a215b8ed9f1b6c17ea2d7a6bd7c1fc8f30c7518b18823c7144bf8d5e0c76b','8d0a91df70242a54669dad2292a1ced15ccf7e3dc7b2a62a3b462475c5406324','ManualHolster'),
 (0xd2830,34,'eb07edf10e49983e0a6c6707ab98146259bbd1c98a4db5e7ea8b7b6e35d45da2','c4857e76ba0d6826cf685620f0e9358c90e281248e5111687bab183238f57492','ForceHolster'),
 (0xd23e0,289,'50895f3ca62c9e072228800f6f427b24c07af30839a0654275d0c4a7f28b6d61','f1cf3dda3498299e23cf18139e23fab1bbfa0456b2624c67e6ebd1efecdbb964','ToggleHolster'),
 (0xd1ae0,125,'2132145aa5144b332db41fa2ec4eb6e45bc5b8c8a52ff5e12820799cc23cadd4','46c943decbb32ac67d42129093c677c3f585a3ef61bb6cc86baba59b251c90e9','FireOrRaise'),
 (0xcf8a0,738,'449bfa9edf3708384a8092885fd4ca50b34f69eeaff8e99835118de31220fc63','0f2d4b49572d5cc283d1b8e369d15b23f8b706c8450c0f1cac1dc4ded53078ac','EquipAnimation'))
WEAPON_GUARDS=(
 (21,0xd1a24,0xd1a32,'Attack','000D1ADB','80d7cf72bdd4805e1688f3dc71964b2e93d6858536894c359d50b72932eeb84e','77ab3b7cf89cb5aacf8af59645276dacaf16624e12df639a2a69388aae234694'),
 (21,0xd0c33,0xd0c3d,'ShieldAndAlternate','000D0C99','4c62eb2b922d79c3b3f41278e6d31ce1a33d5fa5ed6feea38708fd703fa35e88','fe9e312aaa2154c2caeb55a60b415fa13b8e7b612d6f86b18032dbdfa41a7e47'),
 (20,0xc4cdc,0xc4cea,'Switch','000C4EE0','3c9d0cfef5d8658eee668ca564a93c783726119d13ae7475b659e6a8dae46e96','1c77c10d23accf2b77925c4bdf07b09ae9651c87a311e983a8f7edf64260c045'),
 (20,0xca949,0xca958,'Interaction','000CA965','a5979030203ceafe45b2a4c2a355a672d8ebe63e7f3d5de2b005f5da58271182','dee36dcf645a7c10329418e6612268be2b83e2fe6df0178c063db19f3eea6e7b'))

def block(source,address):
    start=source.index(f'X86_FUNC(__imp__sub_{address:08X}) {{')
    return source[start:source.index(f'\nX86_WEAK_FUNC(sub_{address:08X}',start)]

def speech_clean(source):
    return re.sub(r'^\t(?:if\(!ctx.eax\))?TrialConversation[^\n]+\n','',source,flags=re.M)

def strip(source):
    source=source.replace(INCLUDE,'').replace(ENTRY,'').replace(WEAPON_ENTRY,'').replace(MANUAL_ENTRY,'')
    for seams in HOOKS.values():
        for _,hook in dict.fromkeys(seams):source=source.replace(hook,'')
    for address,end,size,prompt in FADES:
        hook=f'\tif(coop_interaction.Fades({str(prompt).lower()})){{ctx.esp+={size};goto coop_fades_{end};}}\n'
        source=source.replace(hook,'').replace(f'coop_fades_{end}:\n','')
    return source

def unprepare(area):
    for chunk in HOOKS:
        path=Path(area)/f'generated/game/x86_recomp.{chunk}.cpp'
        if path.exists():
            before=path.read_text();after=strip(before)
            if before!=after:path.write_text(after)

def prepare(area):
    area=Path(area);local=Path(__file__).parent
    image=(area/'assets/Default.xbe').read_bytes();base=struct.unpack_from('<I',image,0x104)[0]
    count,table=struct.unpack_from('<II',image,0x11c)
    sections=[struct.unpack_from('<6I',image,table-base+i*56) for i in range(count)]
    def raw(va,size):
        section=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4]);at=section[3]+va-section[1]
        return image[at:at+size]
    for va,size,digest in PINS:assert hashlib.sha256(raw(va,size)).hexdigest()==digest,hex(va)
    for va,size,digest,_,_ in WEAPONS:assert hashlib.sha256(raw(va,size)).hexdigest()==digest,hex(va)
    # Original event95 is registered to the actor-local boolean callback BD670.
    assert raw(0xbd670,15).hex()=='8b4424048a10889102020000c20800'
    assert struct.unpack('<I',raw(0x471060+0x9c,4))[0]==0x17c430
    sources={};extracted=''
    for chunk,seams in HOOKS.items():
        path=area/f'generated/game/x86_recomp.{chunk}.cpp';source=strip(path.read_text())
        originals={}
        for unit,address,name,digest in FUNCTIONS:
            if unit!=chunk:continue
            original=speech_clean(block(source,address))
            assert hashlib.sha256(original.encode()).hexdigest()==digest,hex(address)
            originals[address]=original
        for address,hook in seams:
            marker='\t// '+address+' ';assert source.count(marker)==1,address
            source=source.replace(marker,hook+marker,1)
        if chunk==21:
            for address,_,_,digest,name in WEAPONS:
                original=block(source,address)
                assert hashlib.sha256(original.encode()).hexdigest()==digest,hex(address)
                extracted+=original.replace(f'__imp__sub_{address:08X}','Original'+name)+'\n'
            for address,hook in ((0xd25b0,WEAPON_ENTRY),(0xd2510,MANUAL_ENTRY)):
                anchor=f'X86_FUNC(__imp__sub_{address:08X}) {{\n';assert source.count(anchor)==1
                source=source.replace(anchor,anchor+hook,1)
            for address,name in ((0xd25b0,'WeaponDispatch'),(0xd2510,'ManualHolster')):
                extracted+=block(source,address).replace(f'__imp__sub_{address:08X}','Coop'+name)+'\n'
        if chunk==38:
            assert hashlib.sha256(block(source,0x17c430).replace('\tTrialCoopInteractionClose(ctx,base,ctx.ecx);\n','').encode()).hexdigest()=='6c9f210eda231cd7d26fba491f893bf19922b5fa724c3d11c7ee272f9c3f8744'
            start='X86_FUNC(__imp__sub_0017BD80) {\n';assert source.count(start)==1
            source=source.replace(start,start+ENTRY,1)
            for address,end,size,prompt in FADES:
                marker='\t// '+address+' ';assert source.count(marker)==1
                hook=f'\tif(coop_interaction.Fades({str(prompt).lower()})){{ctx.esp+={size};goto coop_fades_{end};}}\n'
                source=source.replace(marker,hook+marker,1)
                marker='\t// '+end+' ';assert source.count(marker)==1
                source=source.replace(marker,f'coop_fades_{end}:\n'+marker,1)
        shared='#include "x86_recomp_shared.h"\n'
        source=source.replace(shared,shared+INCLUDE,1)
        for unit,address,name,_ in FUNCTIONS:
            if unit!=chunk:continue
            extracted+=originals[address].replace(f'__imp__sub_{address:08X}','Original'+name)+'\n'
            extracted+=block(source,address).replace(f'__imp__sub_{address:08X}','Coop'+name)+'\n'
        path.write_text(source);sources[chunk]=source
    turn=sources[38];start=turn.index('\t// 0017BC8B ');turn=turn[start:turn.index('\t// 0017BCBC ',start)]
    for name,body in [('OriginalTurn',strip(turn)),('CoopTurn',turn)]:
        extracted+='X86_FUNC('+name+') {\nX86_LOAD_FLAGS();\nuint32_t ca=0,cb=0;\n'+body+'}\n'
    lock=block((area/'generated/game/x86_recomp.19.cpp').read_text(),0xbd670)
    extracted+=lock.replace('__imp__sub_000BD670','OriginalControlLock')+'\n'
    end=block(sources[38],0x17c430)
    extracted+=strip(end).replace('__imp__sub_0017C430','OriginalConversationEnd')+'\n'
    extracted+=end.replace('__imp__sub_0017C430','CoopConversationEnd')+'\n'
    constants=(0x44c174,0x44c530,0x44bf94,0x44c1f8,0x44d948,0x4f9f20,*range(0x17c414,0x17c430,4),*range(0xd27d8,0xd2808,4))
    values='\n'.join(f'W32(0x{a:x},0x{struct.unpack("<I",raw(a,4))[0]:x});' for a in constants)
    fixture=(local/'test_xbe_coop_interactions.cpp').read_text()
    guards=''
    for unit,start,end,name,label,pin,digest in WEAPON_GUARDS:
        assert hashlib.sha256(raw(start,end-start)).hexdigest()==pin,hex(start)
        source=(area/f'generated/game/x86_recomp.{unit}.cpp').read_text()
        offset=source.index(f'\t// {start:08X} ')
        body=source[offset:source.index(f'\t// {end:08X} ',offset)]
        assert hashlib.sha256(body.encode()).hexdigest()==digest,hex(start)
        fall='false' if name=='Interaction' else 'true'
        taken='true' if name=='Interaction' else 'false'
        guards+=f'bool Native{name}Guard(X86Context& ctx,uint8_t* base) {{\nuint32_t ca=0,cb=0,cf=0,of=0,zf=0,sf=0,pf=0;\n'+body+f'\nreturn {fall};\nloc_{label}:return {taken};\n}}\n'
    fixture=fixture.replace('// WEAPON_GUARD_DECLARATIONS','\n'.join(f'bool Native{row[3]}Guard(X86Context&,uint8_t*);'for row in WEAPON_GUARDS))
    fixture=fixture.replace('// WEAPON_GUARD_BODIES',guards)
    calls=set(re.findall(r'sub_([0-9A-F]{8})\(ctx',extracted))
    declared='\n'.join(f'X86_EXTERN_FUNC(sub_{name});' for name in sorted(calls))+'\n'
    implemented=set(re.findall(r'X86_FUNC\(sub_([0-9A-F]{8})\)',fixture))
    # Unexercised state-machine services fail explicitly; no fake success for a
    # newly reached action/physics path in this focused conversation fixture.
    missing='\n'.join(f'X86_FUNC(sub_{name}){{std::fprintf(stderr,"unexpected native service {name}\\n");std::abort();}}' for name in sorted(calls-implemented))
    extracted=declared+extracted+'\n'+missing+'\n'
    for marker,body in [('// ORIGINAL_INTERACTIONS',extracted),('// ORIGINAL_CONSTANTS',values),('// PRODUCTION_CALL_GUEST',runtime_call_guest())]:
        assert fixture.count(marker)==1;fixture=fixture.replace(marker,body)
    (area/'src/compat/test_coop_interactions.cpp').write_text(fixture)
    for name in ('xbe_coop_interactions.cpp','xbe_coop_interactions.h'):
        (area/'src/common'/name).write_text((local/name).read_text())

"""Reject already consumed co-op items while retaining the original owner-local grant."""
from pathlib import Path
import hashlib
import re
import struct

INCLUDE = '#include "xbe_coop_pickups.h"\n'
ENTRY = '\tif(!TrialCoopPickupAllowed(ctx,base)){ctx.esp+=8;return;}\n'
NOTICE_CALL='\t{ uint32_t t = X86_MEM32((uint32_t)(ctx.edx + 0x1Cu)); X86_PUSH32(0xC51D6u); x86_icall(ctx, base, t); }'
OWNED_NOTICE_CALL=NOTICE_CALL.replace('{ uint32_t t', '{ TrialCoopPickupScope pickup_scope(ctx.esi,base); uint32_t t')
HOOKS = {
    0x4a290: [('X86_FUNC(__imp__sub_0004A290) {\n', '\tconst uint32_t pickup_channel=ctx.ecx;\n'), ('\t// 0004A390 pop edi', '\tTrialCoopPickupNotice(base,pickup_channel,ctx.eax);\n')],
    0x4a3a0: [('\t// 0004A43F pop edi', '\tTrialCoopPickupNoticeExpired(base,ctx.ebx);\n')],
    0x4b4d0: [('X86_FUNC(__imp__sub_0004B4D0) {\n', '\tTrialCoopPickupNoticesReset(ctx.ecx);\n')],
    0x4ba80: [('X86_FUNC(__imp__sub_0004BA80) {\n', '\tTrialCoopPickupTextScope pickup_text(ctx,base);\n\tif(pickup_text.skip_){ctx.esp+=4;return;}\n')],
}

BODIES = (
    (6,0x4a290,271,"f04a098336c051a2b5dd344cca262cade67cd6ca035fa596cf4ebb9a992967fd","4df26b45fcc16bbedb03b048769bd6e355b6409b74ac86dd1d1dadc63d29e105"),
    (6,0x4a3a0,183,"3fe15178653aceff7ddab45352cc250afedfee38d536cd16e2e3b8d3fafd840b","ef0e57a2daaadbc78b42cee2267fa958daab7dcb238e5ca47f4aec3332ddf839"),
    (6,0x4ab90,185,"cd421eeb23f5cc2bb6701b8bc7ff52a57617bd614a130753b2c2d956982242a8","bff25a180528fde02f9d76866b65961183a8a9658dac30f77ea6e0b6d3462512"),
    (6,0x4ba80,736,"6543170324bc1777c63bf462a37067d4b78073a2e8590f527d29c82ffaf92e39","229557a42db842f40d9445654428a69ac8df2a678898e2fa363dd7dd09b5d44d"),
    (6,0x4b4d0,546,"29a0e061d038e1f0532891cae7db45ea0f922bb36dff3788b1f6230f643ab558","787426a6b53b04023497cf71190bf5fbb31c7b66c5d81f6f696cc3566515feed"),
    (20,0xc5040,791,"84b01ab405994938568552abf7098a3ab66be488858aecc21556d1ec4103ad31","23303ad529152c7b28a12f1e72bc3220edd1c46b2d921b386d04862f8ef6ba9d"),
    (20,0xc7cc0,349,"49206f0404619be42194918a5525f6f696bc43fc5e797bfdb9872d58df340aab","b5a7358791db47dde85d428678e9056c398c568a2c0fbf6f1f5d8f52b3fba892"),
    (21,0xd8520,10,"aee5f49a048f993f18a3c9a35fd06a11282e601815ad232cb7741763d5a2db58","77f04937de5ceb51facb63f7afee6ebe8e6e7bba24cc48635320947ea3e00974"),
    (24,0xef600,55,"cf8f3fecc22f381a6b9d2bd66f3a8c415c3e9cf71adc40b75767f2f466b19fc6","6b62a86369ab309092f5a56c235395b717e402fb61924c9d6582452ef6e7282a"),
    (24,0xef820,58,"b0089ba8ddc87d73f418ef7fa133d60accf8fe59006b75fedc2e400160f43749","55f2ef783af1aed04a02426b8af8e26cac3feea0370f44dc043b231f1eb9d660"),
    (49,0x1eefd0,55,"200de1126fbc8a92cc783741d11fb73f9881e255afcdd301672f4bb37a8b0c4c","666d1d08f2cbde0006f566cf972677fe1eedb25573ae3da22f38748a48c44b78"),
    (5,0x41f30,29,"05aeb259989a65b561bdeb7a4beb855ba2b314a1bb7dad24e547d5a78bfd0132","fbf3360560d6194899636a583de4a6fa3f3fbe1ed38b7d39457cb0939f51b375"),
    (5,0x41fc0,24,"52b9a3e84d6d4799b05b4dc3b8cee3e487268ff18e588db331f7d806e5a47754","57e6eb740f40466a6558c7f047b7d10a4345d19ac19a7bab71a8a1695fb0ec14"),
    (5,0x43950,88,"bfdededa60ae67897653b0fc5c7946bb6b0e66eb9d3947bce3bcdbb10f75f4b1","8f8666c02c62403fb51f78ecc557f4cfc185ad6dbd732afdf90fd96cb1f835b2"),
    (5,0x42d60,47,"44e49a07cd832a6ecfa1ebbe6888428968b58d4f00dd4aeebd5db0742c5d5929","52efab5c02e8f801efc467ebac3c3d49dca2ebfb0dbf8be50d54b26e41145329"),
    (5,0x42150,82,"25b1d93031f02e88ce90e2ec01e53c31ce2573943229e60913b284300332e04a","bfaf6da11e6eaa4f1a8aed4eae82ce98656713b752bb4fcbb80e7ce01e13d834"),
    (5,0x43f20,200,"77a90feea0887d0e91efb98796f8aa7abcc5a907597eb8dff0049a68c66ca934","7a7f01deb9057385f0e0030d5bc963a1501756ed7d59737e0e6f06fc25c9a4da"),
    (5,0x437c0,133,"4df8b5d1ea98fc18341265bfc31753676cb854440c1df9ac9c1555e479cc8c68","f9db9bbf1e5aa0498d6adc67e68080929262828cefd8195dd9100f9403c5b285"),
    (5,0x43c00,116,"c30943a51c6d43b4aeeafdc571f6c51b38118e157b47c4e051453810c55b6f72","4a9fbca06aa4621a31f73d77961e485958500120ff20f169390f9ed4e533ad25"),
    (5,0x42c00,224,"dac45953b259dae1ab6ba4fbf2897fcd3b3d91dcffe26b467e3727e8bc166b5f","66110e6763eef07bab58227498a63d026cf0bbbd7d3b42ad691cdb38d5ece71a"),
    (5,0x42230,64,"105d89800795dd7d936e57671b988dfd83823b5b107422877eefac25fcf876c3","34b336144e2278c37f8639d46e67ff54413f4b7712893d4aa584206e76493bdf"),
    (50,0x1f8fc0,17,"0a7eca5142b896005cba82f453ebd149248db7f7aeabee37d4ac2d9a537fa32f","847d3fe1b9e6375243e533205c067c6d41465ae459fecfd33170c41e383efcab"),
    (50,0x1f90a0,56,"f7aa31f3736f27a11b0e94c7672dbb0a20a26401e7758b3140eb82cddbbef866","00c794ac196fd497ca8f5b3bb7492d420000e5a180b2357ba5b53b946905be16"),
    (50,0x1f7af0,74,"d6073750963275def6932975c80ca0a0c30cdc6fac72f193d319c5b115c8b780","d82fb7e9faa677c5e10d15256f313d8bafcc92e8f385b6a75b33500a0ff04ad2"),
    (52,0x20a140,38,"90ace56f24034b7e7014d411b752ed9e0e22bb23243aa13b80d243a7c6bcdb22","42a73726c365f79452f75eb46afeeca9185a41f8e3d4038a6a8b61af4def1ce9"),
    (52,0x207d70,39,"77e4292727d8c6a57d8403523d0c547c85054ccb4ba1a4a8da0a1e9df3b81993","eb0d2bbe96976c5c36636763698a136b93e103bf36f932f8222e643033933c6b"),
)

def body(source, address):
    return re.search(rf'X86_FUNC\(__imp__sub_{address:08X}\) \{{.*?^\}}', source, re.M | re.S).group()

def clean(source):
    source=source.replace(INCLUDE, '').replace(ENTRY, '').replace(OWNED_NOTICE_CALL,NOTICE_CALL)
    for hooks in HOOKS.values():
        for _,hook in hooks:source=source.replace(hook,'')
    return source

def unprepare(area):
    for unit in (6,20):
        p=Path(area)/f'generated/game/x86_recomp.{unit}.cpp'
        if p.exists():
            before=p.read_text();after=clean(before)
            if before!=after:p.write_text(after)

def prepare(area):
    area=Path(area);local=Path(__file__).parent
    xbe=(area/'assets/Default.xbe').read_bytes();base=struct.unpack_from('<I',xbe,0x104)[0]
    count,table=struct.unpack_from('<II',xbe,0x11c)
    sections=[struct.unpack_from('<6I',xbe,table-base+i*56)for i in range(count)]
    def raw(va,size):
        s=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4]);o=s[3]+va-s[1]
        return xbe[o:o+size]
    assert raw(0x44e3c4,14)==b'WEAPON_PICKUP\0'
    assert raw(0x44caec,12)==b'weapon type\0'
    assert raw(0x460898+8,4)==struct.pack('<I',0xef600)
    assert raw(0x47f6e8+8,4)==struct.pack('<I',0x1eefd0)
    sources={u:clean((area/f'generated/game/x86_recomp.{u}.cpp').read_text())for u in {p[0]for p in BODIES}}
    bodies={}
    for unit,va,size,raw_hash,lifted_hash in BODIES:
        assert hashlib.sha256(raw(va,size)).hexdigest()==raw_hash,hex(va)
        text=body(sources[unit],va)
        assert hashlib.sha256(text.encode()).hexdigest()==lifted_hash,hex(va)
        bodies[va]=text
    fixed={}
    for va,text in bodies.items():
        for marker,hook in HOOKS.get(va,[]):
            assert text.count(marker)==1,hex(va)
            text=text.replace(marker,marker+hook if marker.startswith('X86_FUNC') else hook+marker,1)
        if va==0xc5040:
            assert text.count(NOTICE_CALL)==1
            text=text.replace(NOTICE_CALL,OWNED_NOTICE_CALL,1).replace('X86_FUNC(__imp__sub_000C5040) {\n','X86_FUNC(__imp__sub_000C5040) {\n'+ENTRY,1)
        fixed[va]=text
    fixtures=[]
    for va,text in fixed.items():
        if va==0x4b4d0:continue # constructor reset is checked directly; no world factory in this fixture
        if va==0xc5040:
            fixtures += [bodies[va].replace('__imp__sub_000C5040','OriginalPickup'),text.replace('__imp__sub_000C5040','CoopPickup')]
        else:fixtures.append(text.replace('__imp__sub_','sub_'))
    for unit in (6,20):
        source=sources[unit]
        for u,va,*_ in BODIES:
            if u==unit:source=source.replace(bodies[va],fixed[va],1)
        source=source.replace('#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+INCLUDE,1)
        (area/f'generated/game/x86_recomp.{unit}.cpp').write_text(source)
    for name in ('xbe_coop_pickups.cpp','xbe_coop_pickups.h'):
        (area/'src/common'/name).write_text((local/name).read_text())
    test=(local/'test_xbe_coop_pickups.cpp').read_text()
    constants=sorted(set(int(x,16) for x in re.findall(r'\[0x([0-9A-F]{8})\]', bodies[0x4ba80]+bodies[0x4a3a0]+bodies[0x4a290])))
    constants += list(range(0xc5358,0xc5398,4))
    test=test.replace('// ORIGINAL_NOTICE_CONSTANTS',';'.join(f'Put(0x{x:x},0x{struct.unpack("<I",raw(x,4))[0]:x})' for x in constants if x not in (0x509704,0x51c4dc))+';')
    marker='// ORIGINAL_PICKUP_BODIES'
    assert test.count(marker)==1
    functions=set(re.findall(r'\bsub_[0-9A-F]{8}(?=\(ctx)', '\n'.join(fixtures)))
    functions.update(f'sub_{va:08X}'for va in bodies)
    declarations='\n'.join('X86_FUNC('+name+');'for name in sorted(functions))
    declarations+='\nX86_FUNC(OriginalPickup);\nX86_FUNC(CoopPickup);'
    (area/'src/compat/test_coop_pickups.cpp').write_text(test.replace('// ORIGINAL_PICKUP_DECLARATIONS',declarations).replace(marker,'\n'.join(fixtures)))

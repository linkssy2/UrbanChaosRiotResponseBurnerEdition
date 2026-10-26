"""Original multiplayer body children for validated local campaign actors."""
from pathlib import Path
import hashlib
import struct

PINS=(
    (0xd6070,371,'2ab2e1dbd3df4c76280d93cf62312b588dd348573640d8554b4dec77f1232b9b'),
    (0x41f50,36,'cd528813330e132d8e50a8be20e189735977eae9ad9f8985f9552d71b5a9422b'),
    (0x44c178,4,'2238f30df488055daf5e8f362cf904e370011a172d20df193c1b7ae8e5db8e01'),
    (0x4fa090,64,'d1010f232b82b3948a8282ed87c4af60e7433438448d530c9afb28356b515d76'),
    (0xc1e60,127,'1b39161878baaba8e8335113adc54b67c16c72b6f1eecbc9a82225856a71d928'),
    (0xe57b0,810,'ddd5e6d4df298bf823b097dfc0611622ceace4657e500fc76d4abd8cb30ac874'),
    (0x216bd0,62,'dccdb06342c6574db8eb4e7b9e0572245b12ef3328fb82876641fc9c342e391f'),
    (0x276e0,3,'01cb47d078b4841b8408ec4fe278efa83115c6f6e101972987507d2b2b57dcf0'),
    (0x46fcb0,4,'f3e124c48e112b7f61ec45297a771e4d1bad551c154d09df50b3fb7119f4011c'),
    (0x8a080,10,'829e278dcbfd8e13da532961df17bf43f1a6db73a0023129954fb1ca0bf1305f'),
    (0x8a090,4,'b3404b9313e368df73583e10d29d8d5568e5bc326ed5f447a7d9bba6b2c31444'),
    (0xb5e80,10,'f830869d60400819128314aafe38f1f20b865adca35ba1f829d168e22afca9ae'),
    (0xb5e90,4,'b0b7cb280e0f183e8088b419423d32cefe14dfde3785a7245a47f4ff1eafb3da'),
    (0x457890,8,'5736d06ea1e3da3c8ae423570471f11344abf9eab8b666b0e2c3ece156137491'),
    (0x456a88,8,'d974ef2c383d55a9a2270af66f1d5f785a0fc8f03a7fc72687f93ecc6e491890'),
    (0x968a0,112,'4ed3432a307928e6d360f559d460838af6d2f501d6ddbded32dd184c1b1bdcea'),
    (0xca620,621,'46ba643e27fd9d647044e7b945e9d397a73ccdba0dec1c44f2ed184142de53d7'),
    (0x2c2b60,3,'4bc724f3b1d0caf4fe369c18cba3102e6c4ea057f63fe1587e3973134a7f755e'),
    (0x46fca8,4,'d2a6c9dfbb7f1c69550dc7dd4ed10a6e63263c54a870d7efa19ae6dc84a4a968'),
    (0xe8240,1329,'fa3f1883d8328cd3c6687ce13c5fc81c5a686e34699697d9c1aae91cd71e2957'),
    (0xe8780,94,'587de7a24f82384abef9c9737f8313e160f17f363087d01ef58a2d59f53a8cf9'),
    (0xe6110,2602,'f6b7d36a0c211aac4532b5ca3c3d83d12016700f3438dd9e38cfaa6bb2e14894'),
    (0xe6b40,3255,'be33816e0766bbe04225398c9105be8a5de83cee48486ff295c6feea16d2e2c8'),
    (0xe4300,50,'0dd10961c085609925d578f3b32cbae2c9d77b07563644fb5221db58a50fae5c'),
    (0xe7800,41,'f26f214f0298cbf0ce3a36468c0396bf8a9eadc1c9e3913dc73f4b2f07be6e7e'),
    (0xe4600,15,'2f6ada23f916d43cfefc654e44986c9a5edaadb9e5f6fad6c159cf9d48d7bbc7'),
    (0xd6880,637,'5147a80d986548f943864d1b7b3a332908d26692369b37a672f0281d429d1706'),
    (0x215060,794,'a3710b17e87403e3138092af5081882c84c6af71c03b249ee4122961d611167d'),
    (0x214ab0,933,'c164d22a13705f4df3a9ae32e8290a8071d2e6861e27447a546772aecd3ee7ab'),
    (0x208d40,22,'cd7064804d7f0069ffa4e573d8eb8b214785b9c0c09fe54d3842cf87811a5ab2'),
    (0x21e0a0,33,'7c06affe96c5daed64b9aaf92cf6d25e8c0ee374a0fca5272355af60285ec0e3'),
    (0x172e80,144,'0b6ab03a379b4e03d1daacd2ce9a571440ec1deaf615460fe8770773c16749fe'),
    (0xe7fd0,16,'2b3beee41df96e6de90f9819825ecd264e293c4353fdeaa735cd2a0ff2cc2d06'),
    (0x1f8e30,13,'15b6620f25e85ec64bbe906f164cf06783dc9a9a3c4af3bc60769b96e77b3ee4'),
    (0x207d20,70,'ab155c50429f53937391baab4aadadad17ac3859414867ead9463c6c02735504'),
    (0xe55f0,346,'9b63482b7de38115789d9d2007a7c002c5a5a6796d3525459a74924f487271ba'),
    (0x422c0,64,'620a0a445a48065bce388380b3b58433c71e793ec6d2be6430ae1bf1d986828a'),
    (0x173cf0,128,'6610e4e87e987b2290c4212f4600bbd19dd6838038871ec02af5a4c26d61b7bc'),
    (0x1ada40,64,'55626470cfefde7d4fbe88bebb83f5f1272e0c93993b0113e6fe9d5c16e23895'),
    (0x1abdf0,54,'926fcf3868f412203cf5460408baaf1729a72b4d7b3e910ed5d8b08188d82fb7'),
    (0xfd7f0,178,'08b03827d0b8f16b5c9010ac4014e51b2a7ebc337dbcf1fc6d502bc4c116fd50'),
    (0xfd530,693,'fed2aa5cef0c7e147ced4d573037214f9faa3832b56af91eaa7e4cbcf2832e03'),
    (0x2091a6,341,'b472df3f41aab0553ee41366253ce5f83470e817fe8c0c184067680007ef10ca'),
    (0x20a620,106,'87bede7b2052da2b44bdfcfe4e2f70a5f0491a3e01e59573d99837cdc060c5cf'),
    (0x207ff0,239,'cbb085e7333227f9e82c86e95860ae5675e450cdd96b3a6ca00afbbc3b74faf2'),
    (0x218b30,38,'416f7149b276de7d67e90ea6cecb0b1498b500a8697135ad17c5597198f70ce6'),
    (0x462658,56,'c690f66d7648f62ff6646f53473d2aa7f96929f7e6456b71232a005e65f0f8eb'),
    (0x470058,64,'76e88ec1b7eff15eda5ad5d650e3d3b1672daffa6e57ec7d70b6205e27f43dbf'),
    (0x472fc8,64,'0d812227f23a3c0728dd73832b77ed0bab3288d17e970aab012b0c391504a9ed'),
    (0x187af0,51,'c137a966468522add2b4c3dc9ab55b5f07237c56f93ab1708f1b4cdf93d74ea0'),
)

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
    for name in ('xbe_coop_body.h','xbe_coop_body.cpp'):
        (area/'src/common'/name).write_text((local/name).read_text())
    hooks={
        19:[('\t// 000C1E74 cmp edi, 0x03','\tTrialCoopBodyAction(ctx,base);\n')],
        20:[('\t// 000CA851 test eax, eax','\tTrialCoopBodyResolve(ctx,base);\n'),
            ('\t// 000CA864 mov ecx, eax','\tTrialCoopBodyResolve(ctx,base);\n'),
            ('\t// 000CA87A mov ecx, eax','\tTrialCoopBodyResolve(ctx,base);\n'),
            ('\t// 000CA881 mov ecx, esi','\tTrialCoopBodyWeapon(ctx,base);\n')],
        23:[('X86_FUNC(__imp__sub_000E7FD0) {\n','\tTrialCoopBodyDestroyed(ctx,base);\n')],
        37:[('X86_FUNC(__imp__sub_00172E80) {\n','\tTrialCoopBodyDestroyed(ctx,base);\n')],
        52:[('\t// 00209258 mov ecx, [ebp+0x04]','\tTrialCoopBodiesRetire(ctx,base);\n')],
        54:[('X86_FUNC(__imp__sub_00216BD0) {\n',
             '\tif(TrialCoopBodyAnimationSoundSuppressed(ctx,base)){ctx.eax&=0xffffff00u;ctx.esp+=16;return;}\n'),
            ('X86_FUNC(__imp__sub_00215060) {\n','\tTrialCoopBodyView trial_coop_body_view;\n'),
            ('\t// 00215222 mov eax, [esp+0x34]',
             '\tif(TrialCoopSkipUnreadyView(ctx,base)){ctx.esi=ctx.edi;goto loc_0021532D;}\n'),
            ('\t// 0021525D push 0xFF','\ttrial_coop_body_view.Select(ctx,base);\n'),
            ('\t// 0021532D inc esi','\ttrial_coop_body_view.Clear();\n')],
    }
    include='#include "xbe_coop_body.h"\n'
    sync_callers=''
    action_callers=''
    action_sounds=''
    for chunk,seams in hooks.items():
        path=area/f'generated/game/x86_recomp.{chunk}.cpp';source=path.read_text().replace(include,'')
        # Shared hook text must be removed before inserting any of its call sites.
        for hook in dict.fromkeys(hook for _,hook in seams):
            source=source.replace(hook,'')
        if chunk==19:
            start=source.index('X86_FUNC(__imp__sub_000C1E60) {')
            end=source.index('\nX86_WEAK_FUNC(sub_000C1E60',start)
            assert hashlib.sha256(source[start:end].encode()).hexdigest()=='aa54c345c3494ff314b88f2b896aa18c488846f4f2531f89903eda74b1eae5c4'
            action_callers=source[start:end].replace('__imp__sub_000C1E60','OriginalBodyActionCaller')+'\n'
        if chunk==54:
            start=source.index('X86_FUNC(__imp__sub_00216BD0) {')
            end=source.index('\nX86_WEAK_FUNC(sub_00216BD0',start)
            assert hashlib.sha256(source[start:end].encode()).hexdigest()=='3e2658000b3f4307b5b7f3d9ba39d0696f12878c5b5c9274ee67c4a9366cf3c4'
            action_sounds=source[start:end].replace('__imp__sub_00216BD0','OriginalBodyActionSound')+'\n'
        if chunk==20:
            start=source.index('\t// 000CA847 mov eax, [esi]')
            end=source.index('\t// 000CA881 mov ecx, esi',start)
            original=source[start:end]
            assert hashlib.sha256(original.encode()).hexdigest()=='6f285452b277dee5138dd9b3a4e64b33770bb6539c9dfd0d7d567dd29ff774db'
            sync_callers='X86_FUNC(OriginalBodySyncCaller) {\n\tX86_LOAD_FLAGS();\n\tuint32_t ca=0,cb=0;\n'+original+';\n}\n'
        for anchor,hook in seams:
            assert source.count(anchor)==1,anchor
            source=source.replace(anchor,anchor+hook if anchor.startswith('X86_FUNC') else hook+anchor,1)
        for hook in dict.fromkeys(hook for _,hook in seams):
            assert source.count(hook)==sum(candidate==hook for _,candidate in seams),hook
        if chunk==19:
            start=source.index('X86_FUNC(__imp__sub_000C1E60) {')
            end=source.index('\nX86_WEAK_FUNC(sub_000C1E60',start)
            action_callers+=source[start:end].replace('__imp__sub_000C1E60','CoopBodyActionCaller')+'\n'
        if chunk==54:
            start=source.index('X86_FUNC(__imp__sub_00216BD0) {')
            end=source.index('\nX86_WEAK_FUNC(sub_00216BD0',start)
            action_sounds+=source[start:end].replace('__imp__sub_00216BD0','CoopBodyActionSound')+'\n'
        if chunk==20:
            start=source.index('\t// 000CA847 mov eax, [esi]')
            end=source.index('\t// 000CA881 mov ecx, esi',start)
            patched=source[start:end].replace('\tTrialCoopBodyWeapon(ctx,base);\n','')
            sync_callers+='X86_FUNC(CoopBodySyncCaller) {\n\tX86_LOAD_FLAGS();\n\tuint32_t ca=0,cb=0;\n'+patched+';\n}\n'
        source=source.replace('#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+include,1)
        path.write_text(source)
    test=(local/'test_xbe_coop_body.cpp').read_text()
    source=(area/'generated/game/x86_recomp.21.cpp').read_text()
    first=source.index('\t// 000D608F lea edi, [esi+0x498]')
    last=source.index('\t// 000D60D6 mov ecx, [esi+0x890]',first)
    selector=source[first:last]
    assert hashlib.sha256(selector.encode()).hexdigest()=='f7d6dc44add98c86c7f1601f0bc1955390392ac4adb7da4cd54dc29e9b3f85e9'
    # This original local-player selector ends before the network state write.
    # Return the chosen ID while balancing its saved EBX and our test return.
    assert test.count('// ORIGINAL_BODY_WEAPON_SELECTION')==1
    test=test.replace('// ORIGINAL_BODY_WEAPON_SELECTION',
        'X86_FUNC(OriginalBodyWeaponSelection) {\n\tX86_LOAD_FLAGS();\n\tuint32_t ca=0,cb=0;\n'+selector+
        '\tctx.eax=ctx.ebx;ctx.ebx=X86_MEM32(ctx.esp);ctx.esp+=8;\n}\n')
    source=(area/'generated/game/x86_recomp.25.cpp').read_text()
    first=source.index('X86_FUNC(__imp__sub_000FD7F0) {')
    last=source.index('\t// 000FD8A2 ',first)
    prelude=source[first:last]
    assert hashlib.sha256(prelude.encode()).hexdigest()=='83511b044653dbca81a0cc47769bd192bf23094b4679a9e0f44643665c2a1b1e'
    test=test.replace('// ORIGINAL_ATTACHMENT_PARENT_READ',prelude.replace('__imp__sub_000FD7F0','OriginalAttachmentParentRead')+
        '\treturn;\nloc_000FDF07:\n\treturn;\n}\n')
    source=(area/'generated/game/x86_recomp.52.cpp').read_text()
    first=source.index('\t// 002091EF mov esi, [ecx+0x04]')
    last=source.index('\t// 002091FB ',first)
    test=test.replace('// ORIGINAL_WORLD_UPDATE_ADMISSION',
        'X86_FUNC(OriginalWorldUpdateAdmission) {\n\tX86_LOAD_FLAGS();\n\tuint32_t ca=0,cb=0;\n'+source[first:last]+
        '\tctx.eax=1;return;\nloc_002091D1:\n\tctx.eax=0;\n}\n')
    assert test.count('// ORIGINAL_BODY_SYNC_CALLER')==1
    test=test.replace('// ORIGINAL_BODY_SYNC_CALLER',sync_callers)
    for marker,source in [('// ORIGINAL_BODY_ACTION_CALLER',action_callers),
                          ('// ORIGINAL_BODY_ACTION_SOUND',action_sounds)]:
        assert test.count(marker)==1
        test=test.replace(marker,source)
    for unit,address,marker in [(19,'000C1EDF','// ORIGINAL_BODY_ACTION_TAIL'),
            (5,'00041F50','// ORIGINAL_INVENTORY_SHIELD_GATE'),
            (23,'000E4300','// ORIGINAL_BODY_TRANSFORM'),
            (23,'000E7800','// ORIGINAL_BODY_CROUCH'),(23,'000E4600','// ORIGINAL_BODY_VISIBILITY'),
            (13,'0008A080','// ORIGINAL_RENDER_SET_VISIBLE'),(13,'0008A090','// ORIGINAL_RENDER_GET_VISIBLE'),
            (18,'000B5E80','// ORIGINAL_ANIMATED_RENDER_SET_VISIBLE'),(18,'000B5E90','// ORIGINAL_ANIMATED_RENDER_GET_VISIBLE'),
            (15,'000968A0','// ORIGINAL_RENDER_ADMISSION'),
            (52,'0020A620','// ORIGINAL_ENTITY_RETIREMENT'),
            (39,'00187AF0','// ORIGINAL_FLARE_RETIREMENT'),
            (52,'00208D40','// ORIGINAL_VIEW_COUNT'),(55,'0021E0A0','// ORIGINAL_VIEWPORT_ASSIGN'),
            (54,'00214AB0','// ORIGINAL_RENDER_PARAMETERS'),(54,'00215060','// ORIGINAL_WORLD_RENDER')]:
        source=(area/f'generated/game/x86_recomp.{unit}.cpp').read_text()
        first=source.index('X86_FUNC(__imp__sub_'+address+') {')
        last=source.index('\nX86_WEAK_FUNC(sub_'+address,first)
        assert test.count(marker)==1
        test=test.replace(marker,source[first:last])
    (area/'src/compat/test_coop_body.cpp').write_text(test)

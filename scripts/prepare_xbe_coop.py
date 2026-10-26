"""Default-disabled campaign co-op admission through the original actor factory."""
from pathlib import Path
import hashlib
import re
import struct

PINS=(
    (0x159c0,103,'73b9c80a5ff6160d668a25177ef9aef36ad5db2b69687f51b60211ca522a4bf5'),
    (0x1726a0,559,'8ca985df922f96c2e275e5cd670d94e88446996366b9f01401129d76f169b94d'),
    (0x172d60,248,'faf7326ecc9d8f65405146b581c70eed6443eba95f5656d94897e76bc06c4002'),
    (0x172e80,144,'0b6ab03a379b4e03d1daacd2ce9a571440ec1deaf615460fe8770773c16749fe'),
    (0x172f10,94,'b05aac851f8d4d758306bb77fa2b8056a0db42b8c55c577a474a25100ed8106f'),
    (0xd47e7,85,'bb500e34c94bc663425c9d837e3a00d42a3c2e98250eb83e521260c160ce953d'),
    (0xd4a8e,49,'e39bad7407946c6eea380027776dda8759c45d792f67a524a8be00dcf19a0a9f'),
    (0x207d20,70,'ab155c50429f53937391baab4aadadad17ac3859414867ead9463c6c02735504'),
    (0x207cd0,70,'7a070f23441e8d720a92bda17644a1bd3191fcf0781c21aa8c6041669feea841'),
    (0x207e50,170,'9a75393b805742d3280bcae5b3299d7f98f843a2699200391514425f66d9a1f6'),
    (0x221ba0,249,'078a9931110e38463c0adcf31b190eade7c7c19ad9160867b344a26fe03f440c'),
    (0x221ca0,90,'1a95da509d0f39f730747b903238ffe681e3e821b2ef30a56dad6008ff21e479'),
    (0x20a5e0,39,'c54c059d1d27eb4d480f74d62ca11007cf50cf5532adde63fc3fab06a6bc9925'),
    (0x20b83f,64,'bec9f5e8a53f61c329ea9f1276fa4f689d334291d8d8a9680acd9ba681bbf003'),
    (0x20bdc0,337,'cd89678a591cf83650d197814f8eba7954c0e78d114b445ac08571f3a4fbd19e'),
    (0x215d00,522,'fdb8d1c8059d5358db5b376cfc722e7997c34421880151ff54398d8e4f0075f5'),
    (0x1fbc50,210,'49f1e2ba24dd1c553a8e3439e7770030e7806baaa835a8f5199271263827316a'),
    (0x46fbe8,204,'f7efd1f6a5d6a91463dc73d83b0ac396a321509ebe485fe9671f7ff9f0edb4ac'),
    (0x55be0,11,'bd87d6c3d55862e6c811f4054425d0f49f68aa22cff7ac8ac5ce61046f1b1db4'),
    (0xcc38f,124,'0818dcfd1284eed2c8269cae2cfb4e6d826378e28c2e4937d353d176fc1a6302'),
    (0xcc98c,11,'cdd18a9e2643b206f54327f17b1fb31e6a44758fd06661047df1dc183ae75d94'),
    (0xccb98,36,'ff2495b4c527df57a2ed4271601496a12f8ac44dbcc5200175017856c9ef0a13'),
    (0x56260,148,'3ac66aea6fa49e38097950d0546a42ab1be40eaf3f93943925f6e5c54d629a96'),
    (0x2107f0,231,'2b415cce42da942555edcab7647c8dee649a87bfdd922f21db65eb2703d5394d'),
    (0x4acd0,9,'3d88229ba8f81117aa293b673da37176d12ed5fdadb9eeb56bfda6da97e01c38'),
    (0x141a20,35,'43604cbae663aa168af2746ce77e7ca139292a83853c16cab36ce5ad3209c2f8'),
    (0x141b30,55,'edde87ec0b7b16d7db746287d70294fcc43b9a61420786af247fc6ad8ba5a544'),
    (0x1424b4,37,'31aa3a5eea31494365cd12e883ccd1b370af7e6af9579e50f87fcb5f9f1f497d'),
    (0x142650,899,'30cb9dc999ecd653e7e36cb71da0dc185ce228f817fc550bc196d646ce4c3714'),
    (0x142ab0,89,'1ecf27b545a81fd6f43ce58edb8c2f1ff8d4e97c96ee1868c6f00c18c8ac604c'),
    (0x143ae0,64,'3aa4a86a8e4b97f390f10fca3cc57d49de4ab449ec3bc7c51fbde1fde5c1cdc4'),
    (0x172380,104,'8dda0ed13040f9fab66e366e01e6e12e07c0ee3e7be3c76b23443de344962b49'),
    (0x2180a0,34,'424f6d55ac90e45c2d9491ae63804bd7142dea228a27fd7b86ca8ad55251ec6a'),
    (0x218120,7,'c9af73197615758bcbfce6464123bda69bcd94dae6cc0bde4066de331201cee1'),
    (0x2185e4,39,'518d93ec4906ef042a193d7d2baa68aa2d9a72b107243e6bdaa95f8a466d0043'),
    (0x20e019,15,'c747089ecbcb13d4703560c37ff53fa751eb3f5e58006eacee4134ae7fbc8ef9'),
    (0x214f40,72,'0a4e36919365c6e6171a3524eba1d9611038645261b185fc78e255c00d723679'),
    (0x173cf0,128,'6610e4e87e987b2290c4212f4600bbd19dd6838038871ec02af5a4c26d61b7bc'),
    (0x1ada40,64,'55626470cfefde7d4fbe88bebb83f5f1272e0c93993b0113e6fe9d5c16e23895'),
    (0x1abdf0,54,'926fcf3868f412203cf5460408baaf1729a72b4d7b3e910ed5d8b08188d82fb7'),
    (0x4807dc,4,'b4f7d318e86447bc021381e223fc4369b9ed1882c4aef31d517a838278cb258b'),
    (0x9e6a2,191,'ff41c2b58286d67a2db1051e200309f6abb14824b10bf3781230151465c30631'),
    (0x9f590,160,'07f358c7b8c98c7e8065804b0013fa9347f3fa79ae7a71eae91d25e3d73c411e'),
    (0x3f1cd8,24,'e53fe34c8ce88dfe4b48c4b9b013b16413ef6960352fc01d807d1f05478875e1'),
    (0x3e321c,18,'2f87f8b789d4190ce0b39da7ec751af5c7c0e07a6a662bcc49e781f0134b4768'),
    (0x3e82b3,10,'53d8024925a773accd9d792f980588f8c6375e0318520caefd5a45daf40ddf32'),
    (0x3e2b80,51,'a30543dcc4ed906e6b2f8188b7c5b6437598e8e791d484abe76ce9003b52febf'),
    (0xd0c30,488,'adb1022a9cd5d5eaf16a7528de5cc1a19332ecdedfb0a83a6e685cab92662a96'),
    (0x2084e0,10,'b1d7381cf9278dc64c2869ef57a6e9226d9142bfd7a282d0650c3cf953ec6cbe'),
    (0x2107c0,47,'03ec68a2d700219a9bc109f4b02947a80afc5a805ace0554e50c6d486b420198'),
    (0xbbc20,121,'0a4cd2e0b398b57c2b5ef02604e974b10eb2887bc3436e16befe6b634b92969a'),
    (0xc28ca,12,'e1c686e613b432044b6df7247bba9f3cf7467feffae865dbaa10d35cd9f8a91e'),
    (0xd493d,138,'a9f4bda54872d6a296d377551187729505118fa3a7f00db2ffc1764207499df6'),
)
SHIELD_BODIES={
    0x2084e0:(52,'ac534c14e440f547fc4b3995c429dbe70fd8a65f4fe25d2ff22b01f3785ee350'),
    0x2107c0:(53,'68dd06077ca8eadb78393eda7b46c0ff226130fb2311d9f0c7bdfac954f86c04'),
    0xd0c30:(21,'58b660fa02e77e3311285dc86253a7c3c4d8b308799f46b313c16d6e8facfe6d'),
}
STARTUP_BODIES={
    0x2180a0:(54,'934c1d1bd89f0665eda05195aa1bf8499cb202e98ee7e71798924aca9d600037'),
    0x218120:(54,'98986e1f27a70a0afca0b839da9e97e17a45ba34c7c6fbd96a7f1f6ddf5d64b0'),
    0x172380:(37,'10eea431c7a3273f6b1c1bd64f5b19993d402ee0f177ef649e06fe05e7c2deb6'),
    0x207cd0:(52,'6c25d504e24dcabeddb694f68c9386b019363c27601fc0faaa4617140f64069e'),
    0x207d20:(52,'1a60a9a9fa4c37e36e4385fdae94f81cd4bc0338068d06eff7525b3520518ebc'),
    0x207e50:(52,'6a4efa26e9594843c161e2806049290d1593a2a2fe62a2e0a3f7ce18565d56c6'),
    0x221ba0:(55,'eea1c8ef1c8b088e88753bf97cdcbae97e04eae51948e52b409824d1ebd1ab9c'),
    0x221ca0:(55,'476fd8040c6088eb580855de03d532dce74de141fabb46c78305ec52f71f160f'),
}
STARTUP_TRACE='\tTrialCoopTraceStartup(ctx,base,ctx.ecx,TrialCoopTraceStage::Update);\n'

def validate(xbe):
    base=struct.unpack_from('<I',xbe,0x104)[0]
    count,table=struct.unpack_from('<II',xbe,0x11c)
    sections=[struct.unpack_from('<6I',xbe,table-base+i*56) for i in range(count)]
    for va,size,digest in PINS:
        section=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4])
        offset=section[3]+va-section[1]
        assert hashlib.sha256(xbe[offset:offset+size]).hexdigest()==digest,hex(va)

def prepare(area):
    area=Path(area);local=Path(__file__).parent
    validate((area/'assets/Default.xbe').read_bytes())
    for name in ('xbe_coop.h','xbe_coop_pause.h','xbe_coop_guest.h','xbe_coop_guest.cpp'):
        (area/'src/common'/name).write_text((local/name).read_text())
    hooks={
        16:[('\t// 0009E761 mov ecx, [0x00519F60]','\tTrialCoopRestoreViewViewport(ctx,base);\n')],
        21:[('\t// 000D480A call 0x000BBC20','\tTrialCoopPlayerSlot(ctx,base);\n'),
            ('\t// 000D4AA1 call [edx+0x04]','\tTrialCoopPlayerAlias(ctx,base);\n')],
        32:[('X86_FUNC(__imp__sub_00141A20) {\n','\tbool trial_coop_death_admitted=false;\n'),
            ('\t// 00141A2A mov eax, [esp+0x04]','\tif(!TrialCoopDeathSequenceAllowed(ctx,base)){ctx.esp+=12;return;}\n\ttrial_coop_death_admitted=true;\n'),
            ('\t// 00141A40 ret 0x08','\tif(trial_coop_death_admitted)TrialCoopDeathSequenceAdmitted(ctx,base);\n'),
            ('X86_FUNC(__imp__sub_00141B30) {\n','\tconst uint32_t trial_coop_death_sequence=ctx.ecx;\n'),
            ('\t// 00141B3E push eax','\tTrialCoopDeathPlayer(ctx,base,trial_coop_death_sequence);\n'),
            ('\t// 001424C7 push eax','\tTrialCoopDeathPlayer(ctx,base,ctx.esi);\n'),
            ('\t// 001428EB push eax','\tTrialCoopDeathPlayer(ctx,base,ctx.esi);\n'),
            ('\t// 001429B1 push eax','\tTrialCoopDeathPlayer(ctx,base,ctx.esi);\n'),
            ('\t// 00142AE9 mov ecx, eax','\tTrialCoopDeathPlayer(ctx,base,ctx.esi);\n'),
            ('\t// 0014285B mov ecx, eax','\tTrialCoopDeathRumble(ctx,base,ctx.esi);\n'),
            ('\t// 00142897 mov ecx, eax','\tTrialCoopDeathRumble(ctx,base,ctx.esi);\n')],
        37:[('\t// 00172F4E mov ecx, [esp+0x04]','\tTrialCoopMainplayerCreated(ctx,base);\n'),
            ('\t// 00172DEF mov ecx, [esi+0x60]','\tTrialCoopPlayerLayer(ctx,base);\n'),
            ('X86_FUNC(__imp__sub_00172380) {\n',STARTUP_TRACE),
            ('X86_FUNC(__imp__sub_001728D0) {\n','\tconst uint32_t trial_coop_start_owner=ctx.ecx;\n\tTrialCoopTraceStartup(ctx,base,trial_coop_start_owner,TrialCoopTraceStage::CameraEnter);\n'),
            ('\t// 001728BC mov ecx, [esp+0x2D4]','\tTrialCoopTraceStartup(ctx,base,trial_coop_start_owner,TrialCoopTraceStage::CameraExit);\n\tTrialCoopPlayerStarted(ctx,base,trial_coop_start_owner);\n'),
            ('X86_FUNC(__imp__sub_00172E80) {\n','\tTrialCoopPlayerDestroyed(ctx,base);\n')],
        53:[('\t// 0020E028 push ecx','\tTrialCoopAudioListenerCamera(ctx,base,ca);\n')],
    }
    include='#include "xbe_coop_guest.h"\n'
    listeners=''
    audio_selection=''
    viewport_functions=''
    for chunk,seams in hooks.items():
        path=area/f'generated/game/x86_recomp.{chunk}.cpp'
        source=path.read_text().replace(include,'')
        for anchor,hook in seams:
            source=source.replace(hook,'')
        if chunk==16:
            effect=re.search(r'X86_FUNC\(__imp__sub_0009F590\) \{.*?^\}',source,re.S|re.M)[0]
            assert hashlib.sha256(effect.encode()).hexdigest()=='a5812aedfd46648ad50cf19cc5bb6dae882347963897c022862e51d404ac9681'
            viewport_functions=effect.replace('__imp__sub_0009F590','sub_0009F590')+'\n'
            start=source.index('\t// 0009E6A2 mov eax, [esi]')
            end=source.index('\t// 0009E761 mov ecx, [0x00519F60]',start)
            caller=source[start:end]
            assert hashlib.sha256(caller.encode()).hexdigest()=='8128b86297aa8ebdb82b932f469e47ae64a37200dba31ae7cb35a4b8e448d42c'
            for name,hook in (('OriginalViewportCaller',''),('CoopViewportCaller',seams[0][1])):
                viewport_functions+=f'X86_FUNC({name}) {{\n\tX86_LOAD_FLAGS();\n\tuint32_t ca=0,cb=0;\n'+caller+hook+'}\n'
        if chunk==37:
            # Upgrade previous candidates as well as repeat this preparation.
            source=source.replace('\tconst uint32_t trial_coop_start_owner=ctx.ecx;\n','')
            source=source.replace('\tTrialCoopPlayerStarted(ctx,base,trial_coop_start_owner);\n','')
        if chunk==32:
            listener=re.search(r'X86_FUNC\(__imp__sub_00141A20\) \{.*?^\}',source,re.S|re.M)[0]
            assert hashlib.sha256(listener.encode()).hexdigest()=='54b9e907196c2276a41c8a0d919a47f169a75739d68beee765df0fd1cc800ec7'
            listeners=listener.replace('__imp__sub_00141A20','OriginalDeathSequence')
        if chunk==53:
            start=source.index('\t// 0020E019 xor ecx, ecx')
            end=source.index('\t// 0020E028 push ecx',start)
            branch=source[start:end]
            assert hashlib.sha256(branch.encode()).hexdigest()=='4c8d79033cecfcbed8b327f67ad5a8ebc5d3dff3d4909155997f4117e7d3d6c3'
            for name,hook in (('OriginalAudioListenerSelection',''),('CoopAudioListenerSelection',seams[0][1])):
                audio_selection+=f'X86_FUNC({name}) {{\n\tX86_LOAD_FLAGS();\n\tuint32_t ca=0,cb=0;\n'+branch+hook+'}\n'
        for anchor,hook in seams:
            assert source.count(anchor)==1,anchor
            source=source.replace(anchor,anchor+hook if anchor.startswith('X86_FUNC') else hook+anchor,1)
        source=source.replace('#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+include,1)
        if chunk==32:
            listener=re.search(r'X86_FUNC\(__imp__sub_00141A20\) \{.*?^\}',source,re.S|re.M)[0]
            listeners+='\n'+listener.replace('__imp__sub_00141A20','CoopDeathSequence')
        path.write_text(source)
    fixture=(local/'test_xbe_coop_guest.cpp').read_text()
    source=(area/'generated/game/x86_recomp.54.cpp').read_text()
    resolver=re.search(r'X86_FUNC\(__imp__sub_00214F40\) \{.*?^\}',source,re.S|re.M)[0]
    assert hashlib.sha256(resolver.encode()).hexdigest()=='d6085790326e68779697688f43740559d35db6ed0cb3e4465f9b2d0e60a4ce1e'
    audio_selection+='\n'+resolver.replace('__imp__sub_00214F40','sub_00214F40',1)
    assert fixture.count('// RETAIL_DEATH_LISTENERS')==1
    startup=[]
    for va,(unit,digest) in STARTUP_BODIES.items():
        source=(area/f'generated/game/x86_recomp.{unit}.cpp').read_text()
        body=re.search(rf'X86_FUNC\(__imp__sub_{va:08X}\) \{{.*?^\}}',source,re.S|re.M)[0]
        assert hashlib.sha256(body.replace(STARTUP_TRACE,'').encode()).hexdigest()==digest,hex(va)
        startup.append(body.replace('__imp__sub_','sub_'))
    assert fixture.count('// RETAIL_STARTUP_FUNCTIONS')==1
    fixture=fixture.replace('// RETAIL_DEATH_LISTENERS',listeners)
    fixture=fixture.replace('// RETAIL_STARTUP_FUNCTIONS','\n'.join(startup))
    assert fixture.count('// RETAIL_AUDIO_LISTENER_SELECTION')==1
    fixture=fixture.replace('// RETAIL_AUDIO_LISTENER_SELECTION',audio_selection)
    assert fixture.count('// RETAIL_VIEWPORT_FUNCTIONS')==1
    fixture=fixture.replace('// RETAIL_VIEWPORT_FUNCTIONS',viewport_functions)
    shield=[]
    for va,(unit,digest) in SHIELD_BODIES.items():
        source=(area/f'generated/game/x86_recomp.{unit}.cpp').read_text()
        body=re.search(rf'X86_FUNC\(__imp__sub_{va:08X}\) \{{.*?^\}}',source,re.S|re.M)[0]
        assert hashlib.sha256(body.encode()).hexdigest()==digest,hex(va)
        shield.append(body.replace('__imp__sub_','sub_'))
    assert fixture.count('// RETAIL_SHIELD_FUNCTIONS')==1
    fixture=fixture.replace('// RETAIL_SHIELD_FUNCTIONS','\n'.join(shield))
    (area/'src/compat/test_coop_guest.cpp').write_text(fixture)

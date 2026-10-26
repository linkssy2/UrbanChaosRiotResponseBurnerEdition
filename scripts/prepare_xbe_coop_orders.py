"""Keep original command eligibility and callbacks, with one orders HUD per player."""
from pathlib import Path
import hashlib
import re
import struct

INCLUDE='#include "xbe_coop_orders.h"\n'
ENTRIES={
 0x40d60:'\tTrialCoopOrdersChoose(ctx,base);\n',
 0x73d80:'\tTrialCoopOrdersHudScope coop_orders_hud(ctx,base);\n',
 0x73fa0:'\tTrialCoopOrdersHudScope coop_orders_hud(ctx,base);\n',
 0x140e10:'\tTrialCoopHelpScope coop_help(ctx,base);\n',
 0x1407f0:'\tTrialCoopHelpScope coop_help(ctx,base);\n',
 0x156c50:'\tTrialCoopOrdersProperty(ctx,base);\n',
 0x158510:'\tTrialCoopOrdersScope coop_orders(ctx,base);\n',
 **{a:f'\tTrialCoopOrdersEvent(ctx,base,0x{a:x});\n'for a in (0x1583e0,0x158f30,0x159120)},
}
SEAMS={
 '00158764':(4,'\tTrialCoopOrdersControl(ctx,base);\n'),
 '001591E5':(2,'\tif(TrialCoopOrdersRegister(ctx,base))goto loc_00159219;\n'),
 '00168240':(1,'\tTrialCoopOrdersPlayer(ctx,base);\n'),
}
PINS=(
 (11,0x72a80,18,"4b549099d417481d2183797f10c87c7cd2be09f50244e6d7c99ad572918a3042","f7b24009e3f22f800b26ffc8c9eb4ba89886853008427b14aa31a1364bba4be3"),
 (11,0x73090,68,"125f63267ae2493a03f299cab4bf30dcace9a8ee17dde384ffce9d2f824ac434","e3f112083c2820417bacb3890d604ad9c70e3fb0f216183c6e30b991d67d5a42"),
 (11,0x77500,97,"ff9290c607e9031d9178b29e64c154795c707a399b15066570fec99a3e13c397","1ae4eb66ccda93619e32495e4bd397ac61b5e3f489e44c491a0ce0273b240002"),
 (11,0x70d90,455,"879111defca52dd723604dadc820a063cf5705351be989ed3f38d58867be6671","e5a02eca6f2f268fadabbb9f0adf602406e5c0aa8098298173f67c7f3f87ee76"),
 (11,0x79f50,66,"4f66e4f1a280878f797a30955ffa0a7a1ab49a468fd157f08b5addeac6155359","05c1a7da2f0b72cfbea67193c4ff7dbbe84e014b4989590424592336dafa4fdf"),
 (32,0x140e10,97,"66677344d11f638583313340d47213ca53fe9e7f1372d3e4fd38db2d723717d8","89c5eff340d4d85d4145c60d8c3c6ef4d932cb7bad199066b19435403b7eb730"),
 (32,0x140e80,45,"759538907d2954dd4acf3f9b055e0615e789897f53b9c302f544c2f78c30f81c","1fd0c576e97cc4cd485553683f1588b6c010327903225d1a1ff4bdfb9f88cb3e"),
 (32,0x1407f0,98,"e11659a554e7a2bcf9b11a97bff5aec2e5b9d96b5ebc1e9b808372ee3a5098ac","9b0928b22f9ca944377c2bab4633487f410083be19ae5174753e81a58b59844a"),
 (32,0x140680,355,"5fda68d55e49743b7ebca843eaa30970d0009f9a8d5b592a69b66335f876be29","56e77ae021f8f0ee19d0a251fa587f90e071eacc92bc98d891cd54b158e6c396"),
 (32,0x140860,48,"7cd0548200a5e7449d2d72c6f0cd8f276e5f681e98cd570d6eb5c209a7f8882c","0b3464fcb257089b211ead0e738c368bbdeca93dbd2441d6ac5169f32914342c"),
 (32,0x140fb0,486,"58c6f435b79323fd420a3a7d9ac4681ff3df20208639f26091c1d38bc748c725","418e9374956b529706ce64d76f32f2608cf578ce5ebd29631b3b859fba1672d6"),
 (32,0x1411a0,128,"2e185ec203fb9c4a86d5e3735f1e0884c62bae1e212f02a4e7695ca4ca303b28","c0974134f179235173c33350e645e5bc1400d7ab2832c9f8740ea83182b1d7ae"),
 (36,0x168210,149,"348f063eb5b927ea6ab23c872147dd0a8fc8d2ff137a9df052496ba0805220f3","50711495aed5698e7e8e06a38a9d34ed8f8d5e814a0b49e0e1ec521699a341f8"),
 (36,0x168170,146,"889c58a834dfae23039336345db635ba05bf73b60edb28974971aaf545b3db59","fd72f63a7b9b2e5d9eaf3782e3d1eba8b0af45fb656568ef14ba3f84b17b4328"),
 (41,0x19bea0,66,"497920210747f289c0c139209f991f93c476441e8171fc92d555edf73b0f30a5","8f27a5e7061515271658c098a714904303f29dff39b35bc029b2de8dfcf363f2"),
 (5,0x40d40,27,"2b95d82e21812c9150339e9661e9078ea28082f41495a528909ccddd1ea6df99","360a3de0058a8286543b54b28d7d7825d6a17e8cc5d43a04df38488d29739ba5"),
 (5,0x40d60,15,"e0be52a474a17cd0a688838498eadb258401961d95bd04c8ab0b687ef5421dad","6ff9412c87a9b92335259ddb69890fd077e1160d826fce0d5b657236607484da"),
 (5,0x40dd0,58,"a46c779690c9cd8159f30b64956df00068212812a7ef51c8e3d987aea163a8a3","b8511c3d8334f358b3b68ebe67f9b867fd89625b94e642aae31f48c0a3659f10"),
 (11,0x71900,18,"999ec7dfabc9dcd847f7eb54e6b51ae37520ac61ea42d847e27f9e03487db53b","a0d0cffde69f7bac73ccb02506215e6ec4dac6d75f5e579ee7f5464f0c002f7c"),
 (11,0x71920,18,"a5521bc55ba4b58f971aaf10867ba3278f79125c2e068fb9181953545711b316","c4b9a3217f2355d60ca3620d5b33e713a83eca92d82a26bd1568c5a3a4d7259b"),
 (11,0x73fa0,192,"18ef49eaf19ce3868c1a9ea02925a458c2d5c2231488c212944e6ed9b22695e2","07656f9328f5fe4286d81a9c2c2a198eb46854192144fa58ff0922f23a374b90"),
 (11,0x73d80,395,"2580ad629ab2db40193f06f710a4f7bbeebc1b4920efa86c20a8c88f4fdc02c3","ac12c7b6110efbbd2dc4bf18a3296c16c2590667a877f3404558b0081d8e3559"),
 (34,0x156c50,404,"fe9343ed728c2e2f4ff45935171050a500e59237911af5f21b7821ebb88448ce","7e92fbcecc0d1d5dfa273f783207cdaa1ab8a9b74c1fe959978d3110f85aaa63"),
 (34,0x156e60,738,"c528ae21fbcb1ef0c3a6bd698d458650f6b8088782773e9421d76c9598d33304","ff9cc72cf7fc7b1297a6810b22cb7c0f720b16760ba9b078dd722861e7294377"),
 (34,0x157640,40,"583b1830b59bf1ff518c38fd11adf85d810ec7e624b591c6b4d9b39c42a86be3","f893f796d1cfd1facdf068791475f9ef7d3cd8b4e55826635b65318a7f1a0fd3"),
 (34,0x157670,40,"28439e786de679b2fc429b5869d14e58dae2f6a822d3b6a77d2837a3facc7a0b","294feebb09cd9840e841279de1b12598b8f781d8f961d616379b6390971ce2c5"),
 (34,0x1583e0,51,"06f405b2eac8374d3cd3b4ebb58f27cb932c47990f2ef212eba6817745203c9f","7ac4d2dc0789038ee8f57d9689a53225fa795274f2a5c84317ea51cc26a42c1e"),
 (34,0x158510,2071,"de28ac6a357982938b7919f04d7475879fe44d4addf3ec71c8c1e9193c9a9a50","6d5354699754f774573b408f91206bb6f032852339a326c47d119bcbba9cffd2"),
 (34,0x158d90,118,"5cf970b8a8a96b1b56f4f22785827aae5df26111ca6a11a361f1d9a0603d4b6a","779c470c6c3786aab8adcc96b79022be8037c7f6ebe067ffeada8dc0159b1707"),
 (34,0x158ef0,61,"e41e4a0f4b1ee53578bb4ff8d4be65561235b5f54d52721e057d649f71eb9fef","7a01ae2fb4b7238881b41348f7295c50a86c4910e308fa714fb1126d3507091c"),
 (34,0x158f30,121,"5fa3aea143bdc9303671bfacf63f29a45d1988e6a571dd590e3b4e3885734fcd","c7124d6c9f763e3eaf6bd06d3d621fce2d346211bf1f371a73a14dfedf2128b4"),
 (34,0x159120,44,"36d084aab6367a39b7969c1f14ffa4d72ea7cddf6a3b2fb6a157bf7ce6434383","c5117e250b339a9f4c5ad0fac220169fef126d92e16a686518a1a587f4847397"),
 (34,0x159150,220,"6b9fa1ad980c8c268dfa6f49fdd507532cfab667ac6301ee1ab0555d9c1b78a3","c63c161e3e0a3b7a10a7d7e3b075e10680eaa54dbfd176b9eb0b60f4ce510574"),
)

def body(source,address):
    return re.search(rf'X86_FUNC\(__imp__sub_{address:08X}\) \{{.*?^\}}',source,re.M|re.S).group()

def clean(source):
    source=source.replace(INCLUDE,'')
    for hook in set(ENTRIES.values())|{h for _,h in SEAMS.values()}:source=source.replace(hook,'')
    source=source.replace('TrialCoopOrdersHud(base,X86_MEM32(ea))','X86_MEM32(ea)').replace('TrialCoopHelpHud(base,X86_MEM32(ea))','X86_MEM32(ea)')
    return source.replace('loc_00159219:\n','')

def unprepare(area):
    for unit in (5,11,32,34,36):
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
    assert struct.unpack('<I',raw(0x46caa8+8,4))[0]==0x156c50
    assert struct.unpack('<III',raw(0x4531a0,12))==(0x73f10,0x73fa0,0x73d80)
    # Fresh actor-owned helper ID and constructor ownership are retail facts,
    # separate from the last-created global help queue.
    assert raw(0xd48aa,43).hex()=='68c88e450051c68424ac0500000ae8f349120083c408508bcee8685913008d8c24e00200008986f0070000'
    assert raw(0x140ddc,24).hex()=='c744241c60081400897c2420e8a3feffff8b4c24205f8bc6'
    assert struct.unpack('<III',raw(0x453580,12))==(0x775a0,0xa5c00,0x77570)
    assert struct.unpack('<III',raw(0x453bd0,12))==(0x7b860,0x7b5c0,0x7b5f0)
    assert struct.unpack('<III',raw(0x453c20,12))==(0x7ba60,0x7b740,0x7b770)
    sources={u:clean((area/f'generated/game/x86_recomp.{u}.cpp').read_text())for u in (5,11,32,34,36,41)}
    originals={}
    for unit,va,size,raw_hash,lifted_hash in PINS:
        assert hashlib.sha256(raw(va,size)).hexdigest()==raw_hash,hex(va)
        text=body(sources[unit],va)
        assert hashlib.sha256(text.encode()).hexdigest()==lifted_hash,hex(va)
        originals[va]=text
    for unit in (5,11,32,34,36):
        source=sources[unit]
        for va,hook in ENTRIES.items():
            marker=f'X86_FUNC(__imp__sub_{va:08X}) {{\n'
            if marker in source:
                assert source.count(marker)==1
                source=source.replace(marker,marker+hook,1)
        if unit==11:
            for va in (0x71900,0x71920):
                original=body(source,va)
                fixed=original.replace('ctx.ecx = (uint32_t)(X86_MEM32(ea))','ctx.ecx = (uint32_t)(TrialCoopOrdersHud(base,X86_MEM32(ea)))')
                assert fixed!=original;source=source.replace(original,fixed,1)
            for va in (0x72a80,0x73090):
                original=body(source,va)
                fixed=original.replace('ctx.eax = (uint32_t)(X86_MEM32(ea))','ctx.eax = (uint32_t)(TrialCoopHelpHud(base,X86_MEM32(ea)))',1)
                assert fixed!=original;source=source.replace(original,fixed,1)
        if unit in (34,36):
            for address,(count,hook) in SEAMS.items():
                marker='\t// '+address+' '
                if (unit==36)!=(address=='00168240'):continue
                assert source.count(marker)==count,address
                source=source.replace(marker,hook+marker)
        if unit==34:
            marker='\t// 00159219 ';assert source.count(marker)==2
            source=source.replace(marker,'loc_00159219:\n'+marker)
        sources[unit]=source
        (area/f'generated/game/x86_recomp.{unit}.cpp').write_text(source.replace('#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+INCLUDE,1))
    fixtures=[]
    for unit,va,*_ in PINS:
        if va in (0x73d80,0x73fa0,0x158510,0x70d90,0x79f50,0x140680):continue
        original=originals[va];fixed=body(sources[unit],va)
        for name,text in [('Original',original),('Coop',fixed)]:
            fixtures.append(text.replace(f'__imp__sub_{va:08X}',f'{name}_{va:08X}'))
    control_start=originals[0x158510].index('\t// 00158750 ')
    control_end=originals[0x158510].index('\t// 0015876D ')
    control=originals[0x158510][control_start:control_end]
    for name,text in [('Original',control),('Coop',control.replace('\t// 00158764 ',SEAMS['00158764'][1]+'\t// 00158764 '))]:
        fixtures.append('X86_FUNC('+name+'_Control) {\nX86_LOAD_FLAGS();\nuint32_t ca=0,cb=0;\n'+text+'}\n')
    declarations=set(re.findall(r'\bsub_[0-9A-F]{8}(?=\(ctx)', '\n'.join(fixtures)))
    declarations.update(f'{name}_{va:08X}'for _,va,*_ in PINS for name in ('Original','Coop'))
    fixture=(local/'test_xbe_coop_orders.cpp').read_text()
    fixture=fixture.replace('// ORIGINAL_DECLARATIONS','\n'.join('X86_FUNC('+name+');'for name in sorted(declarations)))
    fixture=fixture.replace('// ORIGINAL_BODIES','\n'.join(fixtures))
    from prepare_xbe_coop_interactions import runtime_call_guest
    fixture=fixture.replace('// PRODUCTION_CALL_GUEST',runtime_call_guest())
    constants=(0x44b800,0x462870,0x465074,0x46a8a8,0x45819c,0x458184,0x458254,0x458238,
               0x44d94c,0x46ca34,0x465ae0,0x465b00,0x44db94,0x44bf94,0x44c724,0x44bf18,0x4581c0,0x44c178,0x46b45c,0x46b454,
               0x46b660,0x46b53c,0x46b530,0x46b50c,0x46b51c,0x46b548,0x46b554)
    strings='\n'.join(f'std::memcpy(B()+0x{a:x},"'+''.join(f'\\x{v:02x}'for v in raw(a,32))+f'",32);'for a in constants)
    strings+='\n'+'\n'.join(f'Put(0x{a:x},0x{struct.unpack("<I",raw(a,4))[0]:x});'for a in range(0x158e08,0x158e18,4))
    fixture=fixture.replace('// ORIGINAL_CONSTANTS',strings)
    (area/'src/compat/test_coop_orders.cpp').write_text(fixture)
    for name in ('xbe_coop_orders.cpp','xbe_coop_orders.h'):
        (area/'src/common'/name).write_text((local/name).read_text())

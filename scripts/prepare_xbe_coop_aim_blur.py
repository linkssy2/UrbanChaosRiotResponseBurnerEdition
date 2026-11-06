"""Keep retail zoom pulses owned by their originating split-screen player."""
from pathlib import Path
import hashlib
import struct

PINS=(
    (0x48e10,69,'2c0fd8c942f231829f22bc45235da5d42459d396a6b32fc94e72b3e5ebbd1c98'),
    (0x48e60,20,'f2b03d11da47ff2a82b5aaa52c27bf495418cd455750fa6b54a59823881af3cb'),
    (0x48e80,387,'eec9ba37667d61378e03b0e9c234f06da3195c04d9f3800191e57f962e2f89f0'),
    (0x490a0,48,'eb62542161da7cf2431813d4a2e0f23bff365277124844a4074716e72e7f567c'),
    (0x490d0,143,'3efd60a4370da1c1ea8b13356fa1baeeb93d0267409853bed50c0195fa2c1669'),
    (0x49180,46,'0a3db56114c9d7f75bce3aaa890515dcee51f0d9365c706c109724d5279cb637'),
    (0x94760,10,'f98abd9f9a37e47a60d16adc1b92b03a1ac4d8e590a5239c6bce365d385aa038'),
    (0x94770,4,'341b82a55226eef7e098acc3e6dbbc519c21405907bd11a607e28860eac7aacc'),
    (0x94780,21,'7f11180dbad3260c632cc0f8a343533aea21abe37a61e3bfd5b142511a7fea96'),
    (0x947a0,14,'f0028ebab6e79f304e9ac8a5cc9eaf54c10cd09bf3fd7191c158a18d5cb8548a'),
    (0x947b0,4,'c9e000d84a6b73b79d0c317225e4c7ca47fa7de4bc806de67930f461d77842d1'),
    (0x947c0,25,'a930f57678205517705f8fc669417e53e370c136f247ff75dd836d2ec517aa22'),
    (0x97050,177,'14b65325b2c468365ec5094ebc9fc1e1104fab422e539a40b6e8a88109f1e3f0'),
    (0x97b50,201,'6613d6e5c50fe57a891dc86e54403f6f5bc58468e31ca6f46085025453126d6a'),
    (0xbee50,291,'849366b75b9097a70968abe48c201db58b865896c977535d7e5e9902fd223e18'),
    (0x9e985,120,'c8bf0091d2fb11729529f9ab01c3193a4c3a257f0f6a7c78b701c45f899f9a5f'),
)
BODIES={
    6:{0x48e10:'5d35cc783891b51d15e382102f9e65517137b52da15fc6e0ec0d01bf17e8a0af',
       0x48e60:'80e4a4313911aa98d6fd4c1b7afd2c2a1ddab56d281c4b9880e18c213dc6aeb2',
       0x48e80:'8fbc6a8ec345a05b66be2720918bbeaa5ffa2e6934f6eab514e95e12c97051d7',
       0x490a0:'08974d36acfc34b3578390fa70ebe2b86fb94337de5c0acbc74c0c393bbff933',
       0x490d0:'4d6468f61c1c620d9967d4731bf3e2b41de96d89fb4d417d70ebf760d86b49bd',
       0x49180:'dc555cad416651f443f75f1cba3daa1b53c99ad5d364e1baceaafc52cc1583f2'},
    15:{0x94760:'56b9fb479862ff3986b1ca111702491a483d8757c93b01e601a8f0698ff1c228',
        0x94770:'c1e24369f38fbeea40e3996a7007a93695c421254a965e2d0e71b6fd657a4562',
        0x94780:'b7ac925858c059c72713f74301b0478fcf37a7171e5583970df8ce0304de3cb1',
        0x947a0:'7c9400f1242b390fa2c44803e959dab7dd829f99228ff4e410b3dde6bbe5f01b',
        0x947b0:'7754faa44a0cb014aed49c8ce09d15021d136bf95acb3c4cc3034bdd604ff80a',
        0x947c0:'9c6358b40ed42858f2a63f903ca708103be0e6ccb065cbc94e1c2f35dfeb1288',
        0x97050:'c807831b97bf7ebb01b52c0e7197e4021415cf9bca270ab7997a6075ed9842d2',
        0x97b50:'65db934ea7fddf4ee7a610e245d6cd0da74988bc1f7c61997ab6dc4bd84b41e8'},
}
INCLUDE='#include "xbe_coop_aim_blur.h"\n'
HOOKS={6:(
    ('X86_FUNC(__imp__sub_000490D0) {\n','\tTrialCoopAimBlurRequestScope trial_aim_blur_request(ctx,base);\n'),
    ('X86_FUNC(__imp__sub_00048E80) {\n','\tTrialCoopAimBlurBeginUpdate(ctx,base);\n'),
    ('\t// 00048F50 movss xmm0, dword ptr [ecx+0x08]\n','\tTrialCoopAimBlurNodeBegin(ctx,base);\n'),
    ('\t// 00048F9E mov ecx, [0x005150B0]\n','\tTrialCoopAimBlurPublish(ctx,base);\n'),
    ('\t// 00048F01 mov eax, [ecx+0x04]\n','\tTrialCoopAimBlurForgetNode(ctx);\n'),
    ('X86_FUNC(__imp__sub_00048E60) {\n','\tTrialCoopAimBlurClear(ctx);\n'),
    ('X86_FUNC(__imp__sub_000490A0) {\n','\tTrialCoopAimBlurClear(ctx);\n'),
),15:(
    ('X86_FUNC(__imp__sub_00097B50) {\n','\tTrialCoopAimBlurViewScope trial_aim_blur_view(ctx,base);\n'),
    ('\t// 00097B8B mov edx, [eax+0x08]\n','\ttrial_aim_blur_view.Apply(ctx);\n'),
)}

def clean(source,unit):
    source=source.replace(INCLUDE,'')
    for _,hook in HOOKS.get(unit,()):source=source.replace(hook,'')
    return source

def unprepare(area):
    for unit in HOOKS:
        path=Path(area)/f'generated/game/x86_recomp.{unit}.cpp'
        if path.exists():
            before=path.read_text();after=clean(before,unit)
            if after!=before:path.write_text(after)

def function(source,address):
    name=f'{address:08X}';first=source.index(f'X86_FUNC(__imp__sub_{name})')
    last=source.index(f'\nX86_WEAK_FUNC(sub_{name}',first)
    return source[first:last]

def verified_sources(area):
    area=Path(area);xbe=(area/'assets/Default.xbe').read_bytes()
    base=struct.unpack_from('<I',xbe,0x104)[0];count,table=struct.unpack_from('<II',xbe,0x11c)
    sections=[struct.unpack_from('<6I',xbe,table-base+i*56) for i in range(count)]
    for address,size,digest in PINS:
        section=next(s for s in sections if s[1]<=address and address+size<=s[1]+s[4])
        offset=section[3]+address-section[1]
        assert hashlib.sha256(xbe[offset:offset+size]).hexdigest()==digest,hex(address)
    sources={}
    for unit,bodies in BODIES.items():
        source=clean((area/f'generated/game/x86_recomp.{unit}.cpp').read_text(),unit)
        for address,digest in bodies.items():
            assert hashlib.sha256(function(source,address).encode()).hexdigest()==digest,hex(address)
        sources[unit]=source
    return sources

def patched(source,unit):
    for anchor,hook in HOOKS[unit]:
        assert source.count(anchor)==1,(unit,anchor)
        # Entry scopes follow the opening brace; instruction hooks precede the instruction.
        source=source.replace(anchor,anchor+hook if anchor.startswith('X86_FUNC') else hook+anchor,1)
    assert source.count('#include "x86_recomp_shared.h"\n')==1
    return source.replace('#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+INCLUDE,1)

def fixture(sources):
    parts=[]
    for unit,bodies in BODIES.items():
        for address in bodies:
            if address==0x490a0:continue
            body=function(patched(sources[unit],unit),address)
            parts.append(body.replace(f'__imp__sub_{address:08X}',f'sub_{address:08X}',1))
    # Original complete pulse/update bodies serve as independent native parity oracles.
    for address,name in ((0x490d0,'OriginalPulse'),(0x48e80,'OriginalUpdate')):
        parts.append(function(sources[6],address).replace(f'__imp__sub_{address:08X}',name,1))
    text=(Path(__file__).parent/'test_xbe_coop_aim_blur.cpp').read_text()
    assert text.count('// PRODUCTION_AIM_BLUR_BODIES')==1
    return text.replace('// PRODUCTION_AIM_BLUR_BODIES','\n'.join(parts))

def prepare(area):
    area=Path(area);local=Path(__file__).parent;sources=verified_sources(area)
    for unit,source in sources.items():(area/f'generated/game/x86_recomp.{unit}.cpp').write_text(patched(source,unit))
    for name in ('xbe_coop_aim_blur.h','xbe_coop_aim_blur.cpp'):
        (area/'src/common'/name).write_text((local/name).read_text())
    (area/'src/compat/test_coop_aim_blur.cpp').write_text(fixture(sources))

"""Hold-Interact ragdoll dragging through the original ray and rigid-body force."""
from pathlib import Path
import hashlib
import re
import struct

INCLUDE='#include "xbe_ragdoll_drag.h"\n'
UPDATE='\tTrialRagdollDragUpdate(ctx,base);\n'
STEP='\tTrialRagdollDragStep(ctx,base);\n'
SHARED_HOOKS=(
    '\tTrialAimUpdate(ctx,base);\n',
    '\tTrialHealthPackUpdate(ctx,base);\n',
    '\tTrialGuestHook(ctx, base, 26);\n',
    '\tTrialDeathSpasmStep(ctx,base);\n',
    '\tif(trial_ragdoll_force::Tail(base,ctx.ecx))goto loc_002A5E31;\n',
)
PINS=(
 (21,0xd2ab0,2794,'49c4b4c5fde2eb9549928b28a8fd292c09b21572fc510b2f07ac712bd408b712','141801e02625b764e87fed6ae9f21703e40a39c962e1efafa9cc3f6675fba652'),
 (71,0x2a5da0,231,'16aee95dbd1452a1ee4e3505e9357e4064e832050b5bd074142c4fa10e33892e','b5d4db7e4bf64cade8852e25f65e0d289e88a920029ae8edc0b8c10ed50e905a'),
 (82,0x2f4ff0,87,'ff86222783e610ad699f9afebe58617074437a68ce655f1ebb67d2b6a7947162','4c946fb2c96f9c9c49a38dd6dd63498b73cde45738618a848cb336473b56b326'),
 (81,0x2ecaa0,42,'f9bb2125c6c82525a32b615604a318754adc2a3034e8f3eb2b3b52a5691cf71a','1ba92af13bd1dbd800af469b371b775a9f0d5aa5868ca1253423f08e8531e0e7'),
 (7,0x567f0,1239,'1d3c96186a22976bcfbc3b0033734b07ae3a051f3827ef14232f6cdb7cac4ded','bed8e6ad254b926ad01f7b1fab6af1d54baa358722a05b54aaa8bfc859802bdc'),
 (7,0x55bf0,224,'b94a281c6a4f46bcbd0098c5d9cdac188d8e53de3ccb26c0bff33118eea65f09','b8bb2abaf5d36175fd22a304a8176848a92a159b26626c44e61618576e5ea585'),
 (7,0x55cd0,197,'72c3b9e71e47ef97ceeb803c290452b1cf0cbe12942a699e6d7dc85923c49d14','949efca5d9a13c234133ebfac19a2de0240e57d8abc17bf6ee22083615b56ffb'),
 (54,0x214230,598,'aaee7b4219bfd32935b5f7f7b4782c2c258db9707a54c6cb7b4206c283557e52','3ce47133a1c673d57d0444c5cbb5c10ec8b752bc059338bcaa176b79b01c1b4c'),
 (71,0x2a6fd0,27,'34f3b8cd67fc2e94d2e0c350923f154c60d8219f7c027d4f9c86acc0123240cc','f8115936b0b20f356c2a200d7c54a264961ea8af5b83f15f095e3036e7cb1918'),
 (71,0x2a6ff0,930,'4b33f2624d4d2cc4bb12e1e1fa66b98fb0eb3a512ae65ac15c5407d1c832f045','c792afb45afb298e251a8a0ed78ba4629ca8f1b2cae07c7006848379fbd7cbd9'),
 (71,0x2a7470,415,'5d882d1a87f2482b860bf60567d1c55918a48421b3a4f8c7967922535f20fe95','f8b721079832708fa557bcbf1231027cb49b7bdc2394b31102acb4bf9f7d534e'),
 (71,0x2a8d00,341,'ca5e5e867bf1a587fb6558a11f0c0dfea1a96893cf52ee19fdf8046edc323597','bd3d24aec05b6186d91fc6b03d899551abf6560ad2365b175685c388011fbff8'),
 (70,0x2a2870,88,'2ef33f4eff157c22c3354c9430aef1eedb28117ae99f0ccd24683b330122aa6a','ddf7af3d805db786d3bc0a49c0c4df82c3968c791835417d2f0385e0f9d5a82c'),
 (70,0x2a28d0,57,'1a204dab06fe3a8e9782754815e4b6dd3f858c79730a07135a2b104649b28d94','bf6d3746c497b7c9ed5e2cb2651e7f7f9334f786c5c929dd6c9a7e963e1bf568'),
 (71,0x2a5c60,231,'a2846ca56fc75a9aa79551791c0a125627cc087380befcfb151230a9e9dadc34','1dddc6a6d354407ab6ab6d89dbb0db867d4f8d443f959b23514ca7a0a50d5fdb'),
 (50,0x1f9010,9,'899c718a5333cfa40cdaa23a42713aa616de87fbb074f1150bb0bfaa7873d339','8f5786d86fd1a77504011d1037b9c29cd58d632d252b0c6fdc767eab134f8abb'),
 (50,0x1f9030,9,'46e55b115eddceff00acb9d62402386d99919c3de00a9747cc322de93ac71cb6','ad20c612f3a2ae5ae23f596e4b003067a3316d0397df223b6cc87b6b321468d3'),
)


def body(source,address):
    return re.search(rf'X86_FUNC\(__imp__sub_{address:08X}\) \{{.*?^\}}',source,re.M|re.S)[0]


def clean(source):
    return source.replace(INCLUDE,'').replace(UPDATE,'').replace(STEP,'')


def patch(source,unit):
    source=clean(source)
    if unit==21:
        anchor='X86_FUNC(__imp__sub_000D2AB0) {\n'
        assert source.count(anchor)==1
        result=source.replace(anchor,anchor+UPDATE,1)
    else:
        assert unit==71
        anchor='\t// 002A5E7C '
        assert source.count(anchor)==2 # Normal call and generated mid-function resume.
        result=source.replace(anchor,STEP+anchor)
    result=INCLUDE+result
    assert clean(result)==source
    return result


def unprepare(area):
    for unit in (21,71):
        path=Path(area)/f'generated/game/x86_recomp.{unit}.cpp'
        if path.exists():
            before=path.read_text();after=clean(before)
            if before!=after:path.write_text(after)


def prepare(area):
    area=Path(area);local=Path(__file__).parent
    image=(area/'assets/Default.xbe').read_bytes();base=struct.unpack_from('<I',image,0x104)[0]
    count,table=struct.unpack_from('<II',image,0x11c)
    sections=[struct.unpack_from('<6I',image,table-base+i*56)for i in range(count)]
    def raw(address,size):
        section=next(s for s in sections if s[1]<=address and address+size<=s[1]+s[4])
        offset=section[3]+address-section[1];return image[offset:offset+size]
    sources={u:clean((area/f'generated/game/x86_recomp.{u}.cpp').read_text())for u,_,_,_,_ in PINS}
    originals={}
    for unit,address,size,raw_hash,source_hash in PINS:
        assert hashlib.sha256(raw(address,size)).hexdigest()==raw_hash,hex(address)
        original=body(sources[unit],address)
        # Pin the retail body while retaining each independent installed hook.
        for hook in SHARED_HOOKS:original=original.replace(hook,'')
        assert hashlib.sha256(original.encode()).hexdigest()==source_hash,hex(address)
        originals[address]=original
    for address,size,digest in (
        (0xc7894,335,'bb452dc408e110afb2519badacd35a5c6157659e16709164d1eafae6de30e831'),
        (0xc7bf8,147,'ef9cfafde565b69a175184fadd64d758bedbec5d60e66df8cb234715052c8889'),
        (0x2a5e7c,5,'f1ab846c057c78b79284d388d77ba907b304f4d311de9bfff3b4e6199d1d286d'),
        (0x2095a8,11,'9523f5ccf9b3b32873e542f49181f51741596f2b57fd686ae6fd6123a0a16a9b'),
        (0x29e9ac,54,'ecd71ea97a5e333b4ff897fd9fc95d801144e53dfed15c7f2df8dfee221e9b9a'),
    ):
        assert hashlib.sha256(raw(address,size)).hexdigest()==digest,hex(address)
    assert struct.unpack('<f',raw(0x44d024,4))[0]==struct.unpack('<f',struct.pack('<f',.01))[0]
    assert struct.unpack('<f',raw(0x44c178,4))[0]==100.
    assert struct.unpack('<I',raw(0x48de08,4))[0]==0x2a8d00
    assert struct.unpack('<I',raw(0x49a8b8+0x6c,4))[0]==0x2f4ff0
    leaves=(0x55bf0,0x55cd0,0x567f0,0x2f4ff0,0x2ecaa0,0x2a2870)
    parts=['X86_FUNC(sub_002F6CC0);']+[f'X86_FUNC(sub_{a:08X});'for a in leaves]
    parts.extend(originals[a].replace('__imp__sub_','sub_',1)for a in leaves)
    fixture=(local/'test_xbe_ragdoll_drag.cpp').read_text();marker='// NATIVE_DRAG_CONTRACTS'
    assert fixture.count(marker)==1
    fixture=fixture.replace(marker,'\n'.join(parts))
    constants=[]
    for address,size in ((0x44c174,4),(0x44bf94,4),(0x44c538,4),(0x44c5f0,4),(0x44d024,4),(0x2095a9,4),(0x55d98,64),(0x56cc8,80)):
        data=raw(address,size)
        constants.append('{const uint8_t data[]={'+','.join(str(v)for v in data)+f'}};std::memcpy(B()+0x{address:x},data,sizeof(data));}}')
    assert fixture.count('// NATIVE_INPUT_CONSTANTS')==1
    fixture=fixture.replace('// NATIVE_INPUT_CONSTANTS','\n'.join(constants))
    files={name:(local/name).read_text()for name in ('xbe_ragdoll_drag.cpp','xbe_ragdoll_drag.h')}
    patched={unit:patch(sources[unit],unit)for unit in (21,71)}
    for unit,source in patched.items():(area/f'generated/game/x86_recomp.{unit}.cpp').write_text(source)
    for name,source in files.items():(area/'src/common'/name).write_text(source)
    (area/'src/compat/test_ragdoll_drag.cpp').write_text(fixture)

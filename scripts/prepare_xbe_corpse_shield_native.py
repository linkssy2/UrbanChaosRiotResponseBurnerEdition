"""Native corpse shield backend and original hard-keyframe contract fixture."""
from pathlib import Path
import hashlib
import re
import struct

PINS = (
    (70, 0x29eba0, 146, '75cd9ea213ea1435c5730c7397383af1c0e810050160200e2bc3a95b681b225c', 'ac9a2735230f4e57881a5c9489c172aef3a89d3a4bd738d8ab6a50c654cce061'),
    (78, 0x2d5640, 443, '5ad7e937c871de34d5a50acf6bb0067a1640848caad174c07a30dce2da1eac99', '91d50ffe9a52c613228c29b054781f2e71a621762fbfa9a8cfa63e9740889f37'),
    (79, 0x2e26a0, 123, '5d020a1067e4b75833a5c1ae5eb9e4e3122b7feef67e2997102b8224627b9913', 'bfd78b9372567d00c11bd63580ecbef6afa1718cf537319c5285d5dd9e0f8d18'),
    (80, 0x2e83f0, 163, '0279b51474e70b7fdccaa524e711437ceddbbde146595533bab827e88f4e0030', 'bd5399994d8d99b5666be4325af7177be3d9d7a26006a1cfaccd6c66b3dddd8f'),
    (80, 0x2e84a0, 509, '4f31a00f457a416ab028e5c5f2fe414532bb9712a7c297f003df4d93853fe537', 'bab746ed180607b120e5e80fcdc8f0db98b7de0a875fd9a62f3ea3d6b74136e0'),
    (81, 0x2ecaa0, 42, 'f9bb2125c6c82525a32b615604a318754adc2a3034e8f3eb2b3b52a5691cf71a', '1ba92af13bd1dbd800af469b371b775a9f0d5aa5868ca1253423f08e8531e0e7'),
    (81, 0x2ec5c0, 17, '18370885a0705c00cffe8558fc81c16a2785d85aa3733ae18074980b6c4ae444', 'ff5779f5be5ef2edff76d8b37c2806fdbb9f883a018172e98ea00f3129e28cbb'),
    (81, 0x2ec5e0, 17, 'abbdb83e53b83065f1b4087c83d72247cb3c889c84ce1a209d3f532f10b1b9f0', '77e699aefa416d181f82c70d0a6d345e4d2238075167ab0c41528638e3c4a675'),
)

CONTRACT_PINS = (
    (70, 0x29e4f0, 9, 'dfac3415469bff06689de04f897569580e4ca2714c7c275461dc8a86b595707d', 'd9fe2808ff3b74dc402abb4ae0f589c755347e50001689c11a673c73a3261afa'),
    (71, 0x2a6130, 1096, '4e26b29712731938f60fd57cda7ad394c6124b57d80c84136b9a1a64052815d3', '1db36547d72232258a6122db40d87bb085ce46578a5e19640ec84f202c1fac97'),
    (71, 0x2a5f00, 546, '1f8d22ba5a873e38d8c5c269e87b4f28724b0755f3089b3e82578f664cd5cfeb', 'dbdab212c9879c01898963ed9f6418faaccd85deb4041c1c1c62d8ef2da52d64'),
    (71, 0x2a5da0, 231, '16aee95dbd1452a1ee4e3505e9357e4064e832050b5bd074142c4fa10e33892e', 'b5d4db7e4bf64cade8852e25f65e0d289e88a920029ae8edc0b8c10ed50e905a'),
    (71, 0x2a51c0, 992, '65576416942271ee16fbd27d9bbafe136f1707783eab7e9417cb437b8adb4a3e', 'd428a42c7b2d2855aa9a5bc42b53050047453989e3fa38caff8bcaff1565a237'),
    (71, 0x2a55a0, 1721, '4322f3154c48ad52b032488bb05137bfd97ad2024814b4935bbad10124e1c14b', '1fd9c7ffffc59f615ad8680645f84df931e8b461d688c466c7b469243b94879f'),
    (71, 0x2a6920, 30, 'd27a56b614ce2a06a39990d1745e2b7d77c27d0f6d50f91af72e687baea68f79', '4a07a2e72f6d411b956bac2f364436ee13ac3fcb732efa49279a90473f2ecfa3'),
    (71, 0x2a6590, 511, '55f525f647985170407f4fdb805772194fb534cac377f074f5873761ef82f759', 'efcb9e19f3d010bc40320482a2eb058c36bfb0e4efb0d9c495d5157c3e6bf2ea'),
    (71, 0x2a3120, 38, 'e620588ea421bb8819eec221f91f42e535f7d904849404438f68f841baf0f294', '80046e797ad1b664b5b3266e0eb81d8f78a34f7e860c9dd69cdddd1a82b35d1d'),
    (81, 0x2f1880, 100, 'bd0f6d16a892a637eb2f281417a7c6aa1adbd99eaaf38de215bee06fec88339e', '5c2902162aaa53d2f9b18662e43088033e27074472029523848f0777b265f04d'),
)
CONTRACT_HOOKS = (
    '\tTrialDeathSpasmActivated(ctx,base);\n',
    '\tTrialDeathSpasmStep(ctx,base);\n',
    '\tTrialRagdollDragStep(ctx,base);\n',
    '\tif(trial_ragdoll_force::Tail(base,ctx.ecx))goto loc_002A5E31;\n',
)

def body(source,address):
    return re.search(rf'X86_FUNC\(__imp__sub_{address:08X}\) \{{.*?^\}}',source,re.M|re.S)[0]

def prepare(area):
    area=Path(area);local=Path(__file__).parent
    image=(area/'assets/Default.xbe').read_bytes();base=struct.unpack_from('<I',image,0x104)[0]
    count,table=struct.unpack_from('<II',image,0x11c)
    sections=[struct.unpack_from('<6I',image,table-base+i*56)for i in range(count)]
    def raw(address,size):
        section=next(s for s in sections if s[1]<=address and address+size<=s[1]+s[4])
        offset=section[3]+address-section[1];return image[offset:offset+size]
    for unit,address,size,digest,source_hash in CONTRACT_PINS:
        assert hashlib.sha256(raw(address,size)).hexdigest()==digest,hex(address)
        original=body((area/f'generated/game/x86_recomp.{unit}.cpp').read_text(),address)
        for hook in CONTRACT_HOOKS:original=original.replace(hook,'')
        assert hashlib.sha256(original.encode()).hexdigest()==source_hash,hex(address)
    sources={u:(area/f'generated/game/x86_recomp.{u}.cpp').read_text()for u,*_ in PINS}
    native={}
    for unit,address,size,digest,source_hash in PINS:
        assert hashlib.sha256(raw(address,size)).hexdigest()==digest,hex(address)
        original=body(sources[unit],address)
        assert hashlib.sha256(original.encode()).hexdigest()==source_hash,hex(address)
        native[address]=original.replace('__imp__sub_','sub_',1)
    # Retail source confirms both vector units and the selected motion methods.
    assert struct.unpack('<I',raw(0x49ae58+0x54,4))[0]==0x2ec5c0
    assert struct.unpack('<I',raw(0x49ae58+0x58,4))[0]==0x2ec5e0
    assert struct.unpack('<f',raw(0x44d024,4))[0]==struct.unpack('<f',struct.pack('<f',.01))[0]
    fixture=(local/'test_xbe_corpse_shield_native.cpp').read_text()
    marker='// NATIVE_SHIELD_KEYFRAME';assert fixture.count(marker)==1
    fixture=fixture.replace(marker,'\n'.join([f'X86_FUNC(sub_{a:08X});'for a in native]+list(native.values())))
    constants=[]
    for address,size in ((0x44bf94,4),(0x44c174,4),(0x44c1f8,4),(0x44c770,4),(0x46c31c,4),(0x498ae0,4),(0x49b790,16)):
        data=raw(address,size)
        constants.append('{const uint8_t data[]={'+','.join(str(v)for v in data)+f'}};std::memcpy(B()+0x{address:x},data,sizeof(data));}}')
    marker='// NATIVE_SHIELD_CONSTANTS';assert fixture.count(marker)==1
    fixture=fixture.replace(marker,'\n'.join(constants))
    for name in ('xbe_corpse_shield_physics.cpp','xbe_corpse_shield_physics.h'):
        (area/'src/common'/name).write_text((local/name).read_text())
    (area/'src/compat/test_corpse_shield_native.cpp').write_text(fixture)

if __name__=='__main__':
    prepare(Path(__file__).resolve().parents[1]/'build/xbe-full-integration-20261014')

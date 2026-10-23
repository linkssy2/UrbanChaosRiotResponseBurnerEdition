"""Reverse only the retail firearm scan reached by a PC Previous Firearm press."""
from pathlib import Path
import hashlib
import re
import struct

ROOT=Path(__file__).resolve().parents[1]
PINS={
0x42690:(5,82,'dca66fe19bb00417756718573f61c07851abd25ad751555bb51881368c603d10','24065883d937d3bc6fb5b819c3cb9f46d2795ded9c9c89b69d489e8f9025b966'),
0x426F0:(5,334,'2ac4aaa4cac63489612c9ecc6104de0fc2016af5bd5b06ab02364c005244b007','8f22764c41285b685a2a630e1c394ccf93d7fe60913d90aa6d3398dd35f05db0'),
0x42840:(5,36,'4d8e249c24c79c35d3f999f15195e35693dc824f6df0ed361fe9ceb4bd2e7200','b347a020a4df715134a752da67adb97c9598fa0ec185775143379508e5019b75'),
0x42870:(5,177,'35ccc57e834cf16add2f68f469c8cafc7170c81aaa007002b0224c44c17d3dba','354afb473c7347e89861004b64c2ab90d701818ce26e4a8a94fa3aa822158eb3'),
0x42230:(5,64,'105d89800795dd7d936e57671b988dfd83823b5b107422877eefac25fcf876c3','34b336144e2278c37f8639d46e67ff54413f4b7712893d4aa584206e76493bdf'),
0x41F50:(5,36,'cd528813330e132d8e50a8be20e189735977eae9ad9f8985f9552d71b5a9422b','1eb588ed31250cc6e1e9fe3ef621b3908c85b33f2541e911bc9883898b35f657'),
0x41F80:(5,19,'6df5e43397e00268bc7243feada5ec38e2afe2e3695d1d413b5429a5f7c0a713','724a56ee582cbb2e42eade112ad91541c56b6829750ce81017aa6f5700bbf2f3'),
0x422C0:(5,19,'f186593f015dc62920635cc286f66110c93edbc86999b55d7d7b94e2f979633b','8f3719539e8447f7858bca09123e87ac7e76025861b4e33c7af9e87ddce3ea16'),
0x42DA0:(5,42,'3d5c6b114e83c4958db8fc03742d986fe9b13ad734e38ccc06aeeb1c6a882553','b66dd0eda7342696c23646a5ba7c3d797406048b574e501f87ebd158880bf8c8'),
0x43C00:(5,116,'c30943a51c6d43b4aeeafdc571f6c51b38118e157b47c4e051453810c55b6f72','4a9fbca06aa4621a31f73d77961e485958500120ff20f169390f9ed4e533ad25'),
0xC4CB0:(20,567,'ad402843a2b8205b631e285b3f49f162a1acc49867c030925049535690df6e7f','4ee498a998074badc1e39a0b96fc59463bc511946ee97a4f731467e4fbb21b6c'),
}
INCLUDE='#include "xbe_firearm_cycle.h"\n'
SCOPE='\ttrial_firearm_cycle::Scope firearm_scope(ctx,base);\n'
STEP='\ttrial_firearm_cycle::BeforeIncrement(ctx,base);\n'

def prepare(area):
    area=Path(area)
    paths={unit:area/f'generated/game/x86_recomp.{unit}.cpp' for unit,_,_,_ in PINS.values()}
    original={unit:path.read_text().replace(INCLUDE,'').replace(SCOPE,'').replace(STEP,'') for unit,path in paths.items()}
    bodies={}
    image=(area/'assets/Default.xbe').read_bytes()
    base=struct.unpack_from('<I',image,0x104)[0]
    count,table=struct.unpack_from('<II',image,0x11c)
    sections=[struct.unpack_from('<6I',image,table-base+i*56) for i in range(count)]
    for address,(unit,size,raw_hash,body_hash) in PINS.items():
        section=next(s for s in sections if s[1]<=address and address+size<=s[1]+s[4])
        offset=section[3]+address-section[1]
        assert hashlib.sha256(image[offset:offset+size]).hexdigest()==raw_hash,hex(address)
        body=re.search(rf'X86_FUNC\(__imp__sub_{address:08X}\) \{{.*?^\}}',original[unit],re.S|re.M)[0]
        assert hashlib.sha256(body.encode()).hexdigest()==body_hash,hex(address)
        bodies[address]=body
    bodies[0x42840]=bodies[0x42840].replace('{\n','{\n'+SCOPE,1)
    marker='\t// 00042787 inc ebp\n'
    assert bodies[0x426F0].count(marker)==1
    bodies[0x426F0]=bodies[0x426F0].replace(marker,STEP+marker)
    staged=original[5]
    for address in (0x42840,0x426F0):
        old=re.search(rf'X86_FUNC\(__imp__sub_{address:08X}\) \{{.*?^\}}',staged,re.S|re.M)[0]
        staged=staged.replace(old,bodies[address],1)
    staged=staged.replace('#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+INCLUDE,1)
    common=area/'src/common';compat=area/'src/compat'
    common.mkdir(parents=True,exist_ok=True);compat.mkdir(parents=True,exist_ok=True)
    for name in ('xbe_firearm_cycle.h','xbe_input_memory.h'):
        (common/name).write_text((ROOT/'scripts'/name).read_text())
    declarations='\n'.join(f'X86_EXTERN_FUNC(sub_{a:08X});' for a in bodies)
    extracted='\n\n'.join(body.replace('X86_FUNC(__imp__sub_','X86_FUNC(sub_',1) for body in bodies.values())
    test=(ROOT/'scripts/test_xbe_firearm_cycle.cpp').read_text()
    assert test.count('// RETAIL_FUNCTIONS')==1
    (compat/'test_firearm_cycle.cpp').write_text(test.replace('// RETAIL_FUNCTIONS',declarations+'\n'+extracted))
    if paths[5].read_text()!=staged:paths[5].write_text(staged)

if __name__=='__main__':prepare(ROOT/'build/xbe-full-integration-20261014')

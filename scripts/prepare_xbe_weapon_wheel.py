"""Bounded actor-local weapon wheel; preserve retail pending-selection/transition code."""
from pathlib import Path
import hashlib
import re
import struct
import prepare_xbe_firearm_cycle as firearm
from prepare_xbe_coop_interactions import runtime_call_guest

ROOT=Path(__file__).resolve().parents[1]
PINS={0xBC560:(19,0x15,'b3457d8f03ba3f694ed11d6f6c3f805b56586a319855f800b919c2918eeb8b79',
              '1500c9db3f31a4f4f13fb56b24d315bd51f4b3ef45c58d3605ade11c73a43386')}
INCLUDE='#include "xbe_weapon_wheel.h"\n'
HOOKS={
    19:[('\t// 000BFD86 movss xmm0, dword ptr [esp+0x3C]',
         '\ttrial_weapon_wheel::GuestLook(ctx,base);\n')],
    20:[('loc_000C4D4B:\n',
         '\tif(trial_weapon_wheel::GuestSelect(ctx,base))goto loc_000C4E03;\n')],
    21:[('X86_FUNC(__imp__sub_000D2AB0) {\n',
         '\ttrial_weapon_wheel::GuestUpdate(ctx,base);\n')],
}

def clean(source,unit):
    source=source.replace(INCLUDE,'')
    for anchor,hook in HOOKS.get(unit,[]):
        source=source.replace(hook,'')
    return source

def unprepare(area):
    for unit in HOOKS:
        path=Path(area)/f'generated/game/x86_recomp.{unit}.cpp'
        if path.exists():
            old=path.read_text();new=clean(old,unit)
            if old!=new:path.write_text(new)

def function(source,address):
    return re.search(rf'X86_FUNC\(__imp__sub_{address:08X}\) \{{.*?^\}}',source,re.M|re.S)[0]

def prepare(area,fixture_output=None):
    area=Path(area);local=ROOT/'scripts'
    image=(area/'assets/Default.xbe').read_bytes()
    base=struct.unpack_from('<I',image,0x104)[0];count,table=struct.unpack_from('<II',image,0x11c)
    sections=[struct.unpack_from('<6I',image,table-base+i*56) for i in range(count)]
    def pin(address,size,digest):
        sec=next(s for s in sections if s[1]<=address and address+size<=s[1]+s[4])
        at=sec[3]+address-sec[1]
        assert hashlib.sha256(image[at:at+size]).hexdigest()==digest,hex(address)
    pin(0xd2ab0,15,'2c0a32f728d14663320296e782d1791e322bcbdd020db1223b276cf68f2e5c4d')
    pin(0xbfd5d,0x8b,'35876d969f2f82dcb45641b54fa9af0fe831f6eee0746f1a8db85c8d2724f486')
    originals={}
    for address in (0x42690,0x426f0,0x42840,0x42870,0x42230,0x41f50,0x41f80,0x422c0,0x42da0,0x43c00,0xc4cb0,0xbc560):
        unit,size,raw,digest=PINS[address] if address in PINS else firearm.PINS[address];pin(address,size,raw)
        source=clean((area/f'generated/game/x86_recomp.{unit}.cpp').read_text(),unit)
        for hook in (firearm.INCLUDE,firearm.SCOPE,firearm.STEP,firearm.SELECT,firearm.HUD):source=source.replace(hook,'')
        body=function(source,address)
        assert hashlib.sha256(body.encode()).hexdigest()==digest,hex(address)
        originals[address]=body
    patched={}
    for unit,hooks in HOOKS.items():
        path=area/f'generated/game/x86_recomp.{unit}.cpp';source=clean(path.read_text(),unit)
        for anchor,hook in hooks:
            assert source.count(anchor)==1,(unit,anchor)
            source=source.replace(anchor,hook+anchor if unit==19 else anchor+hook,1)
        patched[unit]=INCLUDE+source
    parts=[]
    for address,body in originals.items():
        parts.append(body.replace(f'__imp__sub_{address:08X}',f'sub_{address:08X}',1))
    parts.append(function(patched[20],0xc4cb0).replace('__imp__sub_000C4CB0','WheelConsumer',1))
    declarations='\n'.join(f'X86_EXTERN_FUNC(sub_{address:08X});' for address in originals)
    fixture=(local/'test_xbe_weapon_wheel.cpp').read_text()
    assert fixture.count('// NATIVE_WEAPON_CONTRACTS')==1
    assert fixture.count('// PRODUCTION_CALL_GUEST')==1
    fixture=fixture.replace('// NATIVE_WEAPON_CONTRACTS',declarations+'\n'+'\n'.join(parts))
    fixture=fixture.replace('// PRODUCTION_CALL_GUEST',runtime_call_guest())
    if fixture_output is not None:
        output=Path(fixture_output);output.parent.mkdir(parents=True,exist_ok=True);output.write_text(fixture)
        return
    for name in ('xbe_weapon_wheel.h','xbe_weapon_wheel.cpp'):(area/'src/common'/name).write_text((local/name).read_text())
    (area/'src/compat/test_weapon_wheel.cpp').write_text(fixture)
    for unit,source in patched.items():(area/f'generated/game/x86_recomp.{unit}.cpp').write_text(source)

if __name__=='__main__':prepare(ROOT/'build/xbe-full-integration-20261014')

"""Scale only the qualified solo campaign world-frame gameplay argument."""
from pathlib import Path
import hashlib
import re
import struct

INCLUDE='#include "xbe_weapon_wheel_time.h"\n'
HOOK='\tTrialWeaponWheelTime wheel_time(ctx,base);\n'
PINS=(
    (0x55610,346,'b4327fea04a0838b7911e4025ba6094af7a7a35d0d0de5c35c344ae909ff4602'),
    (0x5d1ff,21,'0863dc9e1165ac39f7989ceadaffa3cf070ee052b422ebba1dd798ae539a3dd7'),
    (0x208f80,911,'5d74f89e9de9bc3bb3d935054ee02599f9282fb6309313642d4b57df38875fc7'),
    (0x450c7c,4,'3f376b6cca506ecab808dc170a74d86705b38528ce1acc2e7661425640ff9966'),
    (0x276e0,3,'01cb47d078b4841b8408ec4fe278efa83115c6f6e101972987507d2b2b57dcf0'),
    (0x4505a8,4,'f3e124c48e112b7f61ec45297a771e4d1bad551c154d09df50b3fb7119f4011c'),
    (0x1f9010,41,'8e6c0f9ad90635fce0407c7a5c2f1db05be16d47dcb776d2d315fa62e21cc202'),
)

def clean(source):return source.replace(INCLUDE,'').replace(HOOK,'')

def unprepare(area):
    path=Path(area)/'generated/game/x86_recomp.7.cpp'
    if path.exists():
        old=path.read_text();new=clean(old)
        if old!=new:path.write_text(new)

def body(source,address):
    return re.search(rf'X86_FUNC\(__imp__sub_{address:08X}\) \{{.*?^\}}',source,re.M|re.S)[0]

def prepare(area):
    area=Path(area);local=Path(__file__).parent
    image=(area/'assets/Default.xbe').read_bytes()
    base=struct.unpack_from('<I',image,0x104)[0];count,table=struct.unpack_from('<II',image,0x11c)
    sections=[struct.unpack_from('<6I',image,table-base+i*56) for i in range(count)]
    for address,size,digest in PINS:
        section=next(s for s in sections if s[1]<=address and address+size<=s[1]+s[4])
        at=section[3]+address-section[1]
        assert hashlib.sha256(image[at:at+size]).hexdigest()==digest,hex(address)
    path=area/'generated/game/x86_recomp.7.cpp';source=clean(path.read_text())
    original=body(source,0x55610)
    assert hashlib.sha256(original.encode()).hexdigest()=='47a87783da57b97d06a71c00faca0f8ea0a66a18f5eae297da35cbc030fb9010'
    world=body((area/'generated/game/x86_recomp.52.cpp').read_text(),0x208f80)
    # These independent co-op observers are not part of the retail fixture.
    for hook in ('\tTrialCoopBodiesRetire(ctx,base);\n','\tTrialCoopDeathUpdate(ctx,base);\n'):world=world.replace(hook,'')
    assert hashlib.sha256(world.encode()).hexdigest()=='4de70826c50b1a8ca9541bd2d64751006abf9da19db358b220972f956a56ba10'
    patched=original.replace('{\n','{\n'+HOOK,1)
    source=source.replace(original,patched,1)
    source=source.replace('#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+INCLUDE,1)
    assert source.count(INCLUDE)==source.count(HOOK)==1
    declarations='\n'.join(f'X86_EXTERN_FUNC(sub_{name});' for name in sorted(set(re.findall(r'sub_([0-9A-F]{8})\(ctx, base\)',original+world))))
    fixture=(local/'test_xbe_weapon_wheel_time.cpp').read_text()
    marker='// ORIGINAL_WORLD_FRAME';assert fixture.count(marker)==1
    fixture=fixture.replace(marker,declarations+'\n'+world.replace('__imp__sub_00208F80','sub_00208F80',1)+'\n'+
        original.replace('__imp__sub_00055610','OriginalWorldFrame',1)+'\n'+patched.replace('__imp__sub_00055610','WheelWorldFrame',1))
    for name in ('xbe_weapon_wheel_time.h','xbe_weapon_wheel_time.cpp'):(area/'src/common'/name).write_text((local/name).read_text())
    (area/'src/compat/test_weapon_wheel_time.cpp').write_text(fixture)
    path.write_text(source)

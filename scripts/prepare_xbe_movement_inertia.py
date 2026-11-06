"""Optional immediate walking input at the qualified original BEF80 output seam."""
from pathlib import Path
import hashlib
import re
import struct
from prepare_xbe_coop_controls import body, clean as clean_controls, WALK

INCLUDE='#include "xbe_movement_inertia.h"\n'
ANCHOR='\t// 000BF2C8 jz 0x000BF2FA'
HOOK='\tTrialMovementInertia(ctx,base);\n'
PINS=(
    (0xbef80,3695,'f0d83761e926db7a90d629244d6b952e7480bcddd7617538f72b8c6e2f5e59e0'),
    (0xbc4b0,165,'1c3247fbdd7b939b55a7922bfa3d73f4aad5070a92a3e3463e32a1afbe1b1c2f'),
    (0x44350,120,'088771c9c8744b9037d025ed3006265d4564cba416f6a858f19d7ed8c842412b'),
    (0x1f9010,9,'899c718a5333cfa40cdaa23a42713aa616de87fbb074f1150bb0bfaa7873d339'),
    (0xbc930,336,'c12f83624ec599b9598f9ec21135518709ed7240f1aac1eb5df31685408d1e5e'),
)

def clean(source):
    return source.replace(INCLUDE,'').replace(HOOK,'')

def unprepare(area):
    path=Path(area)/'generated/game/x86_recomp.19.cpp'
    if path.exists():
        before=path.read_text();after=clean(before)
        if after!=before:path.write_text(after)

def prepare(area):
    area=Path(area);local=Path(__file__).parent
    image=(area/'assets/Default.xbe').read_bytes();base=struct.unpack_from('<I',image,0x104)[0]
    count,table=struct.unpack_from('<II',image,0x11c)
    sections=[struct.unpack_from('<6I',image,table-base+i*56)for i in range(count)]
    def raw(va,size):
        section=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4])
        offset=section[3]+va-section[1];return image[offset:offset+size]
    for va,size,digest in PINS:
        assert hashlib.sha256(raw(va,size)).hexdigest()==digest,hex(va)
    path=area/'generated/game/x86_recomp.19.cpp';source=clean(path.read_text())
    original=body(source,0xbef80)
    fragments=[]
    for start,end,digest in (
        (0xbf088,0xbf136,'d5a9036b71ba3e594d48ecb0f2c58a3b0c93bddcc09db4c996c339c83aafb8a7'),
        (0xbf292,0xbf3b4,'32160d9d57f6b558daf55a9d727464032c2d88d95d54b875dad115de598bb389')):
        fragment=original[original.index(f'\t// {start:08X} '):original.index(f'\t// {end:08X} ')]
        assert hashlib.sha256(fragment.encode()).hexdigest()==digest,hex(start)
        fragments.append(fragment)
    assert source.count(ANCHOR)==1
    patched=source.replace(ANCHOR,HOOK+ANCHOR,1)
    pieces=[]
    for unit,va,digest in (
        (50,0x1f9010,'8f5786d86fd1a77504011d1037b9c29cd58d632d252b0c6fdc767eab134f8abb'),
        (5,0x44350,'97dbe618b01fafa5efc10bd4928c2cfa4a2f1116b1d972d59d0bed58e660c1e0'),
        (19,0xbc930,'39004c30d333f216c1e6baae46f33db2e0e4e5558e1b8296d81310072b90f443')):
        leaf=body((area/f'generated/game/x86_recomp.{unit}.cpp').read_text(),va)
        assert hashlib.sha256(leaf.encode()).hexdigest()==digest,hex(va)
        pieces.append(leaf.replace('__imp__sub_','sub_',1))
    movement=fragments[0]+'return; // Alternate movement leaves this qualified excerpt.\nloc_000BF292:\n'+fragments[1]+';\n'
    for name,text in (('OriginalMovement',movement),('ImmediateMovement',movement.replace(ANCHOR,HOOK+ANCHOR,1))):
        pieces.append('X86_FUNC('+name+') {\nX86_LOAD_FLAGS();uint32_t ca=0,cb=0;\n'+text+'}\n')
    # Execute the actual clamping/output producer plus the installed Walk hook;
    # physical action sampling is the fixture's sole input boundary.
    axes=body(clean_controls(source),0xbc4b0)
    assert hashlib.sha256(axes.encode()).hexdigest()=='f5503c9c916852a178ce5e4d0425694c0598ecd00ebd0333ee78e450ddbbb11a'
    for va in (0xbc52d,0xbc549):
        anchor=f'\t// {va:08X} ';assert axes.count(anchor)==1
        axes=axes.replace(anchor,WALK+anchor,1)
    pieces.append(axes.replace('__imp__sub_000BC4B0','ResolvedMovement',1))
    constants=sorted({int(a,16)for a in re.findall(r'ea = \(uint32_t\)\(0x([0-9A-F]+)u\)','\n'.join(pieces))}-{0x51c4dc})
    fixture=(local/'test_xbe_movement_inertia.cpp').read_text()
    for marker,value in (
        ('// ORIGINAL_MOVEMENT_BODIES','\n'.join(pieces)),
        ('// ORIGINAL_CONSTANTS','\n'.join(f'Put(0x{a:x},0x{struct.unpack("<I",raw(a,4))[0]:x});'for a in constants))):
        assert fixture.count(marker)==1;fixture=fixture.replace(marker,value)
    path.write_text(INCLUDE+patched)
    (area/'src/common/xbe_movement_inertia.h').write_text((local/'xbe_movement_inertia.h').read_text())
    (area/'src/compat/test_movement_inertia.cpp').write_text(fixture)

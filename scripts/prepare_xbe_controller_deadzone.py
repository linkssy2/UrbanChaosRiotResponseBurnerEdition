"""Replace the inner deadzone at the qualified native controller descriptor seam."""
from pathlib import Path
import hashlib
import re
import struct
from prepare_xbe_coop_controls import body
from prepare_xbe_graphics_input import function

INCLUDE='#include "xbe_controller_deadzone.h"\n'
BEGIN='\ttrial_controls::BeginControllerSample(4);\n'
REGISTER='\ttrial_controls::RegisterControllerAxes(ctx.ebx+0x14);\n'
FILTER='\tif(TrialControllerDeadzone(ctx,base))goto loc_0022EE42;\n'
PINS=(
    (0x22dfa0,631,'2a78b9fbfb5c6f78f9fc070bfed6f9006099f007d7a685eb709fa030a87ca9ca'),
    (0x22edf0,292,'473da8cb5a9d81c54730c621c95e52bc94b20e8e926f935fec97ed5403d5d665'),
    (0x232d80,11,'4f998e151e87a7f304596586bb9d91f02f077efe4b53b23da51322eb27071338'),
    (0x22fb00,169,'4f36257e56ac7843f9b9442d1e37b3bc15546517724ac5dfc0b8a53eb69bbf5d'),
    (0x22f790,802,'68700c634606a2a1e277222c6cd16e4b63b79445f21b2bb4ee6e40d03be7753c'),
    (0x4fd08c,8,'95a957a1d3fa17a27f0682ccfed7c345988c75172fa427917b2a96b5008f7e41'),
    (0x4fcd70,64,'5ca9e99a0ae3db9d70caecea74dd7534d75aa6d27796f279c4d4dc0c9154e3f1'),
    (0x4fceb0,64,'180941be1558d74985419655da4d740bfd03c74d183912260494a08af4cf9bee'),
)

def clean(source):
    for hook in (INCLUDE,BEGIN,REGISTER,FILTER):source=source.replace(hook,'')
    return source

def unprepare(area):
    path=Path(area)/'generated/game/x86_recomp.57.cpp'
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
    path=area/'generated/game/x86_recomp.57.cpp';source=clean(path.read_text())
    pieces=[]
    for va,name,digest in (
        (0x22dfa0,'NativePad','bcb734349494ea3c976b9f7ab4499a0b11646402b3b1b2ba480bcd2b865f3847'),
        (0x232d80,'NativeAxisAddress','2957cffc7120fb7765a75f842289c1c58fafe628703ca6e69f18235f4077bad1'),
        (0x22edf0,'OriginalDescriptor','1d3c5501a0b47f2f9bcc6d6e6e52e1697492bb59db8d675f827231f95daa9e30')):
        original=body(source,va)
        assert hashlib.sha256(original.encode()).hexdigest()==digest,hex(va)
        patched=original
        for address,hook in ((0x22dfb3,BEGIN),(0x22dfb8,REGISTER),(0x22ee10,FILTER)):
            anchor=f'\t// {address:08X} '
            if anchor in patched:
                assert patched.count(anchor)==1
                patched=patched.replace(anchor,hook+anchor,1)
        source=source.replace(original,patched,1)
        pieces.append((original if va==0x22edf0 else patched).replace(f'__imp__sub_{va:08X}',name,1))
        if va==0x22edf0:pieces.append(patched.replace('__imp__sub_0022EDF0','ConfiguredDescriptor',1))
    constants=sorted({int(a,16)for a in re.findall(r'ea = \(uint32_t\)\(0x([0-9A-F]+)u\)','\n'.join(pieces))}|{0x4fd08c,0x4fd090})
    fixture=(local/'test_xbe_controller_deadzone.cpp').read_text()
    bindings=(local/'xbe_controller_bindings.inc').read_text()
    profile=bindings[bindings.index('struct UCRRControllerConfig {'):bindings.index('\nstatic UCRRControllerConfig')]
    helpers=profile+'\n'+function(bindings,'controller_strength','static int ')+'\n'+function(bindings,'controller_axis_response','static bool ')
    for marker,value in (
        ('// CONTROLLER_BINDING_HELPERS',helpers),
        ('// ORIGINAL_CONTROLLER_BODIES','\n'.join(pieces)),
        ('// ORIGINAL_CONSTANTS','\n'.join(f'Put(0x{a:x},0x{struct.unpack("<I",raw(a,4))[0]:x});'for a in constants))):
        assert fixture.count(marker)==1;fixture=fixture.replace(marker,value)
    path.write_text(INCLUDE+source)
    (area/'src/common/xbe_controller_deadzone.h').write_text((local/'xbe_controller_deadzone.h').read_text())
    (area/'src/compat/test_controller_deadzone.cpp').write_text(fixture)

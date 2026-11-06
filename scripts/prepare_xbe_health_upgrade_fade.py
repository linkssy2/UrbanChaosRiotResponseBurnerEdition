"""Make the original health-upgrade dividers inherit their HUD descriptor alpha."""
from pathlib import Path
import hashlib
import struct

INCLUDE='#include "xbe_health_upgrade_fade.h"\n'
ORIGINAL='\t// 00073495 mov dword ptr [esp+0x20], 0x55000000\n\t{ uint32_t ea = (uint32_t)(ctx.esp + 0x20u); X86_MEM32(ea) = (uint32_t)(0x55000000u); }'
FIXED=ORIGINAL.replace('(uint32_t)(0x55000000u)','TrialHealthUpgradeColor(ctx,base)')
PINS=(
    (11,0x733e0,246,'9f384ec95ffa9b0edced770387904fb061bb1d960a2c57e527acc144c71750da','c3d70809e7de6e515e4c8a119fbbe9eb49f2f067cdfde526978bcd7f6a5e3085'),
    (11,0x7346d,105,'c50b4a5cfef5bd8e3de279b4d8230ff6396cd8fe267dbc81b58ecbc152363eca','4be49e337f0934c3560551eee3ece521e5f3e16b74a5104d22cf6d895331ddb7'),
    (9,0x65760,84,'68042d1be5556e9c1b26da234aec5823cc026e44ecfdcf5150c02cc53f42ee12','2ef6643ec469af76a924cce4859b5c1d20c726dd881d1ae9e2db0eaf4391a9ec'),
    (9,0x65550,137,'aab96479d66b4c8839b15e7563beadd6ed280c69ce36ebfd5c59f276fda68c5f','9ddfa68ac26dc59024045c0334473751fcb986a8fca1879664254a620bfc3c1f'),
    (9,0x65170,152,'065c75529af885b1496619ccb3d328b405fb562dc7bf321db649260df5a2f553','ed939a512aedd44490abfc51912b7cabb30a035d717279e83be705c80185e9de'),
)

def body(source,va):
    first=source.index(f'X86_FUNC(__imp__sub_{va:08X}) {{')
    return source[first:source.index('\nX86_WEAK_FUNC',first)]

def prepare(area):
    area=Path(area);local=Path(__file__).parent
    x=(area/'assets/Default.xbe').read_bytes();base=struct.unpack_from('<I',x,0x104)[0]
    count,table=struct.unpack_from('<II',x,0x11c)
    sections=[struct.unpack_from('<6I',x,table-base+i*56) for i in range(count)]
    def raw(va,size):
        section=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4])
        offset=section[3]+va-section[1];return x[offset:offset+size]
    sources={u:(area/f'generated/game/x86_recomp.{u}.cpp').read_text().replace(INCLUDE,'').replace(FIXED,ORIGINAL) for u in (9,11)}
    sources[9]=sources[9].replace('\tif (TrialFrontendHideListSprites()) { ctx.esp += 4; return; }\n','')
    pieces=[]
    for unit,va,size,raw_hash,source_hash in PINS:
        assert hashlib.sha256(raw(va,size)).hexdigest()==raw_hash,hex(va)
        original=body(sources[unit],va)
        assert hashlib.sha256(original.encode()).hexdigest()==source_hash,hex(va)
        if unit==11:
            assert original.count(ORIGINAL)==1
            fixed=original.replace(ORIGINAL,FIXED)
            pieces.extend((original.replace('__imp__sub_', 'Original_',1),fixed.replace('__imp__sub_', 'Fixed_',1)))
        else:pieces.append(original.replace('__imp__sub_','sub_',1))
    assert raw(0x4530ec,4)==struct.pack('<I',0x733e0)
    assert raw(0x73495,8)==bytes.fromhex('c744242000000055')
    assert sources[11].count(ORIGINAL)==2
    source=INCLUDE+sources[11].replace(ORIGINAL,FIXED)
    assert source.count(FIXED)==2 and ORIGINAL not in source
    (area/'generated/game/x86_recomp.11.cpp').write_text(source)
    (area/'src/common/xbe_health_upgrade_fade.h').write_text((local/'xbe_health_upgrade_fade.h').read_text())
    constants='\n'.join(f'W(0x{a:x},0x{struct.unpack("<I",raw(a,4))[0]:x});' for a in
        (0x44bf94,0x44c1f8,0x4e8558,0x4e855c,0x4e8534,0x4e8538,0x46c150,0x44c530,0x46c31c,0x44c5e4))
    fixture=(local/'test_xbe_health_upgrade_fade.cpp').read_text()
    for marker,value in (('// ORIGINAL_MARKER_BODIES','\n'.join(pieces)),('// ORIGINAL_CONSTANTS',constants)):
        assert fixture.count(marker)==1;fixture=fixture.replace(marker,value)
    (area/'src/compat/test_health_upgrade_fade.cpp').write_text(fixture)

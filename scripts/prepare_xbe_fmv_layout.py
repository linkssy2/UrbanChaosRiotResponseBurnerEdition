"""Aspect-preserving Fill/4:3 on the original movie quad, before queued submission."""
from pathlib import Path
import hashlib
import re
import struct


def body(source, address):
    start=source.index(f'X86_FUNC(__imp__sub_{address:08X}) {{')
    return source[start:source.index(f'\nX86_WEAK_FUNC(sub_{address:08X}',start)]


def prepare(area):
    area=Path(area);local=Path(__file__).parent
    xbe=(area/'assets/Default.xbe').read_bytes()
    base=struct.unpack_from('<I',xbe,0x104)[0];count,table=struct.unpack_from('<II',xbe,0x11c)
    sections=[struct.unpack_from('<6I',xbe,table-base+i*56) for i in range(count)]
    def read(va,size):
        section=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4])
        at=section[3]+va-section[1];return xbe[at:at+size]
    for va,size,digest in [
        (0x87810,299,'96a9651d21437c5e643d3f4d3bbeaa4a63f573914f3bc46a5f22bd0907a6728e'),
        (0x87960,505,'417fec040fac47ed61abc6da538232954dda2138790cd68e55911fa8409621c0'),
        (0x877b0,21,'49ca23edf0855657bed8cb9e48d5d81175cc98f2b92a7805cadd02f2f6ae1165'),
    ]:
        assert hashlib.sha256(read(va,size)).hexdigest()==digest,hex(va)
    path=area/'generated/game/x86_recomp.13.cpp';source=path.read_text()
    source=re.sub(r'^.*// uc_fmv_layout\n','',source,flags=re.M)
    original=source
    marker='#include "x86_recomp_shared.h"\n'
    assert source.count(marker)==1
    source=source.replace(marker,marker+'#include "../../src/common/xbe_fmv_layout.h" // uc_fmv_layout\n',1)
    for address,hook in [
        ('00087960','TrialFmvOpen(ctx,base);'),('000877B0','TrialFmvClose();'),
        ('00087810','const auto fmv_layout=TrialFmvLayout(base);'),
        ('00087938','TrialFmvBorders(ctx,base,fmv_layout);'),
        *[(address,'TrialFmvVertex(ctx,base,fmv_layout);') for address in ('000878C2','000878DF','00087902','00087927')],
    ]:
        marker='\t// '+address+' '
        assert source.count(marker)==1,address
        source=source.replace(marker,'\t'+hook+' // uc_fmv_layout\n'+marker,1)
    path.write_text(source)
    for name in ('xbe_fmv_layout.h','xbe_fmv_layout.cpp'):
        (area/'src/common'/name).write_text((local/name).read_text())
    fixture=(local/'test_xbe_fmv_layout.cpp').read_text()
    for marker,text in [('// GENERATED_DRAW',body(source,0x87810)),
                        ('// GENERATED_ORIGINAL_DRAW',body(original,0x87810).replace('__imp__sub_00087810','OriginalMovieDraw',1))]:
        assert fixture.count(marker)==1
        fixture=fixture.replace(marker,text)
    (area/'src/compat/test_fmv_layout.cpp').write_text(fixture)

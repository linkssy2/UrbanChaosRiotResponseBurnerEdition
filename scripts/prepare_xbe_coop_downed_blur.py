"""Queue downed-view scene blur at the original world-to-HUD boundary."""
from pathlib import Path
import hashlib
import struct

PINS=(
    # Campaign world's adjacent scene/HUD virtual slots and the HUD caller.
    (0x450c48,28,'6091f150b0233281e88f0d4f024e8e52d253b31c105231900f19bee079d483ab'),
    (0x557e0,32,'70bbc9097df232a85e24cbe7622486f4179172766afcd6cd6e82591f1516b93f'),
    (0x215357,14,'bf778555c5cace2e40789e30c1e9d2420c33dd169c859355ab086fa86514cc4e'),
    (0x215380,214,'12f02f0a14aa4baefa093cca221de9fe72d3bb71879ae04aab5e167b0c1f6a4c'),
)
INCLUDE='#include "xbe_coop_downed_blur.h"\n'
HOOK='\tTrialCoopDownedSceneBlur(ctx,base);\n'


def unprepare(area):
    """Remove only this hook before preparers which pin the original HUD body."""
    path=Path(area)/'generated/game/x86_recomp.54.cpp'
    if not path.exists():return
    old=path.read_text();source=old.replace(INCLUDE,'').replace(HOOK,'')
    if source!=old:path.write_text(source)


def prepare(area):
    area=Path(area);local=Path(__file__).parent
    xbe=(area/'assets/Default.xbe').read_bytes()
    base=struct.unpack_from('<I',xbe,0x104)[0]
    count,table=struct.unpack_from('<II',xbe,0x11c)
    sections=[struct.unpack_from('<6I',xbe,table-base+i*56) for i in range(count)]
    for va,size,digest in PINS:
        section=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4])
        offset=section[3]+va-section[1]
        assert hashlib.sha256(xbe[offset:offset+size]).hexdigest()==digest,hex(va)
    path=area/'generated/game/x86_recomp.54.cpp'
    source=path.read_text().replace(INCLUDE,'').replace(HOOK,'')
    witnesses=[]
    for unit,name,first,last,digest in (
        (54,'OriginalSceneFinish',0x215357,0x215365,'be653b25716cf12bbfdcbcb4df9ee987843b17b91b427fdf6fb6c0ce642d95ce'),
        (7,'OriginalHudBegin',0x557e0,0x557e8,'fef56b67bd8e7ef1facf7d5f17c54f73ad9dd88ea6136e334385a675c2b9452d'),
    ):
        original=source if unit==54 else (area/f'generated/game/x86_recomp.{unit}.cpp').read_text()
        start=original.index(f'\t// {first:08X} ');stop=original.index(f'\t// {last:08X} ',start)
        part=original[start:stop]
        assert hashlib.sha256(part.encode()).hexdigest()==digest,name
        witnesses.append(f'void {name}(X86Context& ctx,uint8_t* base) {{\n'+part+'}\n')
    anchor='X86_FUNC(__imp__sub_00215380) {\n'
    assert source.count(anchor)==1
    # Entry precedes cinematic HUD ownership and every native HUD submission.
    source=source.replace(anchor,anchor+HOOK,1)
    source=source.replace('#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+INCLUDE,1)
    assert source.count(HOOK)==1 and source.count(INCLUDE)==1
    for name in ('xbe_coop_downed_blur.cpp','xbe_coop_downed_blur.h'):
        (area/'src/common'/name).write_text((local/name).read_text())
    test=(local/'test_xbe_coop_downed_blur.cpp').read_text()
    marker='// ORIGINAL_DOWNED_BLUR_BOUNDARIES';assert test.count(marker)==1
    (area/'src/compat/test_coop_downed_blur.cpp').write_text(test.replace(marker,'\n'.join(witnesses)))
    path.write_text(source)

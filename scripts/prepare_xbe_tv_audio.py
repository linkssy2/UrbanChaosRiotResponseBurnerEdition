"""Apply the user-accepted 300 ms TV cue correction after the original bank parser."""
from pathlib import Path
import hashlib
import struct

INCLUDE = '#include "kernel/memory.h"\n#include "xbe_input_memory.h"\n#include "xbe_tv_audio.h"\n'
HOOK = '\tTrialTvAudio(base,ctx.edi,TrialGuestReadable);\n'
ANCHOR = '\t// 00250451 cmp [ebx], ebp\n'


def unprepare(area):
    p = Path(area)/'generated/game/x86_recomp.61.cpp'
    old = p.read_text()
    new = old.replace(INCLUDE, '').replace(HOOK, '')
    if old != new:
        p.write_text(new)


def prepare(area):
    area = Path(area)
    original = (Path(__file__).resolve().parents[1]/'game_files/Default.xbe').read_bytes()
    assert hashlib.sha256(original).hexdigest() == 'b74ae76fdbf1ef5f95dcfabf4f94f3aa738e649ecaef4068d8531e07071b5dac'
    image = (area/'assets/Default.xbe').read_bytes()
    base = struct.unpack_from('<I', original, 0x104)[0]
    count, table = struct.unpack_from('<II', original, 0x11c)
    sections = [struct.unpack_from('<6I', original, table-base+i*56) for i in range(count)]
    for address, size in ((0x24fe60,0xa68),(0x24ad10,29),(0x24bd20,61)):
        section = next(s for s in sections if s[1]<=address and address+size<=s[1]+s[4])
        offset = section[3]+address-section[1]
        assert image[offset:offset+size] == original[offset:offset+size], hex(address)
    unprepare(area)
    p = area/'generated/game/x86_recomp.61.cpp'
    source = p.read_text()
    assert source.count(ANCHOR) == 1
    assert source.count('X86_PUSH32(0x250451u); sub_0024F560(ctx, base);') == 1
    p.write_text(INCLUDE + source.replace(ANCHOR, HOOK + ANCHOR))
    for name in ('xbe_tv_audio.h', 'test_xbe_tv_audio.cpp'):
        (area/'src/common'/name).write_text((Path(__file__).parent/name).read_text())

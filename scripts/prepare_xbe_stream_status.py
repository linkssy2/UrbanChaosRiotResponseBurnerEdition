"""Restore the retail stream playback status used by dialogue animation."""
from pathlib import Path
import hashlib
import struct


def once(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new, 1)


def between(text, begin, end):
    start = text.index(begin)
    return text[start:text.index(end, start)]


def prepare(source, area):
    """Patch only the private dsound source; stage a source-derived fixture."""
    area = Path(area)
    xbe = (area / 'assets/Default.xbe').read_bytes()
    base = struct.unpack_from('<I', xbe, 0x104)[0]
    count, table = struct.unpack_from('<II', xbe, 0x11c)
    sections = [struct.unpack_from('<6I', xbe, table-base+i*56) for i in range(count)]

    def image(va, size):
        section = next(s for s in sections if s[1] <= va and va+size <= s[1]+s[4])
        offset = section[3]+va-section[1]
        return xbe[offset:offset+size]

    pins = {
        (0x251e10, 0x31): '41e4becfbf250d8a1caa605355ec8a06bf16178f26776e1126e943d8f17defcc',
        (0x484570, 4): 'b5b5524369f6a98612a73756885ee9642761d98f35a715f659da14c510ac141c',
        (0x4293b8, 0x5a): '6393fd9ded917fe0f5f62520615b363100ebba406f3ccb93e34ed3394f327093',
    }
    for (va, size), digest in pins.items():
        assert hashlib.sha256(image(va, size)).hexdigest() == digest, hex(va)

    source = once(source, '#include "xdk/hle.h"',
                  '#include "xdk/hle.h"\n#include "xbe_stream_status.h"')
    source = once(source,
                  '    W32(a[1], v.packets.size() < v.max_packets ? 1u : 0u);  // XMEDIASTATUS_ACCEPT_INPUT',
                  '    W32(a[1], uc_stream::HostStatus(v));')
    local = Path(__file__).parent
    (area / 'src/compat/xbe_stream_status.h').write_text((local / 'xbe_stream_status.h').read_text())

    original = (area / 'generated/game/x86_recomp.124.cpp').read_text()
    original = between(original, 'X86_FUNC(__imp__sub_004293B8) {',
                       '\nX86_WEAK_FUNC(sub_004293B8')
    title = (area / 'generated/game/x86_recomp.61.cpp').read_text()
    title = between(title, 'X86_FUNC(__imp__sub_00251E10) {',
                    '\nX86_WEAK_FUNC(sub_00251E10')
    fixture = (local / 'test_xbe_stream_status.cpp').read_text()
    for marker, body in [
        ('// GENERATED_ORIGINAL', original), ('// GENERATED_TITLE', title),
        ('// GENERATED_RENDER', between(source, 'void Deactivate(Voice& v)', '\nint64_t NowMs()')),
        ('// GENERATED_FLUSH', between(source, 'static void FlushStream(Voice& v)', '\nSTREAM(AddRef')),
        ('// GENERATED_STREAM', between(source, 'STREAM(GetStatus, 2)', '\nSTREAM(SetVolume')),
    ]:
        fixture = once(fixture, marker, body)
    (area / 'src/compat/test_stream_status.cpp').write_text(fixture)
    return source

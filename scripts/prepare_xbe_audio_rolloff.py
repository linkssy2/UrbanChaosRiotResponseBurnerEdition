"""Route the title's stream rolloff interface through the existing host curve owner."""
from pathlib import Path
import struct


ADAPTER = r'''
#define SYM_UrbanChaos_StreamRolloffCurve 004261E7
HLE_STDCALL(UrbanChaos_StreamRolloffCurve, 4) {
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    Voice* v = Find(a[0]);
    if (!v || !v->stream) return 0x80070057u;
    uint32_t bytes = std::min<uint32_t>(a[2], 64) * 4;
    if (a[1] && bytes && !TrialGuestReadable(a[1], bytes)) return 0x80070057u;
    // shortcut: inherit the buffer's 64-point/immediate curve; extend both paths for full deferred parity.
    return buf_SetRolloffCurve(*v, a);
}

'''


def once(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new, 1)


def prepare(source, test_path):
    root = Path(__file__).resolve().parents[1]
    xbe = (root / 'build/xbe-full-integration-20261014/assets/Default.xbe').read_bytes()
    u32 = lambda offset: struct.unpack_from('<I', xbe, offset)[0]
    table = u32(0x120) - u32(0x104)
    # The public entry jumps to the four-stack-argument stream method.
    pins = {0x4261E7: 'e9ddfcffff', 0x425F20: 'c21000'}
    found = set()
    for index in range(u32(0x11C)):
        offset = table + index * 56
        va, size, raw = (u32(offset + k) for k in (4, 8, 12))
        for address, expected in pins.items():
            if va <= address < va + size:
                at = raw + address - va
                assert xbe[at:at + len(expected) // 2].hex() == expected, hex(address)
                found.add(address)
    assert found == pins.keys()

    begin = source.index('BUFFER(SetRolloffCurve, 4) {')
    end = source.index('\nBUFFER(SetOutputBuffer, 2)', begin)
    curve = source[begin:end]
    helper = once(curve, 'BUFFER(SetRolloffCurve, 4)',
                  'static uint32_t buf_SetRolloffCurve(Voice& v, const Args& a)')
    source = once(source, '#include "xdk/hle.h"',
                  '#include "xdk/hle.h"\n#include "scripts/xbe_input_memory.h"')
    source = once(source, curve, curve + ADAPTER)
    fixture = Path(__file__).with_name('test_xbe_audio_rolloff.cpp').read_text()
    fixture = once(fixture, '// GENERATED_CURVE', helper)
    fixture = once(fixture, '// GENERATED_ADAPTER', ADAPTER)
    Path(test_path).write_text(fixture)
    return source

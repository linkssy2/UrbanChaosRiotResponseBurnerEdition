"""Move the exact injured Daniels spawn clear of its overlapping collision walls."""
from pathlib import Path
import hashlib
import re
import struct

INCLUDE = '#include "xbe_daniels_spawn.h"\n'
ENTRY = '\ttrial_daniels_spawn::Correct(base,ctx.ecx);\n'
PINS = (
    (22, 0xde380, 218, 'e1ce96048d0e1f510d0bbcf37054b9dfd038877f9702c5b624ca3720c30d41b7',
     '80678e5a79f02c58fbfe3b1c43ea74bc3be8718c9cc5b2752b8edf39fbfce9bf'),
)

def body(source, address):
    return re.search(rf'X86_FUNC\(__imp__sub_{address:08X}\) \{{.*?^\}}', source, re.M | re.S).group()

def clean(source):
    return source.replace(INCLUDE, '').replace(ENTRY, '')

def unprepare(area):
    path = Path(area) / 'generated/game/x86_recomp.22.cpp'
    if path.exists():
        source = path.read_text()
        if clean(source) != source:
            path.write_text(clean(source))

def prepare(area):
    area = Path(area)
    local = Path(__file__).parent
    xbe = (area / 'assets/Default.xbe').read_bytes()
    base = struct.unpack_from('<I', xbe, 0x104)[0]
    count, table = struct.unpack_from('<II', xbe, 0x11c)
    sections = [struct.unpack_from('<6I', xbe, table - base + i * 56) for i in range(count)]
    def raw(address, size):
        section = next(s for s in sections if s[1] <= address and address + size <= s[1] + s[4])
        offset = section[3] + address - section[1]
        return xbe[offset:offset + size]
    path = area / 'generated/game/x86_recomp.22.cpp'
    source = clean(path.read_text())
    for _, address, size, original_hash, generated_hash in PINS:
        assert hashlib.sha256(raw(address, size)).hexdigest() == original_hash
        original = body(source, address)
        assert hashlib.sha256(original.encode()).hexdigest() == generated_hash
    assert struct.unpack('<I', raw(0x47bf50 + 0x2c, 4))[0] == 0x214e70
    assert raw(0x214e70, 4).hex() == '8d4110c3'  # Actor's transform, passed to 2B15D0.
    assert hashlib.sha256(raw(0x2b167c, 55)).hexdigest() == '7f66e3223ca25877f08a3132e0f7edb686df0cac6773db71fc135a9cebd9f36d'
    assert raw(0x2ba4a2, 5).hex() == '683333b33e'  # Capsule radius .35m.
    assert raw(0x44fd08, 4).hex() == '6666e63e' and raw(0x48f574, 4).hex() == '9999d93f'
    anchor = 'X86_FUNC(__imp__sub_000DE380) {\n'
    assert source.count(anchor) == 1
    source = source.replace(anchor, anchor + ENTRY, 1)
    path.write_text(INCLUDE + source)
    (area / 'src/common/xbe_daniels_spawn.h').write_text((local / 'xbe_daniels_spawn.h').read_text())
    parts = [original.replace('__imp__sub_000DE380', 'OriginalSpawn', 1),
             body(source, 0xde380).replace('__imp__sub_000DE380', 'FixedSpawn', 1)]
    fixture = (local / 'test_xbe_daniels_spawn.cpp').read_text().replace('// NATIVE_BODIES', '\n'.join(parts))
    (area / 'src/compat/test_daniels_spawn.cpp').write_text(fixture)

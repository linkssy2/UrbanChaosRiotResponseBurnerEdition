"""Let either local actor enter the original evidence-pickup commit once."""
from pathlib import Path
import hashlib
import re
import struct

INCLUDE = '#include "xbe_coop_collectibles.h"\n'
ENTRY = '\tTrialCoopEvidenceScope coop_evidence(ctx,base);\n'
SELECT = '\tcoop_evidence.Select(ctx,base);\n'
POPUP = '\tTrialCoopEvidencePopup(ctx,base);\n'
HOOKS = {
    37: [('X86_FUNC(__imp__sub_00171D80) {\n', ENTRY),
         ('\t// 00171E10 add esp, 0x04', SELECT)],
    11: [('\t// 00073066 test ecx, ecx', POPUP)],
}
BODIES = (
    (37, 0x171d80, 471, '4e02e3ffebdc0fd54847287384475da37ccaa4fdd757d4c3c85cb4066e914171', 'dba894d84e11fae1099ecbfac82e4a105f0643f73dbc83583a47d1a8278b7a19'),
    (37, 0x171c70, 89, 'e2c0e4d3d384121ba5c09d49aefb8db1b37f16e6fda736f8e12795e3d664d97b', 'b1b1282c05f035e892eb3549c3c3646aec8a00780f6a5a5b4602c0be04d8c4ca'),
    (6, 0x4a840, 6, 'e51598aae247076e5665f4a60dfcf3e4ed9a470afbcf020165528094ba6f6c58', '77e02a84134be3bac2bfcea70562957f6be493f348198ff2ef8a30e72d891f4b'),
    (6, 0x4acd0, 9, '3d88229ba8f81117aa293b673da37176d12ed5fdadb9eeb56bfda6da97e01c38', '5e658c29e77f331c045498dba19928e7629042aec91a0cc55482940bbc4e1a05'),
    (11, 0x73060, 41, '9dbcda37bba9ebadf6a238e3bddec335b78a5be94c96261a39a0a21b2ea38789', '087445535e851af02b0452af77100d84a5a0180accb5cd2bd474bd4f3567d44c'),
)

def clean(source, unit):
    source = source.replace(INCLUDE, '')
    for _, hook in HOOKS.get(unit, []):
        source = source.replace(hook, '')
    return source

def unprepare(area):
    for unit in HOOKS:
        path = Path(area) / f'generated/game/x86_recomp.{unit}.cpp'
        if path.exists():
            before = path.read_text(); after = clean(before, unit)
            if before != after: path.write_text(after)

def body(source, address):
    return re.search(rf'X86_FUNC\(__imp__sub_{address:08X}\) \{{.*?^\}}', source, re.M | re.S).group()

def prepare(area):
    area = Path(area); local = Path(__file__).parent
    data = (area / 'assets/Default.xbe').read_bytes()
    base = struct.unpack_from('<I', data, 0x104)[0]
    count, table = struct.unpack_from('<II', data, 0x11c)
    sections = [struct.unpack_from('<6I', data, table-base+i*56) for i in range(count)]
    def raw(va, size):
        s = next(s for s in sections if s[1] <= va and va+size <= s[1]+s[4])
        offset = s[3]+va-s[1]
        return data[offset:offset+size]
    # Exact class/factory registration, update dispatch, original geometry and
    # localized evidence label; this is not the weapon/ammo pickup family.
    assert raw(0x4f93b0, 8) == struct.pack('<II', 0x45a84c, 0x171f60)
    assert raw(0x45a84c, 11) == b'MaskPickup\0'
    assert raw(0x46f9a8+0x30, 4) == struct.pack('<I', 0x171d80)
    assert raw(0x46fa70, 11) == b'HUD-Mask-1\0'
    assert raw(0x44c178, 4) == struct.pack('<f', 100)
    assert raw(0x44d528, 4) == struct.pack('<f', 22500)
    sources = {u: clean((area / f'generated/game/x86_recomp.{u}.cpp').read_text(), u)
               for u in set(HOOKS) | {b[0] for b in BODIES}}
    # Pin and extract the original contracts independently of other maintained
    # publishers and the medic-scoped player getter. Runtime sources stay intact.
    from prepare_xbe_coop_rewards import canonical_source
    from prepare_xbe_health_packs import clean as clean_health_packs
    original = {}
    for unit, va, size, raw_hash, lifted_hash in BODIES:
        assert hashlib.sha256(raw(va, size)).hexdigest() == raw_hash, hex(va)
        text = body(clean_health_packs(canonical_source(sources[unit], unit), unit), va)
        assert hashlib.sha256(text.encode()).hexdigest() == lifted_hash, hex(va)
        original[va] = text
    test = (local / 'test_xbe_coop_collectibles.cpp').read_text()
    for va in (0x4a840, 0x4acd0, 0x171c70):
        marker = f'// ORIGINAL_{va:08X}'
        assert test.count(marker) == 1
        test = test.replace(marker, original[va].replace('__imp__sub_', 'sub_'))
    pickup = original[0x171d80]
    updated = pickup.replace('X86_FUNC(__imp__sub_00171D80) {\n', 'X86_FUNC(__imp__sub_00171D80) {\n'+ENTRY, 1)
    updated = updated.replace(HOOKS[37][1][0], SELECT+HOOKS[37][1][0], 1)
    popup = original[0x73060].replace(HOOKS[11][0][0], POPUP+HOOKS[11][0][0], 1)
    test = test.replace('// ORIGINAL_POPUP', popup.replace('__imp__sub_', 'sub_'))
    test = test.replace('// ORIGINAL_PICKUP', pickup.replace('__imp__sub_00171D80', 'OriginalPickup') + '\n' +
                        updated.replace('__imp__sub_00171D80', 'CoopPickup'))
    for unit, seams in HOOKS.items():
        source = sources[unit]
        for anchor, hook in seams:
            assert source.count(anchor) == 1, anchor
            source = source.replace(anchor, anchor+hook if anchor.startswith('X86_FUNC') else hook+anchor, 1)
        source = source.replace('#include "x86_recomp_shared.h"\n', '#include "x86_recomp_shared.h"\n'+INCLUDE, 1)
        (area / f'generated/game/x86_recomp.{unit}.cpp').write_text(source)
    for name in ('xbe_coop_collectibles.h', 'xbe_coop_collectibles.cpp'):
        (area / 'src/common' / name).write_text((local / name).read_text())
    (area / 'src/compat/test_coop_collectibles.cpp').write_text(test)

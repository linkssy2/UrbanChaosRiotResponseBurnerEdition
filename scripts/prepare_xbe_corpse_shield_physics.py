"""Hold and publish the native shield ragdoll once around the shared world step."""
from pathlib import Path
import hashlib
import re
import struct

INCLUDE = '#include "xbe_corpse_shield.h" // shared physics lifecycle\n'
HOOKS = {
    70: (
        ('\t// 0029E5EA ', '\t{float dt; std::memcpy(&dt,&ctx.ebp,4); TrialCorpseShieldBeforePhysics(ctx,base,dt);}\n'),
        ('\t// 0029E5F3 ', '\t{float dt; std::memcpy(&dt,&ctx.ebp,4); TrialCorpseShieldAfterPhysics(ctx,base,dt);}\n'),
        ('X86_FUNC(__imp__sub_0029E930) {\n', '\tTrialCorpseShieldRelease(ctx,base);\n'),
    ),
    52: (('X86_FUNC(__imp__sub_00208D90) {\n', '\tTrialCorpseShieldRelease(ctx,base);\n'),),
}
PINS = (
    (70, 0x29e5b0, 144, '04f60fdf38a01d80ae022997479510defb91d94c3e2cfa7c792a308443b8854b', 'b94a0d43ee8b934adf94a9fd8d67255e5137180e38c99214d0048aa6d1f94890'),
    (70, 0x29e930, 546, 'faa1be7471f501725818bac94348f1b0a9ea2c65a551561f14c8a1c55904aea5', '9f9d624a11ce51fca208ec4c9769873f45c5505f8d845c5c5f4da6e975d9f86a'),
    (52, 0x208d90, 494, 'b7d064b3118b2c010a3b79823da6171c3bc510f1ad9f650db15ad097f42244fe', 'eb3c50ccf679c6e52fd193b566162dd4036dc785844a65446fbd88a8b06ebb50'),
)


def body(source, address):
    return re.search(rf'X86_FUNC\(__imp__sub_{address:08X}\) \{{.*?^\}}', source, re.M | re.S)[0]


def clean(source):
    source = source.replace(INCLUDE, '')
    for hooks in HOOKS.values():
        for _, hook in hooks:
            source = source.replace(hook, '')
    return source


def patch(source, unit):
    original = clean(source)
    result = original
    for anchor, hook in HOOKS[unit]:
        assert result.count(anchor) == 1, anchor
        result = result.replace(anchor, anchor + hook if anchor.startswith('X86_FUNC') else hook + anchor, 1)
    result = INCLUDE + result
    assert clean(result) == original
    return result


def unprepare(area):
    for unit in HOOKS:
        path = Path(area) / f'generated/game/x86_recomp.{unit}.cpp'
        if path.exists():
            before = path.read_text()
            after = clean(before)
            if before != after:
                path.write_text(after)


def prepare(area):
    area = Path(area)
    local = Path(__file__).parent
    image = (area / 'assets/Default.xbe').read_bytes()
    base = struct.unpack_from('<I', image, 0x104)[0]
    count, table = struct.unpack_from('<II', image, 0x11c)
    sections = [struct.unpack_from('<6I', image, table - base + i * 56) for i in range(count)]
    sources = {u: clean((area / f'generated/game/x86_recomp.{u}.cpp').read_text()) for u in HOOKS}
    for unit, address, size, raw_hash, source_hash in PINS:
        section = next(s for s in sections if s[1] <= address and address + size <= s[1] + s[4])
        start = section[3] + address - section[1]
        assert hashlib.sha256(image[start:start + size]).hexdigest() == raw_hash, hex(address)
        assert hashlib.sha256(body(sources[unit], address).encode()).hexdigest() == source_hash, hex(address)
    patched = {u: patch(sources[u], u) for u in HOOKS}
    fixture = (local / 'test_xbe_corpse_shield_step.cpp').read_text()
    marker = '// NATIVE_SHIELD_WORLD_STEP'
    assert fixture.count(marker) == 1
    witness = '\n'.join(body(source, 0x29e5b0).replace('__imp__sub_0029E5B0', name, 1)
                        for source, name in ((sources[70], 'OriginalStep'), (patched[70], 'ShieldStep')))
    (area / 'src/compat/test_corpse_shield_step.cpp').write_text(fixture.replace(marker, witness))
    for unit, source in patched.items():
        (area / f'generated/game/x86_recomp.{unit}.cpp').write_text(source)


if __name__ == '__main__':
    prepare(Path(__file__).resolve().parents[1] / 'build/xbe-full-integration-20261014')

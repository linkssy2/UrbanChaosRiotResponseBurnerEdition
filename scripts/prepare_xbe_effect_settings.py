"""Runtime lighting quality at the original, receiver-only scene boundary."""
from pathlib import Path
import hashlib
import struct


def between(text, begin, end):
    start = text.index(begin)
    return text[start:text.index(end, start)]


def prepare(area):
    """After input preparation; stage a small host source and the source-backed test."""
    area = Path(area)
    local = Path(__file__).parent
    xbe = (area/'assets/Default.xbe').read_bytes()
    base, = struct.unpack_from('<I', xbe, 0x104)
    count, table = struct.unpack_from('<II', xbe, 0x11c)
    sections = [struct.unpack_from('<6I', xbe, table-base+i*56) for i in range(count)]

    def image(va, size):
        s = next(s for s in sections if s[1] <= va and va+size <= s[1]+s[4])
        return xbe[s[3]+va-s[1]:s[3]+va-s[1]+size]

    for va, size, digest in [
        (0x98dc0, 1144, '083f6bd516ac896bbfad38f1c8a93d90866d43ab31bcdb1703fd8b2ecb0ca8c3'),
        (0x99240, 1608, 'f5f9caf0bfb4d8b45ebb9ade3a701050586d8a8e79d2e00cc6bbd987259a157d'),
        (0xb5d80, 80, '21e72f9af5196e3d9f872684e94212c93e561230946aafd1ccc4e6a4be62b4b5'),
        (0xb5dd0, 32, '174db867f8a98510d88dfe9a08eda1f0346a7a0daa9fc9583a0515e12039e892'),
    ]:
        assert hashlib.sha256(image(va, size)).hexdigest() == digest, hex(va)
    assert image(0x9ea85, 5) == bytes.fromhex('e8b6a7ffff')
    assert image(0x9ed70, 5) == bytes.fromhex('e8cba4ffff')

    for name in ('xbe_effect_settings.h', 'xbe_effect_settings.cpp'):
        (area/'src/common'/name).write_text((local/name).read_text())
    path = area/'generated/game/x86_recomp.15.cpp'
    text = path.read_text()
    include = '#include "xbe_effect_settings.h"\n'
    scope = '\tTrialLightingScope trial_lighting_scope(ctx, base);\n'
    gate = '\tif (TrialSkipLightingReceiver(ctx, base)) goto loc_00099128;\n'
    # Rebuilds may retain this chunk; remove only our own additions.
    clean = text.replace(include, '').replace(scope, '').replace(gate, '')
    scheduler = 'X86_FUNC(__imp__sub_00099240) {\n'
    receiver = '\t// 0009911C mov ecx, [ebp+0x08]'
    assert clean.count(scheduler) == clean.count(receiver) == 1
    changed = clean.replace(scheduler, scheduler+scope, 1).replace(receiver, gate+receiver, 1)
    anchor = '#include "x86_recomp_shared.h"\n'
    assert changed.count(anchor) == 1
    changed = changed.replace(anchor, anchor+include, 1)
    # No caster, light Begin/End, source-state, base-material, or cleanup changes.
    assert changed.replace(include, '').replace(scope, '').replace(gate, '') == clean
    path.write_text(changed)

    block = between(changed, gate+receiver, 'loc_00099128:')+'loc_00099128:\n'
    models = (area/'generated/game/x86_recomp.18.cpp').read_text()
    originals = '\n'.join(between(models, f'X86_FUNC(__imp__sub_{a}) {{', f'\nX86_WEAK_FUNC(sub_{a}')
                          for a in ('000B5D80', '000B5DD0'))
    constants = '\n'.join(f'Word(0x{a:x})=0x{struct.unpack("<I",image(a,4))[0]:x};'
                          for a in (0x44c174, 0x44bf94))
    test = (local/'test_xbe_effect_settings.cpp').read_text()
    for marker, source in [('// ORIGINAL_RECEIVER_DISPATCH', block),
                           ('// ORIGINAL_PLAYER_SETTERS', originals),
                           ('// ORIGINAL_CONSTANTS', constants)]:
        assert test.count(marker) == 1
        test = test.replace(marker, source, 1)
    (area/'src/compat/test_effect_settings.cpp').write_text(test)

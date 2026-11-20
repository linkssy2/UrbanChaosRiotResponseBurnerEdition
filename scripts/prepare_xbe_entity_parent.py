"""Decline optional muzzle-flash creation without a registered parent or free ID."""
from pathlib import Path
import hashlib
import re
import struct

ROOT = Path(__file__).resolve().parents[1]
INCLUDE = '#include "xbe_entity_parent.h"\n'
HOOK = '\tif (trial_entity_parent::RejectFlare(ctx, base)) { ctx.eax = 0xffffffffu; ctx.esp += 16; return; }\n'
PLAYER_HOOK = '\tif (!ctx.eax) { ctx.esp += 4; ++ctx.edi; ctx.ebx += 4; goto loc_000D09E0; }\n'
PLAYER_CALL = '\tX86_PUSH32(0xD0A22u); sub_001F8FC0(ctx, base);\n'
PLAYER_PINS = {
    0xD0920: '549008eb02d011d61ffe9b25368af654190d38b932eabe3b290bd28be72d29ab',
    0xD09D9: '12906efa648eeb7f173c1cb667807a4fe09eabe2872357a168b327222dbd04d4',
}
PINS = {
    0x20A230: (52, 46, '46ed0f6bb09ef6c4111f864d06722430432e556ee4661018ac7a54c8c2ef7f9c', 'b270c258e71a3367d2348f1ed6aeae118045d69be0e2467197f9c47e8750bec4'),
    0x20A260: (52, 46, '638290ae26da0b4524c91c74e94bb13f29b0171fe8cb675c4aa1c30fdc4700a9', 'aa2dd56b4db999c0796b22d6a484778f3e5ae73bd729a7ece57d9f918d79cf8f'),
}
FIXTURE_BODIES = {0x20A230: 52, 0x20A260: 52, 0x207C70: 52, 0x207D70: 52,
                  0x20A850: 52, 0x207CD0: 52, 0x207D20: 52, 0x1F8E30: 50,
                  0x1F8FC0: 50, 0x1F92B0: 50}


def body(source, address):
    return re.search(rf'X86_FUNC\(__imp__sub_{address:08X}\) \{{.*?^\}}', source, re.M | re.S)[0]


def clean(source):
    assert source.count(INCLUDE) <= 1 and source.count(HOOK) in (0, 2)
    return source.replace(INCLUDE, '').replace(HOOK, '')


def unprepare(area):
    path = Path(area) / 'generated/game/x86_recomp.52.cpp'
    if path.exists():
        old = path.read_text()
        source = clean(old)
        if source != old:
            path.write_text(source)
    path = Path(area) / 'generated/game/x86_recomp.21.cpp'
    if path.exists():
        old = path.read_text()
        assert old.count(PLAYER_HOOK) in (0, 2)
        if PLAYER_HOOK in old:
            path.write_text(old.replace(PLAYER_HOOK, ''))


def prepare(area):
    area = Path(area)
    path = area / 'generated/game/x86_recomp.52.cpp'
    source = clean(path.read_text())
    image = (area / 'assets/Default.xbe').read_bytes()
    base = struct.unpack_from('<I', image, 0x104)[0]
    count, table = struct.unpack_from('<II', image, 0x11c)
    sections = [struct.unpack_from('<6I', image, table - base + i * 56) for i in range(count)]
    for address, (_, size, raw_hash, generated_hash) in PINS.items():
        section = next(s for s in sections if s[1] <= address and address + size <= s[1] + s[4])
        offset = section[3] + address - section[1]
        assert hashlib.sha256(image[offset:offset + size]).hexdigest() == raw_hash, hex(address)
        assert hashlib.sha256(body(source, address).encode()).hexdigest() == generated_hash, hex(address)
    section = next(s for s in sections if s[1] <= 0x458978 < s[1] + s[4])
    offset = section[3] + 0x458978 - section[1]
    assert image[offset:offset + 9] == b'GunFlare\0'
    player_path = area / 'generated/game/x86_recomp.21.cpp'
    player_source = player_path.read_text().replace(PLAYER_HOOK, '')
    player = body(player_source, 0xD0920)
    for address, pin in PLAYER_PINS.items():
        assert hashlib.sha256(body(player_source, address).encode()).hexdigest() == pin, hex(address)
    section = next(s for s in sections if s[1] <= 0xD0A1A and 0xD0A57 <= s[1] + s[4])
    offset = section[3] + 0xD0A1A - section[1]
    assert hashlib.sha256(image[offset:offset + 61]).hexdigest() == '0c309bccc33a281c0146651939c86c94c840cbba799f76af8481c505cabe1340'
    assert player.count(PLAYER_CALL) == 1
    assert player_source.count(PLAYER_CALL) == len(PLAYER_PINS)
    sources = {52: source, 50: (area / 'generated/game/x86_recomp.50.cpp').read_text()}
    bodies = {a: body(sources[n], a) for a, n in FIXTURE_BODIES.items()}
    patched = {a: bodies[a].replace('{\n', '{\n' + HOOK, 1) for a in PINS}
    calls = set(re.findall(r'sub_([0-9A-F]{8})\(ctx, base\)', '\n'.join(bodies.values())))
    declarations = '\n'.join(f'X86_EXTERN_FUNC(sub_{name});' for name in sorted(calls))
    extracted = '\n'.join(b.replace(f'__imp__sub_{a:08X}', f'sub_{a:08X}', 1) for a, b in bodies.items())
    fixed = '\n'.join(b.replace(f'__imp__sub_{a:08X}', f'Fixed_{a:08X}', 1) for a, b in patched.items())
    fragment = player[player.index('\t// 000D0A1A '):player.index('loc_000D0A57:')]
    consumer = ''
    for name, block in [('OriginalFlareTransform', fragment),
                        ('FixedFlareTransform', fragment.replace(PLAYER_CALL, PLAYER_CALL + PLAYER_HOOK))]:
        consumer += (f'X86_FUNC({name}) {{\n\tX86_LOAD_FLAGS();\n' + block +
                     'loc_000D09E0:\n\treturn;\n}\n')
    fixture = (ROOT / 'scripts/test_xbe_entity_parent.cpp').read_text()
    assert fixture.count('// RETAIL_FUNCTIONS') == 1
    (area / 'src/compat/test_entity_parent.cpp').write_text(fixture.replace('// RETAIL_FUNCTIONS', declarations + '\n' + extracted + '\n' + fixed + '\n' + consumer))
    (area / 'src/compat/xbe_entity_parent.h').write_bytes((ROOT / 'scripts/xbe_entity_parent.h').read_bytes())
    for address, replacement in patched.items():
        source = source.replace(bodies[address], replacement, 1)
    path.write_text(INCLUDE + source)
    player_path.write_text(player_source.replace(PLAYER_CALL, PLAYER_CALL + PLAYER_HOOK))

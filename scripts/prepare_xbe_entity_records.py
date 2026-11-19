"""Repair empty entity-bucket unlinking before its descriptor is reused."""
from pathlib import Path
import hashlib
import re
import struct

PINS = {
    0x221960: '7d6b037f650a48504008a643d2bf0cd2f9645e8affa3b2aa1b1dd95d9901c703',
    0x221AE0: '5e506732362d65f2698817b76da0eb7a833c062ffbc580626618531c4ddcbae7',
    0x221BA0: 'eea1c8ef1c8b088e88753bf97cdcbae97e04eae51948e52b409824d1ebd1ab9c',
    0x221CA0: '476fd8040c6088eb580855de03d532dce74de141fabb46c78305ec52f71f160f',
    0x221D00: 'e422328d3dc8110578ae5639b939651340b336f7c49ffce2810f7a6cbde70bfa',
    0x208F80: '4de70826c50b1a8ca9541bd2d64751006abf9da19db358b220972f956a56ba10',
}
ORIGINAL = '''loc_00221D83:
	// 00221D83 mov esi, [edx+0x08]
	{ uint32_t ea = (uint32_t)(ctx.edx + 0x8u); ctx.esi = (uint32_t)(X86_MEM32(ea)); }
	// 00221D86 test esi, esi
	{ uint32_t a = (uint32_t)(ctx.esi), b = (uint32_t)(ctx.esi); uint32_t r = (uint32_t)(a & b); ca = a; cb = b; cf = 0; of = 0; zf = r == 0; sf = (uint32_t)(r >> 31) & 1; pf = X86_PARITY(r); }
	// 00221D88 jz 0x00221D9D
	if (((uint32_t)(ca & cb)) == 0) { goto loc_00221D9D; }
	// 00221D8A mov ebx, [edx+0x04]
	{ uint32_t ea = (uint32_t)(ctx.edx + 0x4u); ctx.ebx = (uint32_t)(X86_MEM32(ea)); }
	// 00221D8D mov [esi+0x04], ebx
	{ uint32_t ea = (uint32_t)(ctx.esi + 0x4u); X86_MEM32(ea) = (uint32_t)(ctx.ebx); }
	// 00221D90 mov esi, [edx+0x04]
	{ uint32_t ea = (uint32_t)(ctx.edx + 0x4u); ctx.esi = (uint32_t)(X86_MEM32(ea)); }
	// 00221D93 test esi, esi
	{ uint32_t a = (uint32_t)(ctx.esi), b = (uint32_t)(ctx.esi); uint32_t r = (uint32_t)(a & b); ca = a; cb = b; cf = 0; of = 0; zf = r == 0; sf = (uint32_t)(r >> 31) & 1; pf = X86_PARITY(r); }
	// 00221D95 jz 0x00221D9D
	if (((uint32_t)(ca & cb)) == 0) { goto loc_00221D9D; }
	// 00221D97 mov edx, [edx+0x08]
	{ uint32_t ea = (uint32_t)(ctx.edx + 0x8u); ctx.edx = (uint32_t)(X86_MEM32(ea)); }
	// 00221D9A mov [esi+0x08], edx
	{ uint32_t ea = (uint32_t)(ctx.esi + 0x8u); X86_MEM32(ea) = (uint32_t)(ctx.edx); }
'''
FIXED = '''loc_00221D83:
    // Empty buckets leave both neighbors consistent and retain no live links.
    {
        const uint32_t previous = X86_MEM32(ctx.edx + 8);
        const uint32_t next = X86_MEM32(ctx.edx + 4);
        if (previous) X86_MEM32(previous + 4) = next;
        if (next) X86_MEM32(next + 8) = previous;
        X86_MEM32(ctx.edx + 4) = 0;
        X86_MEM32(ctx.edx + 8) = 0;
    }
'''


def clean(source):
    assert source.count(FIXED) <= 1
    return source.replace(FIXED, ORIGINAL)


def unprepare(area):
    path = Path(area) / 'generated/game/x86_recomp.55.cpp'
    if path.exists():
        old = path.read_text()
        source = clean(old)
        if source != old:
            path.write_text(source)


def body(source, address):
    return re.search(rf'X86_FUNC\(__imp__sub_{address:08X}\) \{{.*?^\}}', source, re.M | re.S)[0]


def prepare(area):
    area = Path(area)
    image = (area / 'assets/Default.xbe').read_bytes()
    base = struct.unpack_from('<I', image, 0x104)[0]
    count, table = struct.unpack_from('<II', image, 0x11c)
    sections = [struct.unpack_from('<6I', image, table - base + i * 56) for i in range(count)]
    section = next(s for s in sections if s[1] <= 0x221D00 and 0x221DBD <= s[1] + s[4])
    at = section[3] + 0x221D00 - section[1]
    assert hashlib.sha256(image[at:at + 189]).hexdigest() == '55313088ad0f140a148f7cc5112eb0d2f57cda1794b6703123333d3e8c674d7c'
    path = area / 'generated/game/x86_recomp.55.cpp'
    source = clean(path.read_text())
    bodies = {address: body(source, address) for address in PINS if address != 0x208F80}
    world = body((area / 'generated/game/x86_recomp.52.cpp').read_text(), 0x208F80)
    # These independent co-op observers are not part of the retail fixture.
    for hook in ('\tTrialCoopBodiesRetire(ctx,base);\n', '\tTrialCoopDeathUpdate(ctx,base);\n'):
        world = world.replace(hook, '')
    bodies[0x208F80] = world
    for address, original in bodies.items():
        assert hashlib.sha256(original.encode()).hexdigest() == PINS[address], hex(address)
    original = bodies[0x221D00]
    assert original.count(ORIGINAL) == 1
    patched = original.replace(ORIGINAL, FIXED)
    calls = set(re.findall(r'sub_([0-9A-F]{8})\(ctx, base\)', '\n'.join(bodies.values())))
    declarations = '\n'.join(f'X86_EXTERN_FUNC(sub_{name});' for name in sorted(calls))
    extracted = '\n'.join(value.replace(f'__imp__sub_{address:08X}', f'sub_{address:08X}', 1)
                          for address, value in bodies.items())
    fixture = (Path(__file__).parent / 'test_xbe_entity_records.cpp').read_text()
    marker = '// ORIGINAL_BODIES'
    assert fixture.count(marker) == 1
    fixture = fixture.replace(marker, declarations + '\n' + extracted + '\n' +
                              patched.replace('__imp__sub_00221D00', 'FixedRemove', 1))
    (area / 'src/compat/test_entity_records.cpp').write_text(fixture)
    path.write_text(source.replace(original, patched, 1))

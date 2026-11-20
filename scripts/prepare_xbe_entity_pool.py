"""Extend only current-world entity list nodes, retaining original list logic."""
from pathlib import Path
import hashlib
import re
import struct
from prepare_xbe_entity_records import clean as clean_records, ORIGINAL, FIXED

ROOT = Path(__file__).resolve().parents[1]
INCLUDE = '#include "xbe_entity_pool.h"\n'
ENSURE = '\ttrial_entity_pool::Ensure(ctx, base);\n'
RELEASE = '\ttrial_entity_pool::Release(ctx, base);\n'
PINS = {
    0x221930: (55, 0x30, '8dd8074ce942d1f7d62d23d18e143b6cbaf98fe4705d9b3823eb1ac9a5b04e42', '7cabdb60a0d76a3911a818aaf394294d0325192d9c7f034ce27b16c2bd716b68'),
    0x221960: (55, 0x80, '1e3e1f7b386b7d7fe965ca7508a2318d617c3301b9c8dbfff31cc6a1b3955c2e', '7d6b037f650a48504008a643d2bf0cd2f9645e8affa3b2aa1b1dd95d9901c703'),
    0x2219E0: (55, 0x30, 'b95dea94d43b10c209e4fa846f729a616849357f63e9047fec3b7cdad50e1e71', 'f02fa68a37a5518d8a9bb3c2c4f99897b97224b0573232505f51f32419edbc82'),
    0x221A10: (55, 0x70, 'd8a8f4160f2229c71325375f1e1cf34dc21bcc381966381a6449d10620786de1', 'a15bd40de992d1423c41a93fd1ca698d02e7e5e6a1ffc3e63026ff4148225362'),
    0x221A80: (55, 0x40, 'd17874699043b06c5549be00e16c3ea1d641586b0c05fd7631e50a5afe1a1091', '685c83113770d76d9c372ae486e4210825f67e99d2a12612ca207934a0110a0c'),
    0x221AE0: (55, 0xBA, '4262820482f29698a8d9946d560f2a1de953728c35daf6ab21352bbdb031db32', '5e506732362d65f2698817b76da0eb7a833c062ffbc580626618531c4ddcbae7'),
    0x221BA0: (55, 0x100, '49e5bc3ed3ad9ad5230e2710bd28be9213b9bf4421347a89fdf0b3e0171f3c94', 'eea1c8ef1c8b088e88753bf97cdcbae97e04eae51948e52b409824d1ebd1ab9c'),
    0x221CA0: (55, 0x60, 'c0a85028cedccad566c22828174220df43831ff6f777c703770af7c6365b8edc', '476fd8040c6088eb580855de03d532dce74de141fabb46c78305ec52f71f160f'),
    0x221D00: (55, 0xBD, '55313088ad0f140a148f7cc5112eb0d2f57cda1794b6703123333d3e8c674d7c', 'e422328d3dc8110578ae5639b939651340b336f7c49ffce2810f7a6cbde70bfa'),
    0x208CB0: (52, 0x10, '532e65e4e20bb2849ad9741c573ef000b01c9e5a4bc7da5a406ced36fe09dcfa', 'fccefc54835e4760266178e410d132c0dcbe1ec8370ed9438b38618c17f8bfcc'),
}
RAW_PINS = {
    0x221910: (0x20, 'afa3bb7203903729e2baa6f56c7c5075d4ca0b5d8606e3e9a7f83632375a2abc'),
    0x2D589F: (5, '81fa287397ba56519f983bdbeb7b73d37efa192bd0a4a68356d148b968ca3e1b'),
    0x2D631D: (5, 'cdd81754c93a8221c6589038d1d06f9c1ea0280fa0f353cdd9bde0177de43f7f'),
}
HOOKS = {address: ENSURE for address in (0x2219e0, 0x221ba0, 0x221ca0)}
HOOKS.update({address: RELEASE for address in (0x221930, 0x221960, 0x221a80, 0x208cb0)})


def body(source, address):
    return re.search(rf'X86_FUNC\(__imp__sub_{address:08X}\) \{{.*?^\}}', source, re.M | re.S)[0]


def clean(source):
    assert source.count(INCLUDE) <= 1
    assert source.count(ENSURE) in (0, 3) and source.count(RELEASE) in (0, 1, 3)
    return source.replace(INCLUDE, '').replace(ENSURE, '').replace(RELEASE, '')


def unprepare(area):
    for unit in (52, 55):
        path = Path(area) / f'generated/game/x86_recomp.{unit}.cpp'
        if path.exists():
            old = path.read_text()
            result = clean(old)
            if result != old:
                path.write_text(result)


def prepare(area):
    area = Path(area)
    paths = {unit: area / f'generated/game/x86_recomp.{unit}.cpp' for unit in (52, 55)}
    sources = {unit: clean(path.read_text()) for unit, path in paths.items()}
    image = (area / 'assets/Default.xbe').read_bytes()
    base = struct.unpack_from('<I', image, 0x104)[0]
    count, table = struct.unpack_from('<II', image, 0x11c)
    sections = [struct.unpack_from('<6I', image, table-base+i*56) for i in range(count)]
    for address, (size, pin) in RAW_PINS.items():
        section = next(s for s in sections if s[1] <= address and address+size <= s[1]+s[4])
        offset = section[3] + address-section[1]
        assert hashlib.sha256(image[offset:offset+size]).hexdigest() == pin, hex(address)
    originals = {}
    for address, (unit, size, raw_pin, body_pin) in PINS.items():
        section = next(s for s in sections if s[1] <= address and address+size <= s[1]+s[4])
        offset = section[3] + address-section[1]
        assert hashlib.sha256(image[offset:offset+size]).hexdigest() == raw_pin, hex(address)
        original = body(clean_records(sources[unit]), address)
        assert hashlib.sha256(original.encode()).hexdigest() == body_pin, hex(address)
        originals[address] = original
    replacements = {address: body(sources[PINS[address][0]], address).replace('{\n', '{\n'+hook, 1)
                    for address, hook in HOOKS.items()}
    # Keep the already-qualified empty-bucket unlink in the fixture too.
    originals[0x221d00] = originals[0x221d00].replace(ORIGINAL, FIXED)
    calls = set(re.findall(r'sub_([0-9A-F]{8})\(ctx, base\)', '\n'.join(originals.values())))
    declarations = '\n'.join(f'X86_EXTERN_FUNC(sub_{name});' for name in sorted(calls))
    extracted = '\n'.join(value.replace(f'__imp__sub_{address:08X}', f'sub_{address:08X}', 1)
                          for address, value in originals.items())
    fixed = '\n'.join(value.replace(f'__imp__sub_{address:08X}', f'Fixed_{address:08X}', 1)
                      for address, value in replacements.items())
    fixture = (ROOT / 'scripts/test_xbe_entity_pool.cpp').read_text()
    assert fixture.count('// RETAIL_FUNCTIONS') == 1
    (area / 'src/compat/test_entity_pool.cpp').write_text(fixture.replace('// RETAIL_FUNCTIONS',
        declarations+'\n'+extracted+'\n'+fixed))
    (area / 'src/compat/xbe_entity_pool.h').write_bytes((ROOT/'scripts/xbe_entity_pool.h').read_bytes())
    for address, replacement in replacements.items():
        unit = PINS[address][0]
        sources[unit] = sources[unit].replace(body(sources[unit], address), replacement, 1)
    for unit, source in sources.items():
        paths[unit].write_text(INCLUDE + source)

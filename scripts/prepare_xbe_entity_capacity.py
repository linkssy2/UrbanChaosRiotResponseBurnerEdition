"""Exercise the final emitted world registration and retirement paths together."""
from pathlib import Path
import hashlib
import re
import struct

ROOT = Path(__file__).resolve().parents[1]
BODIES = {
    52: (0x207c70, 0x207d70, 0x207da0, 0x207e50, 0x207f00, 0x207f70,
         0x207ff0, 0x208150, 0x20a140, 0x208cb0),
    55: (0x221930, 0x221960, 0x2219e0, 0x221a10, 0x221a80, 0x221ae0,
         0x221ba0, 0x221ca0, 0x221d00),
    50: (0x1f8fc0,),
}
BODY_TARGETS = {0x22ca0: 2, 0x24a00: 2, 0x1e830: 1, 0x2bcfa0: 73,
                0x1f8fc0: 50, 0x207c70: 52, 0x207d70: 52}
BODY_PINS = {
    0x22ca0: '24ea06610142d32b39c3fc27469cd52d05251295fd50340c88c5ff78de01e036',
    0x24a00: 'e132e0b3aa4f125b63dff93d42169c883e9de6222d5007f84a171a32059d219e',
    0x1e830: 'e77f8dbc4d6a575d18578b1badf0df22b54eaf0e1498b26e180008fb52d96daa',
    0x2bcfa0: 'bedb56e0f8c7695f0c40f6cc6c43321d5418f989e6c0b81f14f5a6a125a08c67',
    0x1f8fc0: '847d3fe1b9e6375243e533205c067c6d41465ae459fecfd33170c41e383efcab',
}


def prepare(area):
    area = Path(area)
    bodies = []
    for unit, addresses in BODIES.items():
        source = (area / f'generated/game/x86_recomp.{unit}.cpp').read_text()
        for address in addresses:
            matches = re.findall(rf'X86_FUNC\(__imp__sub_{address:08X}\) \{{.*?^\}}', source, re.M | re.S)
            assert len(matches) == 1, hex(address)
            bodies.append(matches[0].replace(f'__imp__sub_{address:08X}', f'sub_{address:08X}', 1))
    code = '\n'.join(bodies)
    calls = sorted(set(re.findall(r'sub_([0-9A-F]{8})\(ctx, base\)', code)))
    declarations = '\n'.join(f'X86_EXTERN_FUNC(sub_{name});' for name in calls)
    fixture = (ROOT / 'scripts/test_xbe_entity_capacity.cpp').read_text()
    assert fixture.count('// RETAIL_FUNCTIONS') == 1
    (area / 'src/compat/test_entity_capacity.cpp').write_text(
        fixture.replace('// RETAIL_FUNCTIONS', declarations + '\n' + code))
    image = (area / 'assets/Default.xbe').read_bytes()
    image_base = struct.unpack_from('<I', image, 0x104)[0]
    count, table = struct.unpack_from('<II', image, 0x11c)
    sections = [struct.unpack_from('<6I', image, table-image_base+i*56) for i in range(count)]
    section = next(s for s in sections if s[1] <= 0x22ca0 and 0x22d91 <= s[1]+s[4])
    offset = section[3]+0x22ca0-section[1]
    assert hashlib.sha256(image[offset:offset+0xf1]).hexdigest() == 'fdf408ffa01eff7a9fbdb889bf9c4c924640a9b7f7e89af733484c2822bde991'
    bodies = []
    for address, unit in BODY_TARGETS.items():
        source = (area / f'generated/game/x86_recomp.{unit}.cpp').read_text()
        matches = re.findall(rf'X86_FUNC\(__imp__sub_{address:08X}\) \{{.*?^\}}', source, re.M | re.S)
        assert len(matches) == 1, hex(address)
        body = matches[0]
        if address in BODY_PINS:
            assert hashlib.sha256(body.encode()).hexdigest() == BODY_PINS[address], hex(address)
        else:
            assert ('trial_entity_registry::AllocateExtra' if address == 0x207c70 else
                    'trial_entity_registry::FindExtra') in body, hex(address)
        bodies.append(body.replace(f'__imp__sub_{address:08X}', f'sub_{address:08X}', 1))
    code = '\n'.join(bodies)
    calls = sorted(set(re.findall(r'sub_([0-9A-F]{8})\(ctx, base\)', code)))
    declarations = '\n'.join(f'X86_EXTERN_FUNC(sub_{name});' for name in calls)
    fixture = (ROOT / 'scripts/test_xbe_entity_body.cpp').read_text()
    assert fixture.count('// RETAIL_FUNCTIONS') == 1
    (area / 'src/compat/test_entity_body.cpp').write_text(
        fixture.replace('// RETAIL_FUNCTIONS', declarations + '\n' + code))

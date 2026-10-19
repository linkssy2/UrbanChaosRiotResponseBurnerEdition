"""Stage the narrow graphics-entry menu contract and its production fixture."""
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]


def prepare(area):
    area = Path(area)
    data = (area / 'assets/Default.xbe').read_bytes()
    base, = struct.unpack_from('<I', data, 0x104)
    count, table = struct.unpack_from('<II', data, 0x11c)
    sections = [struct.unpack_from('<6I', data, table - base + i * 56) for i in range(count)]

    def read(address, size):
        section = next(s for s in sections if s[1] <= address and address + size <= s[1] + s[4])
        offset = section[3] + address - section[1]
        return data[offset:offset + size]

    # Original factory arguments and base-list constructor, not labels inferred
    # from screenshots. Controller Options deliberately has a different global.
    pins = {
        0x2722A5: '689cd25100',
        0x2677F4: '68a4cf5100',
        0x267F9B: '68a4cf5100',
        0x276DFC: '6810d35100',
        0x276A1C: '6804d35100',
        0x276D18: '680cd35100',
        0x266A3D: 'c706b4774800',
        0x272928: 'c6461401',  # manager transition byte +14
    }
    for address, hex_bytes in pins.items():
        expected = bytes.fromhex(hex_bytes)
        assert read(address, len(expected)) == expected, hex(address)
    for address, label in ((0x485910, b'NewFrontEnd-Options-1\0'),
                           (0x4855A8, b'NewFrontEnd-ControllerOptions-1\0')):
        assert read(address, len(label)) == label, hex(address)

    guest = (ROOT / 'scripts/xbe_trial_guest_input.cpp').read_text()
    for seam in (
        'menu_pointer_begin(g_ecx);GraphicsMenuBegin();break;',
        'menu_pointer_end();GraphicsMenuEnd();',
        'case 3:GraphicsMenuCapture();ucrr_menu_list_leave();break;',
        'case 15:GraphicsMenuDestroy(g_ecx);ucrr_menu_manager_destroy();break;',
        'case 16:ucrr_menu_dialog_enter();GraphicsMenuSpecial();break;',
        'case 20:ucrr_menu_profile_enter();GraphicsMenuSpecial();break;',
        '#include "xbe_frontend_guest.inc"',
    ):
        assert guest.count(seam) == 1, seam
    out = area / 'src/common'
    for name in ('xbe_graphics_menu_context.h', 'test_xbe_graphics_menu_context.cpp'):
        (out / name).write_text((ROOT / 'scripts' / name).read_text())

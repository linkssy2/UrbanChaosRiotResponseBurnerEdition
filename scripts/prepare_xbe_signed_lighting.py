"""Restore the retail signed normalization cube contract in the private D3D12 owner.

Primary precision reference: NV_texture_shader, issues section lines 770-822:
https://registry.khronos.org/OpenGL/extensions/NV/NV_texture_shader.txt
No upstream edits; no post-sample remap; no new light/shadow algorithm.
"""
from pathlib import Path
import hashlib
import shutil
import struct


def once(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new, 1)


def prepare(source, area):
    area = Path(area)
    # Exact retail setup binds normalization cubes at stages 2/3, then selects
    # RGB signed channels (E0000000). Its teardown resets both to unsigned.
    xbe = (area / 'assets/Default.xbe').read_bytes()
    base = struct.unpack_from('<I', xbe, 0x104)[0]
    count, table = struct.unpack_from('<II', xbe, 0x11c)
    sections = [struct.unpack_from('<6I', xbe, table - base + i * 56) for i in range(count)]
    def image(va, size):
        s = next(s for s in sections if s[1] <= va and va + size <= s[1] + s[4])
        offset = s[3] + va - s[1]
        return xbe[offset:offset + size]
    pins = {
        # Filled from the retained retail image, not generated-code text.
        (0xb1a20, 0x88): 'b31dca982391d0c0185c508b3f65921a555a40aa29a69dcc05e4fe9a930a633c',
        (0xb1ab0, 0x85): '096efa2ef76e4e2227e931c365b0b6f70ae61be6b95c796c23c347056531662e',
    }
    for (va, size), digest in pins.items():
        assert hashlib.sha256(image(va, size)).hexdigest() == digest, hex(va)

    source = once(source, '#include "gpu/texture.h"',
                  '#include "gpu/texture.h"\n#include "xbe_signed_lighting.h"')
    source = once(source, '    uint32_t levels = 1;\n    uint64_t sig = 0;',
                  '    uint32_t levels = 1;\n    uint32_t signs = 0;\n    uint64_t sig = 0;')
    source = once(source, '    std::unordered_map<uint64_t, Tex> textures_;  // by data address, a P8 texture\'s with its palette\'s above',
                  '    std::unordered_map<uc_xbe_signed::TextureKey, Tex, uc_xbe_signed::TextureKeyHash> textures_;')
    old = 'const FormatInfo& fi, uint64_t key)'
    assert source.count(old) == 2
    source = source.replace(old, 'const FormatInfo& fi, uc_xbe_signed::TextureKey key)')
    source = once(source, '    tx.fmt = HostFormat(fi.host);',
                  '    tx.signs = key.signs;\n'
                  '    tx.fmt = key.signs ? DXGI_FORMAT_R32G32B32A32_FLOAT : HostFormat(fi.host);')
    source = once(source, '            UploadTextureLevel(tx, f, l, px, pitch);',
                  '            uc_xbe_signed::Expand(px, pitch, tx.signs);\n'
                  '            UploadTextureLevel(tx, f, l, px, pitch);')
    source = once(source, '    const uint64_t key = uint64_t(palette.va) << 32 | d.data;',
                  '    const uc_xbe_signed::TextureKey key{d.data, palette.va,\n'
                  '        uc_xbe_signed::CubeSigns(d, s.tss[stage][kTssColorSign])};')

    generated = (area / 'generated/game/x86_recomp.18.cpp').read_text()
    original = ''
    for name in ('000B1A20', '000B1AB0'):
        start = generated.index(f'X86_FUNC(__imp__sub_{name}) {{')
        end = generated.index(f'\nX86_WEAK_FUNC(sub_{name}', start)
        original += generated[start:end] + '\n'
    fixture = Path(__file__).with_name('test_xbe_signed_lighting.cpp').read_text()
    fixture = once(fixture, '// GENERATED_ORIGINAL', original)
    (area / 'src/compat/test_signed_lighting.cpp').write_text(fixture)
    shutil.copyfile(Path(__file__).with_name('xbe_signed_lighting.h'), area / 'src/compat/xbe_signed_lighting.h')
    return source

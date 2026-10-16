"""Keep retail shader getters coherent with the private host graphics owner."""
from pathlib import Path
import hashlib
import struct


PACK = r'''
// The retained getter reads the SDK's packed ARGB mirror, not the host floats.
// Quantize once with the same MINPS/MAXPS/CVTSS2SI rules for both consumers.
static uint32_t TitlePixelConstant(const std::array<float, 4>& color) {
    uint32_t c[4];
    for (unsigned i = 0; i < 4; ++i)
        c[i] = uint32_t(sse_cvt(sse_max(sse_min(color[i], 1.0f), 0.0f) * 255.0f));
    return c[3] << 24 | c[0] << 16 | c[1] << 8 | c[2];
}
'''


def once(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new, 1)


def between(text, begin, end):
    start = text.index(begin)
    return text[start:text.index(end, start)]


def prepare(sdk, area):
    """Run after the other private d3d8 adapters; never change upstream inputs."""
    sdk, area = Path(sdk), Path(area)
    compat = area / 'src/compat'
    xbe = (area / 'assets/Default.xbe').read_bytes()
    base = struct.unpack_from('<I', xbe, 0x104)[0]
    count, table = struct.unpack_from('<II', xbe, 0x11c)
    sections = [struct.unpack_from('<6I', xbe, table - base + i * 56) for i in range(count)]

    def image(va, size):
        section = next(s for s in sections if s[1] <= va and va + size <= s[1] + s[4])
        at = section[3] + va - section[1]
        return xbe[at:at + size]

    pins = {
        0x3e6930: (20, '967a3b3b2f8cfc802cccbd6ab0080c0e7f9f50cc4f60f9442f31d66f88507611'),
        0x3e6ee0: (355, '331d6371e6e3b1a0a53449715265b95e549f54b473fcc7dd0675260ff5e48dcb'),
        0x3ee920: (872, '98f787a46315fd719062986567b570a2cd510df62fd4b2a37b9d33d2de0d4e0b'),
        0x3eee90: (711, 'a13f9227dd8350350e65bb7b938d10f1d46535dfe8ef8a92cb4312eec0e20ea6'),
    }
    for address, (size, digest) in pins.items():
        assert hashlib.sha256(image(address, size)).hexdigest() == digest
    assert image(0x4eaa80, 16) == struct.pack('<4f', *([1] * 4))
    assert image(0x4eaa90, 16) == struct.pack('<4f', *([255] * 4))

    path = compat / 'd3d8.cpp'
    source = path.read_text()
    vertex = between(source, 'HLE_STDCALL(D3DDevice_SetVertexShader, 1)', '\nHLE_FASTCALL(D3DDevice_SetVertexShaderConstant1Fast')
    fixed_vertex = once(vertex, '    g_state.vs_handle = h;', '''    g_state.vs_handle = h;
    // 3E6930 is still retail code; +794 is an internal object, +798 its public handle.
    SetDev(0x798, h);''')
    pixel = between(source, 'HLE_STDCALL(D3DDevice_SetPixelShaderConstant, 3)', '\n// drawing')
    fixed_pixel = once(pixel,
        '    for (uint32_t i = 0; i < n && reg + i < 16; i++) std::memcpy(g_state.psc[reg + i].data(), HostPtr(src + i * 16), 16);',
        '''    for (uint32_t i = 0; i < n && reg + i < 16; i++) {
        auto& color = g_state.psc[reg + i];
        std::memcpy(color.data(), HostPtr(src + i * 16), 16);
        const uint32_t packed = TitlePixelConstant(color);
        W32(kDevice + 0x8E4 + (reg + i) * 4, packed);
        color = {float((packed >> 16) & 255) / 255, float((packed >> 8) & 255) / 255,
                 float(packed & 255) / 255, float(packed >> 24) / 255};
    }''')
    source = once(source, vertex, fixed_vertex)
    source = once(source, pixel, PACK + '\n' + fixed_pixel)
    path.write_text(source)

    originals = ''
    for chunk, addresses in [(120, ['003E6930']), (121, ['003EE920', '003EEE90'])]:
        generated = (area / f'generated/game/x86_recomp.{chunk}.cpp').read_text()
        for address in addresses:
            originals += between(generated, f'X86_FUNC(__imp__sub_{address}) {{', f'\nX86_WEAK_FUNC(sub_{address}') + '\n'
    generated = (area / 'generated/game/x86_recomp.120.cpp').read_text()
    # The exact original public-handle store, independent of the host setter.
    remember = between(generated, '\t// 003E6F74 mov [esi+0x798], ebx', '\t// 003E6F7A')
    remember = 'static void OriginalRememberHandle(X86Context& ctx, uint8_t* base) {\n' + remember + '}\n'
    texture = (sdk / 'runtime/gpu/texture.cpp').read_text()
    constants = between(texture, 'uint32_t PackColor(', '\n}  // namespace gpu')
    init = []
    for va, size in [(0x4eaa80, 16), (0x4eaa90, 16), (0x4eaaa0, 64), (0x44c23c, 4)]:
        words = struct.unpack('<' + 'I' * (size // 4), image(va, size))
        init.extend(f'W32(0x{va + i * 4:x}, 0x{word:x});' for i, word in enumerate(words))
    test = Path(__file__).with_name('test_xbe_shader_state.cpp').read_text()
    for marker, body in [('// GENERATED_VERTEX', fixed_vertex), ('// GENERATED_PIXEL', PACK + '\n' + fixed_pixel),
                         ('// GENERATED_ORIGINAL', originals + remember),
                         ('// GENERATED_MAPPING', 'namespace gpu {\n' + constants + '\n}\n'),
                         ('// GENERATED_CONSTANTS', '\n'.join(init))]:
        test = once(test, marker, body)
    (compat / 'test_shader_state.cpp').write_text(test)


if __name__ == '__main__':
    root = Path(__file__).resolve().parents[1]
    prepare(root / 'build/upstream-review-xboxxbe-20261014', root / 'build/xbe-full-integration-20261014')

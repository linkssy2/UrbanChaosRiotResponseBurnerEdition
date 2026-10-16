"""Bind the title's verified BeginPush/EndPush ABI to the existing host owner."""
from pathlib import Path
import struct


SYMBOLS = '''
// 9A225 and 9B4D1 emit inline quad arrays through these stdcall(1) APIs.
// The host EndPush already consumes their 52-byte declaration and preserves state.
#undef SYM_D3DDevice_BeginPush
#undef HAS_D3DDevice_BeginPush
#define SYM_D3DDevice_BeginPush 003E9350
#define HAS_D3DDevice_BeginPush 1
#undef SYM_D3DDevice_EndPush
#undef HAS_D3DDevice_EndPush
#define SYM_D3DDevice_EndPush 003E9370
#define HAS_D3DDevice_EndPush 1
'''


def between(text, begin, end):
    start = text.index(begin)
    return text[start:text.index(end, start)]


def replace_once(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new, 1)


def prepare(sdk, area):
    """Run after the graphics/visibility preparation; only touches trial outputs."""
    sdk, area = Path(sdk), Path(area)
    compat = area / 'src/compat'
    path = compat / 'd3d8.cpp'
    d3d = path.read_text()
    assert '// 9A225 and 9B4D1 emit' not in d3d
    d3d = replace_once(d3d, '#include "xdk/hle.h"', '#include "xdk/hle.h"\n' + SYMBOLS)
    path.write_text(d3d)

    # The shader selected by both producers is initialized from this XBE table.
    # Keep all original-code/data fixtures private to the generated test folder.
    xbe = (area / 'assets/Default.xbe').read_bytes()
    base = struct.unpack_from('<I', xbe, 0x104)[0]
    count, headers = struct.unpack_from('<II', xbe, 0x11c)
    sections = [struct.unpack_from('<6I', xbe, headers - base + i * 56) for i in range(count)]

    def words(va, count):
        section = next(s for s in sections if s[1] <= va and va + count * 4 <= s[1] + s[4])
        return struct.unpack_from('<' + 'I' * count, xbe, section[3] + va - section[1])

    # 99890 initializes each row as (program, is_pixel, output, declaration).
    assert words(0x4f8520, 4) == (0x4eb158, 0, 0x51a204, 0x4f7c6c)
    decl = words(0x4f7c6c, 7)
    assert decl == (0x20000000, 0x40320000, 0x40400001, 0x40320002, 0x40320003, 0x40320004, 0xffffffff)
    title = (area / 'generated/game/x86_recomp.15.cpp').read_text()
    helpers = ''
    for address in ['00099960', '00099980']:
        body = between(title, f'X86_FUNC(__imp__sub_{address}) {{', f'\nX86_WEAK_FUNC(sub_{address}')
        helpers += body.replace(f'__imp__sub_{address}', f'sub_{address}') + '\n'
    emitter = between(title, '\t// 0009B4C4 mov esi, edi', '\t// 0009B688 mov ecx, [esp+0x18]')
    emitter = 'static void EmitOriginal(X86Context& ctx, uint8_t* base) {\nX86_LOAD_FLAGS();\nuint32_t ca=0, cb=0;\n' + emitter + '\n}\n'

    source = (sdk / 'runtime/xdk/d3d8.cpp').read_text()
    texture = (sdk / 'runtime/gpu/texture.cpp').read_text()
    vertex = (sdk / 'runtime/gpu/vertex.cpp').read_text()
    pieces = {
        '// GENERATED_SYMBOLS': SYMBOLS,
        '// GENERATED_DECLARATION': 'static const std::vector<uint32_t> title_decl = {' + ','.join(hex(t) + 'u' for t in decl) + '};',
        '// GENERATED_PARSE': between(texture, 'static uint32_t TypeSize(', '// pixel shader constants'),
        '// GENERATED_FVF': between(vertex, 'uint32_t TypeBytes(', 'constexpr uint32_t kResData') + between(vertex, 'void FvfInputs(', '\nbool Assemble('),
        '// GENERATED_SCRATCH': between(source, 'uint32_t ScratchPush(', '\ngpu::Surface SurfaceOf('),
        '// GENERATED_SYNC': between(source, 'void SyncDeferred() {', '\nfloat ZScale('),
        '// GENERATED_DRAWUP': between(source, 'void DrawUp(', '\nuint32_t g_push_begin'),
        '// GENERATED_PUSH': between(source, 'HLE_STDCALL(D3DDevice_BeginPush, 1)', '// frame buffer compression tiles'),
        '// GENERATED_ORIGINAL': helpers + emitter,
    }
    test = Path(__file__).with_name('test_xbe_inline_draws.cpp').read_text()
    for marker, body in pieces.items():
        test = replace_once(test, marker, body)
    (compat / 'test_inline_draws.cpp').write_text(test)


if __name__ == '__main__':
    root = Path(__file__).resolve().parents[1]
    prepare(root / 'build/upstream-review-xboxxbe-20261014',
            root / 'build/xbe-full-integration-20261014')

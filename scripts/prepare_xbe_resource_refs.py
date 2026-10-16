"""Preserve public surface/parent ownership in the isolated SDK graphics hooks."""
from pathlib import Path
import hashlib
import struct


PUBLIC_REF = '''// A returned surface owns a public reference, distinct from the device's
// internal references. Its first public reference also retains its parent;
// incrementing Common directly lets a later Release free a still-owned texture.
uint32_t PublicReference(X86Context& ctx, uint32_t resource) {
    if (resource) kernel::CallGuest(ctx, 0x003E5570, {resource});
    return resource;
}

'''


def between(text, begin, end):
    start = text.index(begin)
    return text[start:text.index(end, start)]


def once(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new, 1)


def prepare(sdk, area):
    """Change only generated private outputs; keep original ownership code intact."""
    sdk, area = Path(sdk), Path(area)
    compat = area / 'src/compat'
    xbe = (area / 'assets/Default.xbe').read_bytes()
    base = struct.unpack_from('<I', xbe, 0x104)[0]
    count, headers = struct.unpack_from('<II', xbe, 0x11c)
    sections = [struct.unpack_from('<6I', xbe, headers - base + i * 56) for i in range(count)]
    va, size = 0x3e5570, 55
    section = next(s for s in sections if s[1] <= va and va + size <= s[1] + s[4])
    offset = section[3] + va - section[1]
    assert hashlib.sha256(xbe[offset:offset + size]).hexdigest() == 'fc04f80709305c363d8573abf7ed0d75db798bfb139abc8e23234f38282fd9b5'

    path = compat / 'd3d8.cpp'
    d3d = path.read_text()
    assert 'uint32_t PublicReference(' not in d3d
    start = 'HLE_STDCALL(D3DDevice_GetBackBuffer2, 1) {'
    getters = between(d3d, start, '\nnamespace {\nstd::atomic<uint32_t> g_presents')
    fixed = once(getters, '    W32(s, R32(s) + 1);\n    return s;', '    return PublicReference(ctx, s);')
    assert fixed.count('    if (s) W32(s, R32(s) + 1);\n    return s;') == 2
    fixed = fixed.replace('    if (s) W32(s, R32(s) + 1);\n    return s;', '    return PublicReference(ctx, s);')
    for name, argc in [('GetBackBuffer2', 1), ('GetRenderTarget2', 0), ('GetDepthStencilSurface2', 0)]:
        marker = f'HLE_STDCALL(D3DDevice_{name}, {argc}) {{\n'
        fixed = once(fixed, marker, marker + '    std::lock_guard<std::recursive_mutex> lock(g_lock);\n')
    path.write_text(once(d3d, getters, PUBLIC_REF + fixed))

    # The original-code oracle stays in the private build directory, never in
    # the source tree or asset-free release. Exercise real retain/release logic.
    original = (area / 'generated/game/x86_recomp.120.cpp').read_text()
    oracle = ''
    for address in ['003E5530', '003E5570', '003E55B0', '003E56D0', '003E3250', '003E35D0', '003E35F0']:
        body = between(original, f'X86_FUNC(__imp__sub_{address}) {{', f'\nX86_WEAK_FUNC(sub_{address}')
        if address in ['003E5530', '003E5570', '003E55B0', '003E56D0']:
            body = body.replace(f'__imp__sub_{address}', f'sub_{address}')
        oracle += body + '\n'
    dispatch = (sdk / 'runtime/cpu/dispatch.cpp').read_text()
    call = between(dispatch, 'uint32_t CallGuest(X86Context& ctx, uint32_t fn, const uint32_t* args, size_t count) {', '\n}  // namespace kernel')
    test = Path(__file__).with_name('test_xbe_resource_refs.cpp').read_text()
    for marker, body in [('// GENERATED_GETTERS', PUBLIC_REF + fixed), ('// GENERATED_ORIGINAL', oracle),
                         ('// GENERATED_CALL_GUEST', 'namespace kernel {\n' + call + '\n}\n')]:
        test = once(test, marker, body)
    (compat / 'test_resource_refs.cpp').write_text(test)


if __name__ == '__main__':
    root = Path(__file__).resolve().parents[1]
    prepare(root / 'build/upstream-review-xboxxbe-20261014', root / 'build/xbe-full-integration-20261014')

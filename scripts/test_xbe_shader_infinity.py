"""Check the private shader syntax patch; optionally compare native DXBC.

Pass --compiler with an installed x64 d3dcompiler_47.dll to prove identical
bytecode for dynamic, zero, negative-zero and finite helper inputs. This does
not validate Wine's compiler or live D3D12 rendering.
"""
from pathlib import Path
import argparse
import ctypes as c
import hashlib
import json
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parent))
import prepare_xbe_native_lighting as adapter


def check_sources():
    path = adapter.ROOT / 'build/upstream-review-xboxxbe-20261014/runtime/gpu/shaders.cpp'
    original_bytes = path.read_bytes()
    source = path.read_text()
    patched = adapter.patch_infinity_source(source)
    expected = source
    for old, new in adapter.INFINITY_REPLACEMENTS:
        expected = expected.replace(old, new, 1)
    assert patched == expected and '#INF' not in patched
    assert sum(a != b for a, b in zip(source.splitlines(), patched.splitlines(), strict=True)) == 2
    invalid = [patched]
    for old, new in adapter.INFINITY_REPLACEMENTS:
        invalid.extend((source.replace(old, '', 1), source + '\n' + old,
                        source.replace(old, new, 1)))
    for text in invalid:
        try:
            adapter.patch_infinity_source(text)
        except ValueError:
            pass
        else:
            raise AssertionError('Unknown, partial, duplicate or repeated patch accepted')
    with tempfile.TemporaryDirectory(prefix='uc-shader-infinity-') as directory:
        output = Path(directory) / 'shaders.cpp'
        adapter.prepare_shaders(path, output)
        assert output.read_text() == adapter.patch_sum_source(patched)
        first = output.read_bytes()
        adapter.prepare_shaders(path, output)
        assert output.read_bytes() == first
    assert path.read_bytes() == original_bytes
    start, end = source.index('float4 ilu_rsq('), source.index('// power clamped to 128')
    return source[start:end], patched[start:patched.index('// power clamped to 128')]


def check_bytecode(compiler, original, patched):
    library = c.WinDLL(str(compiler.resolve()))
    compile_shader = library.D3DCompile
    compile_shader.argtypes = [c.c_void_p, c.c_size_t, c.c_char_p, c.c_void_p, c.c_void_p,
                               c.c_char_p, c.c_char_p, c.c_uint, c.c_uint,
                               c.POINTER(c.c_void_p), c.POINTER(c.c_void_p)]
    compile_shader.restype = c.c_long

    def read_blob(blob):
        if not blob:
            return b''
        table = c.cast(blob, c.POINTER(c.POINTER(c.c_void_p))).contents
        try:
            pointer = c.WINFUNCTYPE(c.c_void_p, c.c_void_p)(table[3])(blob)
            size = c.WINFUNCTYPE(c.c_size_t, c.c_void_p)(table[4])(blob)
            return c.string_at(pointer, size)
        finally:
            c.WINFUNCTYPE(c.c_ulong, c.c_void_p)(table[2])(blob)

    def compile_one(text, profile):
        data = text.encode('ascii')
        code, errors = c.c_void_p(), c.c_void_p()
        status = compile_shader(data, len(data), None, None, None, b'main',
                                profile.encode(), 0, 0, c.byref(code), c.byref(errors))
        message, bytecode = read_blob(errors), read_blob(code)
        assert status >= 0 and bytecode, (hex(status & 0xffffffff), message.decode('utf8', 'replace'))
        return bytecode

    dynamic = ' { float4 a=ilu_rsq(v.x), b=ilu_log(v.y); return float4(a.x,b.x,b.z,a.z); }'
    cases = {
        'dynamic-vs': ('vs_5_0', 'float4 main(float4 v:POSITION):SV_Position' + dynamic),
        'dynamic-ps': ('ps_5_0', 'float4 main(float4 v:TEXCOORD):SV_Target' + dynamic),
        'zero-cs': ('cs_5_0', '''RWStructuredBuffer<uint4> output:register(u0);
            [numthreads(1,1,1)] void main(uint3 id:SV_DispatchThreadID) {
                output[0]=asuint(ilu_rsq(0)); output[1]=asuint(ilu_log(0));
                output[2]=asuint(ilu_rsq(asfloat(0x80000000u)));
                output[3]=asuint(ilu_log(asfloat(0x80000000u))); }'''),
        'finite-cs': ('cs_5_0', '''RWStructuredBuffer<float4> output:register(u0);
            [numthreads(1,1,1)] void main(uint3 id:SV_DispatchThreadID) {
                output[0]=ilu_rsq(4); output[1]=ilu_rsq(-4);
                output[2]=ilu_log(2); output[3]=ilu_log(-2); }'''),
    }
    report = {}
    for name, (profile, body) in cases.items():
        before, after = compile_one(original + body, profile), compile_one(patched + body, profile)
        assert before == after, name + ' changed native bytecode'
        report[name] = {'identical': True, 'bytes': len(after), 'sha256': hashlib.sha256(after).hexdigest()}
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path)
    args = parser.parse_args()
    original, patched = check_sources()
    result = {'source_checks': True, 'native_dxbc': None, 'wine_or_live_verified': False}
    if args.compiler:
        result['native_dxbc'] = check_bytecode(args.compiler, original, patched)
    print(json.dumps(result, indent=2))

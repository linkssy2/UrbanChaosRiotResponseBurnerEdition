"""Prepare the opt-in typed pixel-lighting adapter, without editing upstream.

The caller supplies an already prepared private d3d12.cpp. Unknown source seams
and repeated application fail closed. No vertex shader replacement is included:
the new compiler's palette rounding/zero arithmetic still need separate parity.
"""
from pathlib import Path
import argparse
import shutil

ROOT = Path(__file__).resolve().parents[1]
INCLUDE = '#include "gpu/shaders.h"'
SEAM = '    sh.ps_code = Compile(PixelHlsl(pk), "main", "ps_5_0", sh.ph);'
REPLACEMENT = '''    auto source = uc_xbe_lighting::Pixel(pk, PixelHlsl(pk), uc_xbe_lighting::Enabled());
    sh.ps_code = Compile(source, "main", "ps_5_0", sh.ph);'''
SUM_SEAM = '''inv_v1 ? "(1 - v1)" : "v1",
                         inv_r0 ? "(1 - r0)" : "r0"'''
SUM_REPLACEMENT = '''inv_v1 ? "(1 - max(v1, 0))" : "max(v1, 0)",
                         inv_r0 ? "(1 - max(r0, 0))" : "max(r0, 0)"'''
INFINITY_REPLACEMENTS = (
    ('float4 ilu_rsq(float v) { return v == 0 ? (1.#INF).xxxx : (rsqrt(abs(v))).xxxx; }',
     'float4 ilu_rsq(float v) { return v == 0 ? (asfloat(0x7f800000u)).xxxx : (rsqrt(abs(v))).xxxx; }'),
    ('    return mag == 0 ? float4(-1.#INF, 1, -1.#INF, 1) : float4(ex, mag / exp2(ex), l2, 1);',
     '    return mag == 0 ? float4(asfloat(0xff800000u), 1, asfloat(0xff800000u), 1) : float4(ex, mag / exp2(ex), l2, 1);'),
)


def patch_source(source):
    if source.count(INCLUDE) != 1 or source.count(SEAM) != 1 or 'uc_xbe_lighting::' in source:
        raise ValueError('Expected the unique unmodified pixel shader cache-miss seam')
    return source.replace(INCLUDE, INCLUDE + '\n#include "xbe_native_lighting.h"', 1).replace(SEAM, REPLACEMENT, 1)


def patch_sum_source(source):
    """Correct final SUM operand mapping in a private shader compiler copy.

    NVIDIA NV_register_combiners section 3.8.12.3 maps each SUM operand
    through unsigned identity before addition. Current upstream omits that
    lower clamp. The title's qualified SUM shaders use final flags 0x80.
    https://registry.khronos.org/OpenGL/extensions/NV/NV_register_combiners.txt
    """
    if source.count(SUM_SEAM) != 1:
        raise ValueError('Expected the unique original final-SUM operand mapping')
    return source.replace(SUM_SEAM, SUM_REPLACEMENT, 1)


def patch_infinity_source(source):
    """Spell IEEE infinities as HLSL bit casts instead of Microsoft literals."""
    if any(source.count(old) != 1 or new in source for old, new in INFINITY_REPLACEMENTS):
        raise ValueError('Expected the two unique original infinity helper lines')
    for old, new in INFINITY_REPLACEMENTS:
        source = source.replace(old, new, 1)
    return source


def private_paths(source_path, output_path):
    source_path, output_path = Path(source_path).resolve(), Path(output_path).resolve()
    upstream = (ROOT / 'build/upstream-review-xboxxbe-20261014').resolve()
    if source_path == output_path or output_path.is_relative_to(upstream):
        raise ValueError('Use a distinct private output; upstream cannot be overwritten')
    return source_path, output_path


def prepare_shaders(source_path, output_path):
    """Prepare gpu/shaders.cpp alongside the private D3D12 adapter, even when
    typed lighting is disabled. No upstream source is modified.
    """
    source_path, output_path = private_paths(source_path, output_path)
    source = patch_infinity_source(patch_sum_source(source_path.read_text()))
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(source)


def prepare(source_path, output_path):
    source_path, output_path = private_paths(source_path, output_path)
    source = patch_source(source_path.read_text())
    output_path.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(ROOT / 'scripts/xbe_native_lighting.h', output_path.parent / 'xbe_native_lighting.h')
    shutil.copyfile(ROOT / 'src/native_lighting_pixel_source.h', output_path.parent / 'native_lighting_pixel_source.h')
    output_path.write_text(source)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--shaders-input', type=Path)
    parser.add_argument('--shaders-output', type=Path)
    args = parser.parse_args()
    if bool(args.shaders_input) != bool(args.shaders_output):
        parser.error('--shaders-input and --shaders-output must be supplied together')
    prepare(args.input, args.output)
    if args.shaders_input:
        prepare_shaders(args.shaders_input, args.shaders_output)

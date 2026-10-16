"""Preserve every original ECG draw; add only a short synchronized bar highlight."""
from pathlib import Path
import hashlib
import struct


def between(text, begin, end):
    start = text.index(begin)
    return text[start:text.index(end, start)]


def prepare(sdk, area):
    """After generated-input preparation; stage one source and its CPU fixture."""
    sdk, area = Path(sdk), Path(area)
    local = Path(__file__).parent
    root = local.parent
    xbe = (area / 'assets/Default.xbe').read_bytes()
    base = struct.unpack_from('<I', xbe, 0x104)[0]
    count, table = struct.unpack_from('<II', xbe, 0x11c)
    sections = [struct.unpack_from('<6I', xbe, table-base+i*56) for i in range(count)]

    def image(va, size):
        s = next(s for s in sections if s[1] <= va and va+size <= s[1]+s[4])
        at = s[3]+va-s[1]
        return xbe[at:at+size]

    for va, size, digest in [
        (0x71960, 976, '93f52968b031cfe38f8fa66c5d192f928003caea051ffaca6b0438299d2b0f19'),
        (0x71d70, 992, '3ce95e6b986f9569e32a88b00cc0a87953de9e6c0f2034ccf458359825db4b62'),
        (0x76a50, 736, '5d703977efb27bb1283f614464c7a46dcf7269c5af635b34aa5cc119268d030e'),
        (0x7b180, 434, '5cc15ed48e65f9ccacf47389384b562701fa7c1c2ff9fe107c2a4549b24d3246'),
        (0x74190, 260, '6d7969b559d730c6a6dc19d0db343ac3c8db94c17526b0c514079e50214f8347'),
        (0x718a0, 87, '939c58895e89c7cb466570e62e84178461b147aaa4b11fb85cb79d285846efd7'),
        (0x2d50c0, 118, '31f5d6c2ede8d6ae231ae05eb620b2a21f922c7f2fb381ae4be786477aec4370'),
    ]:
        assert hashlib.sha256(image(va, size)).hexdigest() == digest
    assert image(0x7200d, 5) == bytes.fromhex('8b4e7833c0')
    assert image(0x71ea1, 8) == bytes.fromhex('8b4604bf01000000')
    assert image(0x70fbe, 22) == bytes.fromhex('894e08c706902f4500895610f30f114614f30f114618')
    assert image(0x453558, 4) == struct.pack('<I', 0x76d30)
    assert image(0x452f98, 4) == struct.pack('<I', 0x71d70)
    assert image(0x4531c8, 4) == struct.pack('<I', 0x74190)
    assert image(0x4e8558, 16) == struct.pack('<4f', 55, 11, 1, -2)
    assert image(0x4e8518, 8) == struct.pack('<2f', 0, 14)
    assert image(0x44c1f8, 4) == struct.pack('<f', .5)
    # These accepted scalar owners were checked against unchanged71960 and
    # the shared retail RNG; the fixture uses their real clock/sample stream.
    for name,digest in [
        ('hud_heartbeat.c','71da4f33d275429d35d7c8eef8aceb4593f987ca89fb4bf3a8dcf22888436e82'),
        ('game_random.c','957465e62fd1f084c14683f1ad765412c6723a07a4dd98832fbc11b8b84da7dd'),
        ('game_random_seed.inc','aaa426fa05b9f52f9c4dd1295966e280186ead6b832fa2db8f159abc9990bfad')]:
        assert hashlib.sha256((root/'native'/name).read_bytes()).hexdigest()==digest

    path = area/'generated/game/x86_recomp.11.cpp'
    generated = path.read_text()
    fill = between(generated, 'X86_FUNC(__imp__sub_00074190) {', '\nX86_WEAK_FUNC(sub_00074190')
    # Remove both prior experiments before installing one additive seam. No
    # original ECG segment, decorative frame, fill or damage layer is skipped.
    generated=generated.replace('\tTrialHealthPulse(ctx, base);\n','')
    generated=generated.replace('\tif (TrialHealthPulse(ctx, base)) goto loc_0007200D;\n','')
    generated=generated.replace('extern bool TrialHealthPulse(X86Context&, uint8_t*);',
                                'extern void TrialHealthPulse(X86Context&, uint8_t*);')
    marker = '\t// 0007200D mov ecx, [esi+0x78]'
    assert generated.count(marker) == 1
    hook = '\tTrialHealthPulse(ctx, base);\n'
    generated = generated.replace(marker, hook+marker, 1)
    if 'extern void TrialHealthPulse(X86Context&, uint8_t*);' not in generated:
        generated = generated.replace('#include "x86_recomp_shared.h"',
            '#include "x86_recomp_shared.h"\nextern void TrialHealthPulse(X86Context&, uint8_t*);', 1)
    assert generated.count(hook) == 1 and hook+marker in generated
    assert fill == between(generated, 'X86_FUNC(__imp__sub_00074190) {', '\nX86_WEAK_FUNC(sub_00074190')
    path.write_text(generated)
    (area/'src/common/xbe_health_pulse.cpp').write_text((local/'xbe_health_pulse.cpp').read_text())

    original = ''
    for chunk, address in [(11, '000718A0'), (11, '00074190'), (12, '0007B180'), (13, '000871A0'), (78, '002D50C0')]:
        text = (area/f'generated/game/x86_recomp.{chunk}.cpp').read_text()
        body = between(text, f'X86_FUNC(__imp__sub_{address}) {{', f'\nX86_WEAK_FUNC(sub_{address}')
        original += body.replace(f'__imp__sub_{address}', f'sub_{address}')+'\n'
    heartbeat = between(generated, 'X86_FUNC(__imp__sub_00071D70) {', '\nX86_WEAK_FUNC(sub_00071D70')
    original += heartbeat.replace('__imp__sub_00071D70','RetailHeartbeat').replace(hook,'')+'\n'
    original += heartbeat.replace('__imp__sub_00071D70','HookedHeartbeat')+'\n'
    dispatch = (sdk/'runtime/cpu/dispatch.cpp').read_text()
    call = between(dispatch, 'uint32_t CallGuest(X86Context& ctx, uint32_t fn, const uint32_t* args, size_t count) {',
                   '\n}  // namespace kernel')
    constants = '\n'.join(f'W32(0x{a:x},0x{struct.unpack("<I",image(a,4))[0]:x});'
                          for a in [0x44c174,0x44e868,0x4505cc,0x44bf94,0x4e846c,0x4e8394,
                                    0x4e8518,0x4e851c,0x44c510,0x44c530,0x453010])
    test = (local/'test_xbe_health_pulse.cpp').read_text()
    for marker, source in [('// GENERATED_ORIGINAL', original),
                           ('// GENERATED_CALL_GUEST', 'namespace kernel {\n'+call+'\n}\n'),
                           ('// GENERATED_CONSTANTS', constants)]:
        assert test.count(marker) == 1
        test = test.replace(marker, source, 1)
    (area/'src/compat/test_health_pulse.cpp').write_text(test)

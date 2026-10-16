"""Recover a byte-qualified retail SoundRegion callback without repartitioning the game.

The scanner admitted overlapping interior guesses before reaching the constructor's
callback literal. Use its own bounded-function code generator, then add the real
entry to the normal generated dispatch table. No guest behavior is replaced.
"""
from pathlib import Path
import hashlib
import json
import os
import re
import struct
import subprocess


def between(text, begin, end):
    start = text.index(begin)
    return text[start:text.index(end, start)]


def body(text, address):
    name = f'{address:08X}'
    return between(text, f'X86_FUNC(__imp__sub_{name}) {{',
                   f'\nX86_WEAK_FUNC(sub_{name}, __imp__sub_{name})')


def write_changed(path, text):
    if not path.exists() or path.read_text() != text:
        path.write_text(text)


def prepare(sdk, area):
    """Run after input preparation, inside the parent's serialized build wrapper."""
    assert os.environ.get('UCRR_SERVICES_HEAVY_MUTEX') == 'owned'
    sdk, area = Path(sdk), Path(area)
    local = Path(__file__).parent
    image_path = area/'assets/Default.xbe'
    xbe = image_path.read_bytes()
    image_base = struct.unpack_from('<I', xbe, 0x104)[0]
    count, table = struct.unpack_from('<II', xbe, 0x11c)
    sections = [struct.unpack_from('<6I', xbe, table-image_base+i*56) for i in range(count)]

    def image(va, size):
        section = next(s for s in sections if s[1] <= va and va+size <= s[1]+s[4])
        at = section[3]+va-section[1]
        return xbe[at:at+size]

    pins = [
        (0x10ffd0, 90, '487a6e4566299915b7940933215320890b9d95074c4c4e15c6def25ce24f6627'),
        (0x110510, 108, 'ba7983b1b16bbd943efea9fd1e0e56bc8e8a16143e1767c02a2abfabd40f02ea'),
        (0x110a20, 127, '9f843a66233a0474ccda86e4dd0ba86af0456fb77e549f36f31cb26267fd00ed'),
        (0x1b0e60, 55, 'f3b4ae86a0fa2aac2c58fee69311dc5bcad33a0921f1a6b65dc53a2ebd1d88b8'),
        (0x110aa0, 460, '038804b28a85f4e7e04ccab2a960d5c6d5643a349224dfb93739a52afca955b5'),
    ]
    for va, size, digest in pins:
        assert hashlib.sha256(image(va, size)).hexdigest() == digest, hex(va)
    assert image(0x4f9890, 8) == struct.pack('<2I', 0x459cd4, 0x110c70)
    assert image(0x459cd4, 12) == b'SoundRegion\0'
    assert image(0x4f9ea8, 4) == struct.pack('<I', 0x77)
    assert image(0x110bdc, 8) == bytes.fromhex('c7442428d0ff1000')
    assert image(0x110567, 3) == bytes.fromhex('ff5004')

    # Keep the real image and its entry unchanged. Mark unrelated bytes as data
    # only in this isolated extraction config; the original entry emits no code
    # and is discarded. None of the main config's symbols/hooks are regenerated.
    scratch = area/'generated-callbacks/sound-region'
    scratch.mkdir(parents=True, exist_ok=True)
    config = '''[xbe]
path = "assets/Default.xbe"
title = 0x534300F7
entry = 0x82027
out = "generated/callback"
partitions = 1
code_sections = [".text"]
gap_fill = false
pointer_seeds = false
data_ranges = [[0x11000, 0x10FFD0], [0x11002A, 0x3ADAF0]]

[[function]]
at = 0x10FFD0
end = 0x11002A
'''
    config_path = scratch/'callback.toml'
    write_changed(config_path, config)
    compiler = area/'out-msvc/xbe_recomp.exe'
    generated = scratch/'generated'
    manifest = scratch/'codegen-inputs.json'
    inputs = dict(xbe=hashlib.sha256(xbe).hexdigest(),
                  codegen=hashlib.sha256(compiler.read_bytes()).hexdigest(),
                  config=hashlib.sha256(config.encode()).hexdigest())
    if not manifest.exists() or json.loads(manifest.read_text()) != inputs or not (generated/'x86_recomp.0.cpp').exists():
        with (scratch/'codegen.log').open('w') as log:
            result = subprocess.run([str(compiler), str(config_path), str(image_path), str(generated)],
                                    cwd=area, stdout=log, stderr=subprocess.STDOUT)
        assert result.returncode == 0, (scratch/'codegen.log').read_text()
        write_changed(manifest, json.dumps(inputs, indent=2)+'\n')
    callback = body((generated/'x86_recomp.0.cpp').read_text(), 0x10ffd0)
    addresses = [int(v, 16) for v in re.findall(r'^\s*// ([0-9A-F]{8}) ', callback, re.M)]
    assert addresses == [0x10ffd0, 0x10ffd4, 0x10ffd7, 0x10ffdf, 0x10ffe2,
                         0x10ffe4, 0x10ffe9, 0x10fff1, 0x10fff4, 0x10fff8,
                         0x10fffb, 0x110003, 0x11000b, 0x110012, 0x11001a,
                         0x11001f, 0x110027]
    assert 'x86_trap' not in callback and 'ctx.esp += 12; return;' in callback
    assert not re.search(r'\bsub_[0-9A-F]{8}\(ctx', callback)
    full = callback+'\nX86_WEAK_FUNC(sub_0010FFD0, __imp__sub_0010FFD0)\n'
    chunk = area/'generated/game/x86_recomp.27.cpp'
    source = chunk.read_text()
    if 'X86_FUNC(__imp__sub_0010FFD0)' in source:
        assert body(source, 0x10ffd0) == callback
    else:
        anchor = 'X86_FUNC(__imp__sub_00110008) {'
        assert source.count(anchor) == 1
        source = source.replace(anchor, full+'\n'+anchor, 1)
    write_changed(chunk, source)
    declaration = 'X86_EXTERN_FUNC(sub_0010FFD0);\n'
    mapping_path = area/'generated/game/x86_func_mapping.cpp'
    mapping = mapping_path.read_text()
    # Only the mapping TU needs a declaration; avoid forcing all128 generated
    # partitions to rebuild by changing their shared header.
    if declaration not in mapping:
        anchor = '#include "x86_recomp_shared.h"\n'
        assert mapping.count(anchor) == 1
        mapping = mapping.replace(anchor, anchor+declaration, 1)
    assert mapping.count(declaration) == 1
    registration = '\t{0x10FFD0u, sub_0010FFD0},\n'
    if registration not in mapping:
        anchor = '\t{0x110008u, sub_00110008},'
        assert mapping.count(anchor) == 1
        mapping = mapping.replace(anchor, registration+anchor, 1)
    assert mapping.count(registration) == 1
    write_changed(mapping_path, mapping)

    # Execute the real constructor's entry publication, vector copy, event
    # dispatcher, emitted callback and runtime indirect-function lookup. The
    # allocator growth branch is outside this reserved-capacity fixture.
    original = body((area/'generated/game/x86_recomp.43.cpp').read_text(), 0x1b0e60)
    original += '\n'+body(source, 0x110a20)+'\n'+body(source, 0x110510)
    original = re.sub(r'__imp__(sub_[0-9A-F]{8})', r'\1', original)
    original += '\n'+callback.replace('__imp__sub_0010FFD0', 'sub_0010FFD0')
    constructor = between(source, '\t// 00110BC7 ', '\t// 00110BED ')
    original += '\nX86_FUNC(RegisterSoundRegion) {\n'+constructor+'}\n'
    dispatch = (sdk/'runtime/cpu/dispatch.cpp').read_text()
    dispatch = between(dispatch, 'constexpr uint32_t kKernelThunkBase', '\nvoid x86_resume(')
    test = (local/'test_xbe_event_callback.cpp').read_text()
    for marker, replacement in [('// GENERATED_ORIGINAL', original),
                                ('// GENERATED_MAPPING', registration),
                                ('// GENERATED_DISPATCH', dispatch)]:
        assert test.count(marker) == 1
        test = test.replace(marker, replacement, 1)
    write_changed(area/'src/compat/test_event_callback.cpp', test)


"""Apply the qualified upstream stereo EEPROM correction; extract its real callers."""
from pathlib import Path
import hashlib
import struct


def once(source, old, new):
    assert source.count(old) == 1, old
    return source.replace(old, new, 1)


def between(source, begin, end):
    start = source.index(begin)
    return source[start:source.index(end, start)]


def prepare(sdk, area):
    """Run after prepare_io_apc creates xboxkrnl.cpp and before prepare_language."""
    area = Path(area)
    local = Path(__file__).parent
    data = (area / 'assets/Default.xbe').read_bytes()
    base = struct.unpack_from('<I', data, 0x104)[0]
    count, table = struct.unpack_from('<II', data, 0x11c)
    sections = [struct.unpack_from('<6I', data, table-base+i*56) for i in range(count)]

    # Original thunk, XAPI query, title selection, and speaker-mode setter. Both
    # disc bytes and extracted translations are pinned before generating a test.
    pins = [
        (0x86ede, 13, 'ff258cb24400',
         'e9af9560d92e4fa3e87073ad34d0dd77a6239f9c2fc64d23f9b37a74d3b1da5d',
         '1429d131d457796d985c83425d2c829cc4afc2cf46949777ec5805326c7920d9'),
        (0x385a9d, 101,
         '558bec51516a006a048d45fc508d45f8506a09e82914d0ff85c07d048365fc00'
         'e8c202d0ff83f803740c83f806750e668365fe00eb07c745fc010000008b45fcc9c3',
         '48bb51329e26a8a52e9e9240e8b0c7cdbf9a6694a43ab9b264747744a75a7786',
         '1b219b3ef68d468e76c0b729d78e4ca6092200afe02223fda980a911e2c5d74a'),
        (0x243b9d, 60,
         'e8fb1e140025ffff000083e800741648740d487518c744240c02000000eb18'
         '897c240ceb08c744240c000000008b44240c50e8ceff1d00',
         '511bc97730dfa5be32271580d7fb3ada21e23697ddb928a367a6ae274fe3eee6',
         '3d97e7223a6699322fc284eb722a986ad56fbba0905067e211efcb49426c940b'),
        (0x423ba2, 123,
         'e80effffff0fb6c88b44240483f8ff75080905941c4400eb0a25ffff0000'
         'a3941c440085c9740b68a41c4400ff15ecb34400c20400',
         '37a3f0fe53214da0af63169e902f0ac2f11de729f35be18f4daa19cc6af44a97',
         '14f50bb715fa4299bada55f583cdba5263624ad59976f889d37cabab6755a318')]
    bodies = {}
    for va, file, raw_hex, raw_hash, source_hash in pins:
        expected = bytes.fromhex(raw_hex)
        section = next(s for s in sections if s[1] <= va and va+len(expected) <= s[1]+s[4])
        offset = section[3]+va-section[1]
        raw = data[offset:offset+len(expected)]
        assert raw == expected and hashlib.sha256(raw).hexdigest() == raw_hash, hex(va)
        source = (area / f'generated/game/x86_recomp.{file}.cpp').read_text()
        if va == 0x243b9d:
            code = between(source, '\t// 00243B9D ', '\t// 00243BD4 ')
        else:
            code = between(source, f'X86_FUNC(__imp__sub_{va:08X}) {{',
                           f'\nX86_WEAK_FUNC(sub_{va:08X}')
        assert hashlib.sha256(code.encode()).hexdigest() == source_hash, hex(va)
        bodies[va] = code.replace('__imp__sub_', 'sub_')

    path = area / 'src/compat/xboxkrnl.cpp'
    kernel = path.read_text()
    # Xbox stereo is zero. Upstream 5f760a2714cb1abe11b8620354246d0a659ac132.
    changes = [
        ('case 9: v = 0x00010001; break;             // XC_AUDIO_FLAGS stereo',
         'case 9: v = 0; break;                      // XC_AUDIO_FLAGS stereo'),
        ('W32(value + 0xF0, 0x00010001);            // audio flags',
         'W32(value + 0xF0, 0);                     // audio flags, stereo')]
    for old, new in changes:
        if new not in kernel:
            kernel = once(kernel, old, new)
        assert kernel.count(new) == 1 and old not in kernel
    query = between(kernel, 'KERNEL_EXPORT(24, ExQueryNonVolatileSetting, 5) {',
                    '\nKERNEL_EXPORT(1, AvGetSavedDataAddress')
    original = query
    for old, new in changes:
        original = once(original, new, old)
    signature = 'KERNEL_EXPORT(24, ExQueryNonVolatileSetting, 5)'
    original = once(original, signature, 'static uint32_t BeforeQuery(const uint32_t* a)')
    query = once(query, signature, 'static uint32_t AfterQuery(const uint32_t* a)')
    extracted = '\n'.join(bodies[va] for va in (0x86ede, 0x385a9d, 0x423ba2))
    extracted += ('\nX86_FUNC(OriginalTitleMode) {\nX86_LOAD_FLAGS();\n'
                  'uint32_t ca=0,cb=0; (void)ca; (void)cb;\n'
                  + bodies[0x243b9d] + '\treturn;\n}\n')
    test = (local / 'test_xbe_audio_config.cpp').read_text()
    for marker, replacement in [('/* BEFORE_QUERY */', original),
                                ('/* AFTER_QUERY */', query),
                                ('/* ORIGINAL_FUNCTIONS */', extracted)]:
        test = once(test, marker, replacement)
    path.write_text(kernel)
    (area / 'src/compat/test_audio_config.cpp').write_text(test)

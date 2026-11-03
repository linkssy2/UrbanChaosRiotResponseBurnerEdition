"""Correct the active CameraPush integrator without replacing pulse or matrix ownership."""
from pathlib import Path
import hashlib
import re
import struct

INCLUDE = '#include "xbe_camera_push.h"\n'
ENTRY = 'X86_FUNC(__imp__sub_00045C00) {'
SNAPSHOT = '\n\ttrial_camera_push::Step trial_push(base + ctx.ecx, X86_MEMF32(ctx.esp + 4));'
MATRIX = '\t// 0004607F movss xmm0, dword ptr [esi+0x04]'
APPLY = '\ttrial_push.Apply();\n'
ORIGINAL_SHA = '9493ba682e49de86cbef8582d80a0f0090ba6923f42e3401cb8a1c174989bc0d'

def clean(source):
    return source.replace(INCLUDE, '').replace(ENTRY + SNAPSHOT, ENTRY).replace(APPLY + MATRIX, MATRIX)

def function(source):
    return re.search(r'X86_FUNC\(__imp__sub_00045C00\) \{.*?^\}', source, re.M | re.S).group()

def unprepare(area):
    path = Path(area) / 'generated/game/x86_recomp.5.cpp'
    if path.exists():
        before = path.read_text()
        after = clean(before)
        if after != before: path.write_text(after)

def prepare(area):
    area = Path(area)
    local = Path(__file__).parent
    xbe = (area / 'assets/Default.xbe').read_bytes()
    base = struct.unpack_from('<I', xbe, 0x104)[0]
    count, table = struct.unpack_from('<II', xbe, 0x11c)
    sections = [struct.unpack_from('<6I', xbe, table - base + i * 56) for i in range(count)]
    def original(address, size):
        section = next(s for s in sections if s[1] <= address and address + size <= s[1] + s[4])
        at = section[3] + address - section[1]
        return xbe[at:at + size]
    assert hashlib.sha256(original(0x45c00, 0x5ee)).hexdigest() == '8059c5018490083a7abc5e286a7504541948b302df3d39904067150b71b4ab27'
    constants = {int(a,16): original(int(a,16),4).hex() for a in
                 ('44bf80','44bf94','44c5fc','44d964','450008','45000c','450014',
                  '45007c','450080','450084','450088','45008c','450090')}
    path = area / 'generated/game/x86_recomp.5.cpp'
    before = clean(path.read_text())
    native = function(before)
    assert hashlib.sha256(native.encode()).hexdigest() == ORIGINAL_SHA
    assert before.count(ENTRY) == before.count(MATRIX) == 1
    after = before.replace(ENTRY, ENTRY + SNAPSHOT, 1).replace(MATRIX, APPLY + MATRIX, 1)
    matrix_source = (area / 'generated/game/x86_recomp.78.cpp').read_text()
    matrix = re.search(r'X86_FUNC\(__imp__sub_002D3DC0\) \{.*?^\}', matrix_source, re.M | re.S).group()
    assert hashlib.sha256(matrix.encode()).hexdigest() == '538b1ae70f0e9335ca2388f5d8a675a0044de1c264f3bc43bc1a9146785a854a'
    fixture = (local / 'test_xbe_camera_push.cpp').read_text()
    fixture = fixture.replace('// ORIGINAL_AND_CORRECTED_INTEGRATORS',
        matrix.replace('__imp__sub_002D3DC0', 'sub_002D3DC0', 1) + '\n' +
        native.replace('__imp__sub_00045C00', 'Original', 1) + '\n' +
        function(after).replace('__imp__sub_00045C00', 'Corrected', 1))
    seed = '\n'.join(f'Put(0x{a:x}, 0x{int.from_bytes(bytes.fromhex(h),"little"):08x});'
                     for a,h in constants.items())
    fixture = fixture.replace('// ORIGINAL_CONSTANTS', seed)
    path.write_text(INCLUDE + after)
    (area / 'src/common/xbe_camera_push.h').write_text((local / 'xbe_camera_push.h').read_text())
    (area / 'src/compat/test_camera_push.cpp').write_text(fixture)

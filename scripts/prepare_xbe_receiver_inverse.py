"""Exact inverse reuse at the pinned B3370 receiver call, preserving other callers."""
from pathlib import Path
import hashlib
import re

INVERSE_SHA256 = '320708bed36e2d31f48749170fa1823664441eb0fbc4bc2e504f752fad3cdf22'
PREFIX_SHA256 = '69ca41ee2a0d9dfe10b802493cb961097295cbf129c55112dac2773c7fd3aad2'
CALL = 'X86_PUSH32(0xB3404u); sub_002D4360(ctx, base);'
HOOK = 'X86_PUSH32(0xB3404u); TrialReceiverInverse(ctx, base, sub_002D4360);'
INCLUDE = '#include "xbe_receiver_inverse.h"\n'


def body(source, name):
    match = re.search(r'X86_FUNC\(__imp__sub_'+name+r'\) \{.*?^\}', source, re.M | re.S)
    assert match, name
    return match[0]


def verify_inverse(source):
    original = body(source, '002D4360')
    assert hashlib.sha256(original.encode()).hexdigest() == INVERSE_SHA256, 'Inverse ABI drift'
    return original


def transform(source):
    clean = source.replace(INCLUDE, '').replace(HOOK, CALL)
    receiver = body(clean, '000B3370')
    assert receiver.count(CALL) == 1
    assert hashlib.sha256(receiver[:receiver.index(CALL)].encode()).hexdigest() == PREFIX_SHA256, 'Receiver input ABI drift'
    changed = clean.replace(receiver, receiver.replace(CALL, HOOK, 1), 1)
    anchor = '#include "x86_recomp_shared.h"\n'
    assert changed.count(anchor) == 1
    changed = changed.replace(anchor, anchor + INCLUDE, 1)
    assert changed.replace(INCLUDE, '').replace(HOOK, CALL) == clean
    return changed


def prepare(area):
    area, local = Path(area), Path(__file__).parent
    original = verify_inverse((area/'generated/game/x86_recomp.78.cpp').read_text())
    path = area/'generated/game/x86_recomp.18.cpp'
    changed = transform(path.read_text())
    test = (local/'test_xbe_receiver_inverse.cpp').read_text()
    assert test.count('// CURRENT_GENERATED_INVERSE') == 1
    test = test.replace('// CURRENT_GENERATED_INVERSE', original.replace('__imp__sub_002D4360', 'Original', 1))
    (area/'src/common/xbe_receiver_inverse.h').write_text((local/'xbe_receiver_inverse.h').read_text())
    (area/'src/compat/test_receiver_inverse.cpp').write_text(test)
    path.write_text(changed)

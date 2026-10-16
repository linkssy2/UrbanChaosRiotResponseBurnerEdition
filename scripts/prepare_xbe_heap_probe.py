"""Stage an opt-in allocator history probe in the private XBE SDK trial only."""
from pathlib import Path


def prepare(area):
    area = Path(area)
    root = Path(__file__).resolve().parents[1]
    source = '''#include "cpu/x86_ops.h"
#include "core/log.h"
#include "kernel/memory.h"
#include "scripts/xbe_input_memory.h"
#include "scripts/xbe_heap_probe.inc"
'''
    (area / 'src/common/xbe_heap_probe.cpp').write_text(source)
    (area / 'src/compat/test_heap_probe.cpp').write_text((root / 'scripts/test_xbe_heap_probe.cpp').read_text())


if __name__ == '__main__':
    prepare(Path(__file__).resolve().parents[1] / 'build/xbe-full-integration-20261014')

"""Stage the project coroutine bridge written for XboxXBERecomp's X86Context ABI."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
AREA = ROOT / 'build/xbe-full-integration-20261014'


def prepare():
    for source, destination in (
        ('xbe_trial_coroutines.cpp', 'src/common/xbe_trial_coroutines.cpp'),
        ('test_xbe_coroutines.cpp', 'src/compat/test_coroutines.cpp'),
    ):
        (AREA / destination).write_bytes((ROOT / 'scripts' / source).read_bytes())


if __name__ == '__main__':
    prepare()

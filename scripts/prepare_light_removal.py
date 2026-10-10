"""Own one exact original Begin body with one pre-state admission seam.

No generated source is changed. --check is a read-only configure/test gate.
"""
import argparse
import hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BODY_SHA256 = "8c6439f5023bfad8ecaca1536de8dfed2014c03524eea3d49402678a4a200dd6"
DECL = "void sub_000B31C0(void)"
SEAM = "loc_000B327B: ;"
INSERTION = "\n    if (uc_light_removal_admit(ebx, MEM32(ebp + 4))) goto loc_000B3358;"


def derive():
    source = (ROOT / "src/recomp/gen/recomp_0007.c").read_text()
    start = source.index(DECL)
    original = source[start:source.index("\n}\n", start) + 3]
    assert hashlib.sha256(original.encode()).hexdigest() == BODY_SHA256
    assert original.count(SEAM) == 1 and "uc_light_removal" not in original
    candidate = original.replace(DECL, "void uc_light_removal_begin(void)")
    candidate = candidate.replace(SEAM, SEAM + INSERTION)
    return ("/* Mechanically derived by scripts/prepare_light_removal.py.\n"
            " * Original Begin body SHA256: " + BODY_SHA256 + "\n"
            " * Sole behavioral seam: B327B -> original false epilogue B3358. */\n" + candidate)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    path = ROOT / "src/light_removal_begin.inc"
    expected = derive()
    if args.check:
        assert path.read_text() == expected, "Light-removal body differs from its exact derivation"
        print("Selected light Begin derivation: PASS")
    else:
        path.write_text(expected)
        print(path)


if __name__ == "__main__":
    main()

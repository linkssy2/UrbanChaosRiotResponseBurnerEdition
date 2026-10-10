"""Apply byte-verified, title-specific discoveries without changing scanner output.

Writes derived analysis metadata, never the XBE or generated C by hand.
Run before lifting; the original scanner database remains available for comparison.
"""
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "third_party/xboxrecomp"))
from tools.recomp import config
from tools.recomp.disasm import Disassembler

REPAIRS = (
    # Timeout failure-menu factory passed to 0x2727E0, captured 2026-09-29.
    (0x00267930, 0x00267AAE,
     "9422a5eac87bca0a5fc0ae5ca01315494ecba82bd135d5854c4eef8a37f3e549"),
    # Live failures immediately around the final hostage marker, 20260916-201116.
    (0x00160840, 0x00160860,
     "3e39596ef376ef6d50145e029049b819b9dcab9f3dfb29cb702583058f708d77"),
    (0x00160860, 0x00160868,
     "541cc225c41dd4213c245f208ebd20c6a2b5374fd8f1df5c9c442b05f9e058ca"),
    (0x00161D10, 0x00161D6E,
     "8ed836aa9caa2a0dfd2d24669b8439d2bdc8da845eac994200b3b2edd3be5323"),
    (0x001AFF10, 0x001AFF6D,
     "9a77a425fb5e57e39b0a329ecf8e85b3c18f5b12c54f18ed9326319611a3b77f"),
    (0x001074B0, 0x00107583,
     "26e1018c69fe6ca3860dedc39af8d5bc04d4ec8a7651d70293e11bebe21b3f2c"),
    (0x0031B960, 0x0031B97C,
     "87912ae896cd658dfca508b33c1e4947188f4fc15439a0ed50be8417d9e202e9"),
    (0x0031F5C0, 0x0031F5FD,
     "85fc6833ce22e9ae2a86eca3eee25d2ea7542043013277042d30da6834cf52bc"),
    (0x000E4CC0, 0x000E4CE1,
     "2ec83f321d7377c1ef94fc914d3ad8b2e8305884e2a16a54374a3210b4c072cd"),
    (0x002B5A20, 0x002B5A41,
     "404adb2957ac38a07d80eb6c4a164b604c0f61dc7dbe757a05b7816644a81734"),
    (0x00057450, 0x00057704,
     "17f4f55ba3020b874b69f93c63defb4d9b8d2aaf3dd5eafeb15e36a68f71f3c3"),
    (0x001DE8B0, 0x001DEA0B,
     "47314d66758a0450bae037ea17c61e24c3ac3ea7736e34da7239cbfbf4a8c012"),
    (0x002A90F0, 0x002A9111,
     "5f54cb47602717bdc7fb0ec5d115bc21904290b32dfa07ca190ea0a40bd50b31"),
    (0x00167C10, 0x00167C2E,
     "ea39b3fb44146462b3b580ae553134958cf4c8768df7f9ad7f68667206c575bf"),
    (0x0031F540, 0x0031F556,
     "32afadbcd609eaa90887cd81197ab6b0942dd2b2dad56601fe202dfa00fcc9db"),
    (0x0001FC00, 0x000200CA,
     "f23eb9f4b0baea687d960e23727840681a5ebf9f3bedb852d8c8d086b76dc71d"),
    (0x00251690, 0x002516B6,
     "f1d1351740500233547f009671c485a79ccdfb7a3433143458b6454111fe8591"),
    (0x001F0330, 0x001F0340,
     "706fc8e88e22d9174d111d10f5c1c220b6dfa59a34b1b1d4e2b5d28810413492"),
    (0x002270C0, 0x002270CE,
     "62bda052575cb114e06d6850dd27af8defb703a303879431b064948740aeb572"),
    (0x000EAD70, 0x000EAD75,
     "329c05551dda10d89cba75f2ca57c0ae77c5e6970a6ef2e955d5f1ebdf7d3869"),
    (0x00178AC0, 0x00178B4C,
     "606263567ce629edfd248a158be55f42c5c1e2def38ab11863aa5ac99d4d3731"),
    (0x002DE995, 0x002DEB08,
     "8440b44046a6f673744bd47087f32e4e67297ea4bc03795bb7505e44cb2bde93"),
    (0x002DF454, 0x002DF519,
     "ba8212c81a35a9d65dc3cfbd304f649bd64616e4e81d9c93b9d65ff2d5447a28"),
)


def repair_metadata(functions, binary):
    entries = {int(f["start"], 16): deepcopy(f) for f in functions}
    decoder = Disassembler()
    for start, end, expected_hash in REPAIRS:
        offset = config.va_to_file_offset(start)
        if offset is None:
            raise ValueError(f"Unmapped repair address: {start:#x}")
        raw = binary[offset:offset + end - start]
        if hashlib.sha256(raw).hexdigest() != expected_hash:
            raise ValueError(f"Original bytes differ at {start:#x}; review required")
        # Mission coroutine has four byte-verified local switch entries and
        # intentionally never returns; it yields through sub_001FB930 instead.
        seeds = {start}
        if start == 0x00178AC0:
            seeds.update((0x00178AE7, 0x00178AFC, 0x00178B11, 0x00178B26))
        instructions = decoder.disassemble_cfg(raw, start, end, seeds)
        addresses = {i.address for i in instructions}
        # Every local branch must have a real instruction and every terminal
        # block must remain inside the reviewed bounds. Fail closed on drift.
        for insn in instructions:
            tail_thunk = ((start == 0x000EAD70 and insn.jump_target == 0x000EA790)
                          or (start == 0x00057450 and insn.jump_target == 0x00238B50))
            if insn.jump_target is not None and insn.jump_target not in addresses and not tail_thunk:
                raise ValueError(f"Unclosed branch at {insn.address:#x}")
            if (not insn.is_terminator and insn.end_address not in addresses):
                raise ValueError(f"Unclosed fallthrough at {insn.address:#x}")
        if not instructions or (start not in (0x000EAD70, 0x00178AC0, 0x00057450)
                                and not any(i.is_ret for i in instructions)):
            raise ValueError(f"No return in reviewed routine {start:#x}")
        entry = entries.setdefault(start, {
            "start": f"0x{start:08X}", "name": f"sub_{start:08X}",
            "section": ".text", "has_prologue": True,
            "called_by": [],
        })
        entry.update(end=f"0x{end:08X}", size=end - start,
                     recover_cfg=True, confidence=1.0,
                     detection_method="reviewed_runtime_failure",
                     num_instructions=len(instructions),
                     calls_to=sorted({f"0x{i.call_target:08X}" for i in instructions
                                      if i.call_target is not None}))
        if start in (0x00267930, 0x001AFF10, 0x001074B0, 0x0031B960, 0x001DE8B0, 0x002A90F0, 0x00167C10, 0x0031F540,
                     0x000E4CC0, 0x002B5A20, 0x00057450, 0x0031F5C0,
                     0x00160840, 0x00160860, 0x00161D10):
            # Actual indirect targets observed in mission sessions, not a
            # guessed conventional prologue or fabricated direct caller.
            entry['runtime_entry'] = True
    # These CRT classification helpers are reached by CALL from a normal
    # frame, not by SEH dispatch. Their [ebp-N] stores belong to that caller.
    for start, end, expected in (
        (0x002DAD88, 0x002DADEF, 'fb9d5ecccaa7b908150ba9881b49d974899e1bfb93716bf26821713ce45b4c6d'),
        (0x002DADEF, 0x002DAE7B, '36abad2126236c939d342e53f16d4fc505dc5216c5adb84fdc851942ba52e9c4'),
    ):
        offset = config.va_to_file_offset(start)
        if offset is None or hashlib.sha256(binary[offset:offset+end-start]).hexdigest() != expected:
            raise ValueError(f'Unverified caller-frame helper {start:#x}')
        entries[start]['frame_source'] = 'caller'
    return [entries[a] for a in sorted(entries)]


def main():
    xbe = ROOT / "game_files/Default.xbe"
    config.configure_from_xbe(str(xbe))
    functions = json.loads((ROOT / "analysis/disasm/functions.json").read_text())
    result = repair_metadata(functions, xbe.read_bytes())
    output = ROOT / "analysis/disasm/functions_repaired.json"
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"Verified {len(REPAIRS)} routine repairs; wrote {output}")


if __name__ == "__main__":
    main()

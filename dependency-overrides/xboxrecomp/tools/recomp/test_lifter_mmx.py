import unittest

from .disasm import Instruction, Operand
from .lifter import Lifter


def _insn(mnemonic, op_str, operands):
    instruction = Instruction(0, 4, mnemonic, op_str, "")
    instruction.operands = operands
    return instruction


def _mm(name):
    return Operand(type="reg", reg=name)


def _xmm(name):
    return Operand(type="reg", reg=name)


def _gpr(name):
    return Operand(type="reg", reg=name)


def _mem(base=None, disp=0, size=8):
    return Operand(type="mem", mem_base=base, mem_disp=disp, mem_size=size)


def _imm(value):
    return Operand(type="imm", imm=value)


class MmxLifterTest(unittest.TestCase):
    """Xbox XMV's WMV2 decoder is mostly MMX.  A bare comment for any of
    these instructions leaves its decoded luma/chroma surfaces unchanged."""

    def lift(self, mnemonic, op_str, operands):
        return Lifter().lift_instruction(_insn(mnemonic, op_str, operands))

    def test_movq_loads_and_stores_all_eight_bytes(self):
        self.assertEqual(
            self.lift("movq", "mm0, qword ptr [esi]",
                      [_mm("mm0"), _mem("esi")]),
            ["mm0 = MMX_MEM(esi); /* movq */"],
        )
        self.assertEqual(
            self.lift("movq", "qword ptr [edi + 8], mm3",
                      [_mem("edi", 8), _mm("mm3")]),
            ["MMX_STORE(edi + 8, mm3); /* movq */"],
        )

    def test_lane_arithmetic_uses_mmx_helpers(self):
        cases = {
            "paddw": "MMX_PADDW",
            "psubd": "MMX_PSUBD",
            "pmullw": "MMX_PMULLW",
            "pmaddwd": "MMX_PMADDWD",
            "pavgb": "MMX_PAVGB",
        }
        for mnemonic, helper in cases.items():
            self.assertEqual(
                self.lift(mnemonic, "mm2, mm5", [_mm("mm2"), _mm("mm5")]),
                [f"mm2 = {helper}(mm2, mm5); /* {mnemonic} */"],
            )

    def test_unpack_and_saturating_pack_are_not_comments(self):
        self.assertEqual(
            self.lift("punpcklbw", "mm0, mm4", [_mm("mm0"), _mm("mm4")]),
            ["mm0 = MMX_PUNPCKLBW(mm0, mm4); /* punpcklbw */"],
        )
        self.assertEqual(
            self.lift("packuswb", "mm0, mm1", [_mm("mm0"), _mm("mm1")]),
            ["mm0 = MMX_PACKUSWB(mm0, mm1); /* packuswb */"],
        )

    def test_immediate_and_register_shift_counts(self):
        self.assertEqual(
            self.lift("psrad", "mm5, 8", [_mm("mm5"), _imm(8)]),
            ["mm5 = MMX_PSRAD(mm5, 8); /* psrad */"],
        )
        self.assertEqual(
            self.lift("psllq", "mm3, mm6", [_mm("mm3"), _mm("mm6")]),
            ["mm3 = MMX_PSLLQ(mm3, mm6); /* psllq */"],
        )

    def test_shuffle_and_move_mask(self):
        self.assertEqual(
            self.lift("pshufw", "mm1, mm0, 0xd8",
                      [_mm("mm1"), _mm("mm0"), _imm(0xD8)]),
            ["mm1 = MMX_PSHUFW(mm0, 0xD8); /* pshufw */"],
        )
        self.assertEqual(
            self.lift("pmovmskb", "edx, mm7", [_gpr("edx"), _mm("mm7")]),
            ["edx = MMX_PMOVMSKB(mm7); /* pmovmskb */"],
        )

    def test_mmx_float_conversions_bridge_the_low_two_lanes(self):
        self.assertEqual(
            self.lift("cvtpi2ps", "xmm1, mm0", [_xmm("xmm1"), _mm("mm0")]),
            ["xmm1 = MMX_CVTPI2PS(xmm1, mm0); /* cvtpi2ps */"],
        )
        self.assertEqual(
            self.lift("cvtps2pi", "mm2, xmm3", [_mm("mm2"), _xmm("xmm3")]),
            ["mm2 = MMX_CVTPS2PI(xmm3); /* cvtps2pi */"],
        )

    def test_all_xmv_mmx_mnemonics_emit_statements(self):
        binary = (
            "paddb paddw paddd psubb psubw psubd pmullw pmaddwd "
            "pand pandn por pxor pcmpeqb pcmpeqw pcmpeqd "
            "pcmpgtb pcmpgtw pcmpgtd punpcklbw punpcklwd punpckldq "
            "punpckhbw punpckhwd punpckhdq packssdw packuswb pavgb"
        ).split()
        for mnemonic in binary:
            lifted = self.lift(mnemonic, "mm0, mm1", [_mm("mm0"), _mm("mm1")])
            self.assertEqual(len(lifted), 1, mnemonic)
            self.assertFalse(lifted[0].lstrip().startswith("/*"),
                             (mnemonic, lifted[0]))


if __name__ == "__main__":
    unittest.main()

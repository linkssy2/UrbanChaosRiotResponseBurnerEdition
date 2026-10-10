import unittest

from .disasm import Instruction, Operand
from .lifter import Lifter


class BitScanLifterTest(unittest.TestCase):
    def test_bsf_uses_forward_scan_and_preserves_destination_on_zero(self):
        insn = Instruction(0, 3, "bsf", "eax, ecx", "0fbcc1")
        insn.operands = [
            Operand(type="reg", reg="eax"),
            Operand(type="reg", reg="ecx"),
        ]

        generated = "\n".join(Lifter().lift_instruction(insn))

        self.assertIn("uint32_t _scan = ((uint32_t)(ecx))", generated)
        self.assertIn("if (_scan != 0)", generated)
        self.assertIn("eax = RECOMP_BSF32(_scan);", generated)

    def test_bsr_uses_reverse_scan(self):
        insn = Instruction(0, 3, "bsr", "edx, eax", "0fbdd0")
        insn.operands = [
            Operand(type="reg", reg="edx"),
            Operand(type="reg", reg="eax"),
        ]

        generated = "\n".join(Lifter().lift_instruction(insn))

        self.assertIn("edx = RECOMP_BSR32(_scan);", generated)

    def test_word_source_is_masked_before_scan(self):
        insn = Instruction(0, 4, "bsf", "ax, word ptr [esi]", "660fbc06")
        insn.operands = [
            Operand(type="reg", reg="ax"),
            Operand(type="mem", mem_base="esi", mem_size=2),
        ]

        generated = "\n".join(Lifter().lift_instruction(insn))

        self.assertIn("MEM16(esi)", generated)
        self.assertIn("& 0xFFFFu", generated)
        self.assertIn("SET_LO16(eax, RECOMP_BSF32(_scan));", generated)


if __name__ == "__main__":
    unittest.main()

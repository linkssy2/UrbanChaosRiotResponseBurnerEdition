import unittest
from .disasm import Instruction, Operand
from .lifter import Lifter

class MemoryReadPollTest(unittest.TestCase):
    def test_selected_load_and_negative_witnesses(self):
        insn=Instruction(0x3E9697,2,"mov","ecx, dword ptr [edx]","8b0a")
        insn.operands=[Operand(type="reg",reg="ecx"),Operand(type="mem",mem_base="edx",mem_size=4)]
        self.assertEqual(Lifter().lift_instruction(insn),["ecx = MEM32(edx);"])
        lifter=Lifter(memory_read_polls={0x3E9697:"8b0a"})
        self.assertEqual(lifter.lift_instruction(insn),["RECOMP_MEMORY_READ_POLL(edx);","ecx = MEM32(edx);"])
        insn.bytes_hex="8b0b"
        with self.assertRaises(ValueError):lifter.lift_instruction(insn)
        insn.bytes_hex="8b0a";insn.operands[1].mem_size=2
        with self.assertRaises(ValueError):lifter.lift_instruction(insn)
        insn.mnemonic="add"
        with self.assertRaises(ValueError):lifter.lift_instruction(insn)

if __name__=="__main__":unittest.main()

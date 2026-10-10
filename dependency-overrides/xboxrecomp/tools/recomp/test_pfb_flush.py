import unittest
from .disasm import Instruction, Operand
from .lifter import Lifter

class PfbFlushTest(unittest.TestCase):
    def test_progress_poll_precedes_original_register_read(self):
        insn=Instruction(0x3E92C0,6,"mov","eax, dword ptr [edx + 0x400b10]","")
        insn.operands=[Operand(type="reg",reg="eax"),Operand(type="mem",mem_base="edx",mem_disp=0x400B10,mem_size=4)]
        self.assertEqual(Lifter().lift_instruction(insn),[
            "RECOMP_GPU_PROGRESS_READ(edx + 0x400B10);",
            "eax = MEM32(edx + 0x400B10);"])
        insn.operands[1].mem_disp=0x400B14
        self.assertEqual(len(Lifter().lift_instruction(insn)),1)

    def test_original_mov_store_then_notification(self):
        # Original003E93B3: 89 88 10 04 10 00, mov [eax+100410],ecx.
        insn=Instruction(0x3E93B3,6,"mov","dword ptr [eax + 0x100410], ecx","")
        insn.operands=[Operand(type="mem",mem_base="eax",mem_disp=0x100410,mem_size=4),Operand(type="reg",reg="ecx")]
        self.assertEqual(Lifter().lift_instruction(insn),[
            "MEM32(eax + 0x100410) = ecx;",
            "RECOMP_PFB_FLUSH_WRITE(eax + 0x100410);"])
        insn.operands[0].mem_disp=0x100414
        self.assertEqual(len(Lifter().lift_instruction(insn)),1)
        insn.operands[0].mem_disp=0x100410
        insn.operands[0].mem_size=2
        self.assertEqual(len(Lifter().lift_instruction(insn)),1)

if __name__ == "__main__":unittest.main()

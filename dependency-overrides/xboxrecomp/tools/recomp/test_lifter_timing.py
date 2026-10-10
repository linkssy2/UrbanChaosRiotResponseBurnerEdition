import unittest

from .disasm import Instruction
from .lifter import Lifter, advance_flag_state


class TimingInstructionLifterTest(unittest.TestCase):
    def test_rdtsc_reads_scaled_runtime_counter_into_edx_eax(self):
        insn = Instruction(0, 2, "rdtsc", "", "0f31")

        generated = "\n".join(Lifter().lift_instruction(insn))

        self.assertIn("uint64_t _tsc = recomp_read_tsc()", generated)
        self.assertIn("eax = (uint32_t)_tsc", generated)
        self.assertIn("edx = (uint32_t)(_tsc >> 32)", generated)

    def test_rdtsc_preserves_eflags_provenance(self):
        insn = Instruction(0, 2, "rdtsc", "", "0f31")
        state = ("cmp", [])

        self.assertEqual(advance_flag_state(insn, state), state)


if __name__ == "__main__":
    unittest.main()

"""Explicit CFG bounds must skip interleaved callable cleanup helpers."""
import unittest
from .translator import FunctionTranslator


class ReviewedCfgTest(unittest.TestCase):
    def test_branch_over_funclet_is_local_not_a_stub_or_tail_call(self):
        # push ebp; mov ebp,esp; jmp continuation; [cleanup: int3; ret];
        # continuation: mov eax,42; pop ebp; ret
        raw = bytes.fromhex("55 8b ec eb 02 cc c3 b8 2a 00 00 00 5d c3")
        info = {"name": "sub_00001000", "end": 0x100E, "recover_cfg": True}
        tr = FunctionTranslator(b"", {0x1000: info})
        tr._read_func_bytes = lambda start, end: raw[start-0x1000:end-0x1000]
        code = tr.translate_function(0x1000, info)
        self.assertIn("goto loc_00001007", code)
        self.assertIn("eax = 0x2A", code)
        self.assertIn("POP32(esp, ebp)", code)
        self.assertNotIn("loc_00001005", code)
        self.assertNotIn("sub_00001007()", code)
        self.assertEqual(code.count("/* ret */"), 1)

    def test_reviewed_empty_decode_fails_closed(self):
        info = {"end": 0x1001, "recover_cfg": True}
        tr = FunctionTranslator(b"", {0x1000: info})
        tr._read_func_bytes = lambda start, end: b""
        with self.assertRaises(ValueError):
            tr.translate_function(0x1000, info)

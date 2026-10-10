"""Translate a real push/mov/pop/ret sequence and check frame inheritance."""
import unittest
from .translator import FunctionTranslator


class FrameEntryTest(unittest.TestCase):
    def test_prologue_saves_inherited_frame_pointer(self):
        raw = bytes.fromhex("55 8b ec 5d c3")
        info = {"name": "sub_00001000", "end": 0x1005}
        translator = FunctionTranslator(b"", {0x1000: info})
        translator._read_func_bytes = lambda start, end: raw
        code = translator.translate_function(0x1000, info)
        self.assertIn("uint32_t ebp = g_ebp;", code)
        self.assertLess(code.index("uint32_t ebp = g_ebp;"),
                        code.index("PUSH32(esp, ebp)"))
        self.assertIn("POP32(esp, ebp)", code)


if __name__ == "__main__":
    unittest.main()

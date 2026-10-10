"""MM0 is a guest return channel, not an uninitialized host local."""
import unittest
from .translator import FunctionTranslator


class MmxCallStateTest(unittest.TestCase):
    def test_original_packing_helper_preserves_mmx_return(self):
        raw = bytes.fromhex('0f28c8 0f12c8 0f2dc1 0f2dc8 0f6bc8 0f7fc8 c3')
        info = {'name': 'sub_00099960', 'end': 0x99960 + len(raw)}
        translator = FunctionTranslator(b'', {0x99960: info})
        translator._read_func_bytes = lambda start, end: raw
        code = translator.translate_function(0x99960, info)
        self.assertIn('mm0 = mm1;', code)
        self.assertIn('MMX_PACKSSDW(mm1, mm0)', code)
        self.assertNotIn('uint64_t mm', code)

    def test_read_only_mmx_consumer_does_not_shadow_state(self):
        raw = bytes.fromhex('0f7fc1 0f67c8 c3')
        info = {'name': 'sub_000999A7', 'end': 0x999A7 + len(raw)}
        translator = FunctionTranslator(b'', {0x999A7: info})
        translator._read_func_bytes = lambda start, end: raw
        code = translator.translate_function(0x999A7, info)
        self.assertIn('mm1 = mm0;', code)
        self.assertIn('MMX_PACKUSWB(mm1, mm0)', code)
        self.assertNotIn('uint64_t mm', code)


if __name__ == '__main__':
    unittest.main()

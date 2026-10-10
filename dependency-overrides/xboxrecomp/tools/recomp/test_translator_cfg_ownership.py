"""Regression tests for recovering scanner-split control-flow graphs."""

from types import SimpleNamespace
import unittest

from .translator import FunctionTranslator


class CfgOwnershipTest(unittest.TestCase):
    def test_observed_indirect_entry_is_not_absorbed_as_weak_seed(self):
        translator = FunctionTranslator.__new__(FunctionTranslator)
        translator.func_db = {
            0x1000: dict(end=0x1010, section='.text', has_prologue=True),
            0x1010: dict(end=0x1020, section='.text', runtime_entry=True),
            0x1100: dict(end=0x1110, section='.text', has_prologue=True),
        }
        translator.owned_function_starts = set()
        translator._recovered_cfg = {}
        translator._ownership_ready = False
        translator._read_func_bytes = lambda *args: self.fail('No weak seed to recover')
        translator.discover_cfg_ownership()
        self.assertEqual(translator.owned_function_starts, set())
        self.assertFalse(FunctionTranslator._is_strong_entry({}))
        self.assertTrue(FunctionTranslator._is_strong_entry({'runtime_entry': True}))

    def test_adjacent_weak_fallthrough_is_owned_by_preceding_function(self):
        """A seed cannot start a new function in a live fall-through stream."""
        translator = FunctionTranslator.__new__(FunctionTranslator)
        translator.func_db = {
            0x1000: {
                "start": "0x00001000", "end": 0x1010, "section": ".text",
                "has_prologue": True, "called_by": ["0x00000800"],
            },
            0x1010: {
                "start": "0x00001010", "end": 0x1020, "section": ".text",
                "has_prologue": False, "called_by": [],
            },
            0x1100: {
                "start": "0x00001100", "end": 0x1110, "section": ".text",
                "has_prologue": True, "called_by": [],
            },
        }
        translator.owned_function_starts = set()
        translator._recovered_cfg = {}
        translator._ownership_ready = False
        translator.xbe_data = b""
        translator._read_func_bytes = lambda start, end: b"\x90"
        translator.disasm = SimpleNamespace(
            disassemble_function=lambda raw, start, end: [
                SimpleNamespace(
                    address=0x1000, end_address=0x1010,
                    is_cond_jump=False, is_jump=False,
                    is_terminator=False, jump_target=None,
                )
            ]
        )
        recovered = [
            SimpleNamespace(address=0x1000, end_address=0x1010),
            SimpleNamespace(address=0x1010, end_address=0x1020),
        ]
        translator._recover_cfg = (
            lambda start, upper, bridges, stops: (recovered, {}, set())
        )

        translator.discover_cfg_ownership()

        self.assertEqual(translator.owned_function_starts, {0x1010})
        self.assertEqual(translator._recovered_cfg[0x1000]["end"], 0x1020)

    def test_weak_indirect_entry_can_own_later_weak_splits(self):
        """A vtable-only root may be weak even though it owns the whole CFG."""
        translator = FunctionTranslator.__new__(FunctionTranslator)
        translator.func_db = {
            0x1000: {
                "start": "0x00001000", "end": 0x1010, "section": ".text",
                "has_prologue": False, "called_by": [],
            },
            0x1010: {
                "start": "0x00001010", "end": 0x1020, "section": ".text",
                "has_prologue": False, "called_by": [],
            },
            0x1020: {
                "start": "0x00001020", "end": 0x1030, "section": ".text",
                "has_prologue": False, "called_by": [],
            },
            0x1100: {
                "start": "0x00001100", "end": 0x1110, "section": ".text",
                "has_prologue": True, "called_by": [],
            },
        }
        translator.owned_function_starts = set()
        translator._recovered_cfg = {}
        translator._ownership_ready = False
        translator.xbe_data = b""
        translator._read_func_bytes = lambda start, end: b"\x90"
        translator.disasm = SimpleNamespace(
            disassemble_function=lambda raw, start, end: [
                SimpleNamespace(is_cond_jump=True, jump_target=0x1020)
            ]
        )
        recovered = [
            SimpleNamespace(address=0x1000, end_address=0x1001),
            SimpleNamespace(address=0x1010, end_address=0x1011),
            SimpleNamespace(address=0x1020, end_address=0x1030),
        ]
        translator._recover_cfg = (
            lambda start, upper, bridges, stops: (recovered, {}, set())
        )

        translator.discover_cfg_ownership()

        self.assertEqual(translator.owned_function_starts, {0x1010, 0x1020})
        self.assertEqual(translator._recovered_cfg[0x1000]["end"], 0x1030)

    def test_terminal_jump_across_padding_recovers_split_switch_body(self):
        """An adjacent loop header after padding remains part of its owner."""
        translator = FunctionTranslator.__new__(FunctionTranslator)
        translator.func_db = {
            0x2000: {
                "start": "0x00002000", "end": 0x201A, "section": ".text",
                "has_prologue": False, "called_by": ["0x00001000"],
            },
            0x2020: {
                "start": "0x00002020", "end": 0x2030, "section": ".text",
                "has_prologue": False, "called_by": [],
            },
            0x2040: {
                "start": "0x00002040", "end": 0x2050, "section": ".text",
                "has_prologue": False, "called_by": [],
            },
            0x2100: {
                "start": "0x00002100", "end": 0x2110, "section": ".text",
                "has_prologue": True, "called_by": [],
            },
        }
        translator.owned_function_starts = set()
        translator._recovered_cfg = {}
        translator._ownership_ready = False
        translator.xbe_data = b""
        translator._read_func_bytes = lambda start, end: b"\x90"
        translator.disasm = SimpleNamespace(
            disassemble_function=lambda raw, start, end: [
                SimpleNamespace(
                    is_cond_jump=False, is_jump=True,
                    jump_target=0x2020, end_address=0x201A,
                )
            ]
        )
        recovered = [
            SimpleNamespace(address=0x2000, end_address=0x2001),
            SimpleNamespace(address=0x2020, end_address=0x2021),
            SimpleNamespace(address=0x2040, end_address=0x2050),
        ]
        observed_bridges = []

        def recover(start, upper, bridges, stops):
            observed_bridges.append(set(bridges))
            return recovered, {0x2080: [0x2040]}, set()

        translator._recover_cfg = recover

        translator.discover_cfg_ownership()

        self.assertEqual(observed_bridges, [{0x2020}])
        self.assertEqual(translator.owned_function_starts, {0x2020, 0x2040})
        self.assertEqual(translator._recovered_cfg[0x2000]["end"], 0x2050)

    def test_local_indexed_dispatch_owns_its_case_fragments(self):
        """Jump-table cases share the dispatcher's frame and return path."""
        translator = FunctionTranslator.__new__(FunctionTranslator)
        translator.func_db = {
            0x3000: {
                "start": "0x00003000", "end": 0x3010, "section": ".text",
                "has_prologue": False, "called_by": ["0x00001000"],
            },
            0x3020: {
                "start": "0x00003020", "end": 0x3030, "section": ".text",
                "has_prologue": False, "called_by": [],
            },
            0x3040: {
                "start": "0x00003040", "end": 0x3050, "section": ".text",
                "has_prologue": False, "called_by": [],
            },
            0x3100: {
                "start": "0x00003100", "end": 0x3110, "section": ".text",
                "has_prologue": True, "called_by": [],
            },
        }
        translator.owned_function_starts = set()
        translator._recovered_cfg = {}
        translator._ownership_ready = False
        translator.xbe_data = b""
        translator._read_func_bytes = lambda start, end: b"\x90"
        operand = SimpleNamespace(
            type="mem", mem_index="eax", mem_base=None, mem_disp=0x3080,
        )
        translator.disasm = SimpleNamespace(
            disassemble_function=lambda raw, start, end: [
                SimpleNamespace(
                    is_cond_jump=False, is_jump=True, jump_target=None,
                    end_address=0x3010, operands=[operand],
                )
            ]
        )
        translator._read_local_jump_table = (
            lambda table, lower, upper: [0x3020, 0x3040]
        )
        recovered = [
            SimpleNamespace(address=0x3000, end_address=0x3001),
            SimpleNamespace(address=0x3020, end_address=0x3021),
            SimpleNamespace(address=0x3040, end_address=0x3050),
        ]
        observed_bridges = []

        def recover(start, upper, bridges, stops):
            observed_bridges.append(set(bridges))
            return recovered, {0x3080: [0x3020, 0x3040]}, set()

        translator._recover_cfg = recover

        translator.discover_cfg_ownership()

        self.assertEqual(observed_bridges, [{0x3020, 0x3040}])
        self.assertEqual(translator.owned_function_starts, {0x3020, 0x3040})
        self.assertEqual(translator._recovered_cfg[0x3000]["end"], 0x3050)


if __name__ == "__main__":
    unittest.main()

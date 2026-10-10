"""Regression tests for manual overrides at CFG-owned alternate entries."""

from types import SimpleNamespace
import os
import tempfile
import unittest

from .translator import BatchTranslator


class ManualOverrideTest(unittest.TestCase):
    def test_wrapped_body_keeps_public_direct_calls_and_dispatch(self):
        batch = BatchTranslator.__new__(BatchTranslator)
        batch.func_db = {0x1000: {"name": "sub_00001000", "end": 0x1010}}
        batch.translator = SimpleNamespace(
            owned_function_starts=set(),
            lifter=SimpleNamespace(referenced_calls={0x1000: "sub_00001000"}),
            translate_function=lambda addr, info:
                f"void {info['name']}(void) {{ sub_00001000(); }}\n",
        )
        batch._make_declaration = lambda addr, name: f"void {name}(void)"
        with tempfile.TemporaryDirectory() as output_dir:
            stats = batch.translate_batch_split(
                list(batch.func_db.items()), output_dir, wrapped={0x1000})
            self.assertEqual(stats["unresolved_stubs"], 0)
            self.assertEqual(batch.func_db[0x1000]["name"], "sub_00001000")
            for filename, expected in [
                ("recomp_0000.c", "void sub_00001000_gen(void) { sub_00001000(); }"),
                ("recomp_funcs.h", "void sub_00001000(void);"),
                ("recomp_funcs.h", "void sub_00001000_gen(void);"),
                ("recomp_dispatch.c", "(recomp_func_t)sub_00001000 }"),
            ]:
                with open(os.path.join(output_dir, filename), encoding="utf-8") as source:
                    self.assertIn(expected, source.read())

    def test_owned_manual_entry_suppresses_unresolved_stub(self):
        batch = BatchTranslator.__new__(BatchTranslator)
        batch.func_db = {
            0x1010: {
                "start": "0x00001010",
                "end": 0x1020,
                "name": "sub_00001010",
            },
        }
        batch.translator = SimpleNamespace(
            owned_function_starts={0x1010},
            lifter=SimpleNamespace(
                referenced_calls={0x1010: "sub_00001010"},
            ),
        )

        with tempfile.TemporaryDirectory() as output_dir:
            stats = batch.translate_batch_split(
                [(0x1010, batch.func_db[0x1010])],
                output_dir=output_dir,
                manual={0x1010},
            )

            self.assertEqual(stats["unresolved_stubs"], 0)
            self.assertEqual(stats["manual_functions"], 1)
            self.assertFalse(os.path.exists(
                os.path.join(output_dir, "recomp_stubs_unresolved.c")))
            with open(os.path.join(output_dir, "recomp_funcs.h"),
                      encoding="utf-8") as header:
                self.assertIn("void sub_00001010(void);", header.read())


if __name__ == "__main__":
    unittest.main()

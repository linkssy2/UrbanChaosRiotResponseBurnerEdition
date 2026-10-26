"""Bounded branch census checks; no game execution or compilation."""
from pathlib import Path
import struct
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parent))
import coop_progression_assets as c


def packed(*values):
    return struct.pack("<" + "I" * len(values), *values).hex()


class BranchSelection(unittest.TestCase):
    def test_original_strict_bounds_fallback_first_match(self):
        # The second row has no integer strictly between its bounds. The last
        # row is shadowed by the first; neither may add a speculative gate.
        rows = [(0, 0, 99), (1, 50, 51), (2, 1, 98)]
        self.assertEqual(c.possible_scripts([(rows, 0)], 0, 3), [0])
        self.assertEqual(c.possible_scripts([([(0, 0, 49), (1, 50, 99)], 0)], 0, 2), [0, 1])
        self.assertEqual(c.possible_scripts([([(0xffffffff, 0, 0), (1, 0xffffffff, 100)], 0)], 0, 2), [1])
        self.assertEqual(c.possible_scripts([([(0xffffffff, 0, 99)], 0)], 0, 2), [])
        with self.assertRaises(c.DecodeError):
            c.possible_scripts([([(4, 0, 99)], 0)], 0, 2)

    def test_only_satisfiable_flag_conditions(self):
        entity = dict(payload=packed(0, 0xffffffff), names=[("test", "ffffffff")], records2=[],
                      records=[("", 0, packed(0, 0, 99), 0),
                               (packed(3, 0, 3, 1), 0, packed(0, 0, 99), 0),
                               (packed(4, 1), 0, packed(0, 0, 99), 0)])
        selected = c.controller_scripts(entity, 5)
        self.assertEqual(len(selected), 1)
        self.assertEqual(selected[0][1]["record"], 2)
        with self.assertRaises(c.DecodeError):
            c.controller_scripts(entity, 4)

    def test_command_blob_and_termination(self):
        blob = struct.pack("<3I", 22, 100, 0xffffffff)
        data = struct.pack("<2I", 42, len(blob)) + blob + struct.pack("<I", 0xffffffff)
        self.assertEqual([x["opcode"] for x in c.decode(data)], [42])
        for data in (b"", b"\x16\x00\x00\x00", struct.pack("<2I", 0xffffffff, 22)):
            with self.assertRaises(c.DecodeError):
                c.decode(data)

    def test_local_campaign_census(self):
        area = Path(__file__).resolve().parents[1] / "build/xbe-full-integration-20261014"
        result = c.build(area)
        self.assertEqual((len(result["gates"]), len(result["proofs"])), (43, 45))
        self.assertFalse(any(x["unproved"] for x in result["report"]["levels"]))
        levels = {x["file"]: x for x in result["report"]["levels"]}
        self.assertEqual(set(levels), {f"{i}-01.ent" for i in c.CAMPAIGN_LEVELS})
        self.assertEqual(sum(x["enter_groups"] for x in levels.values()), 524)
        for level in levels.values():
            self.assertEqual(len(level["gates"]) + len(level["excluded"]), level["enter_groups"])
            self.assertTrue(all(g["reasons"] for g in level["gates"]))
        hope = {x["volume_ordinal"] for x in levels["12-01.ent"]["gates"]}
        self.assertTrue({39, 43, 66} <= hope)
        self.assertNotIn(30, hope)  # Original shield pickup remains independent.
        # Birth of a door does not close a route. Keep population events running
        # when players cross separately, including the garage vehicles/doors.
        population = {"4-01.ent": {35}, "5-01.ent": {32, 705}, "6-01.ent": {8, 34},
                      "11-01.ent": {6}, "12-01.ent": {29, 37, 51}, "13-01.ent": {36, 38}}
        for file, ordinals in population.items():
            self.assertTrue(ordinals <= {g["volume_ordinal"] for g in levels[file]["excluded"]})
        garage = next(x for x in levels["12-01.ent"]["excluded"] if x["volume_ordinal"] == 29)
        self.assertEqual(garage["scripts"], ["create walkway action"])
        self.assertTrue({41, 58, 67} <= hope)  # Explicit door/cutscene/checkpoint operations remain gated.
        # Unlocks allow passage; retirements that also remove ladders/areas must
        # still wait. In particular 12:43 also removes a RestartPoint.
        for file, ordinals in {"6-01.ent": {21}, "9-01.ent": {18, 25}}.items():
            self.assertTrue(ordinals <= {g["volume_ordinal"] for g in levels[file]["excluded"]})
        for file, ordinals in {"6-01.ent": {16, 23, 24, 31, 33, 36}, "11-01.ent": {16},
                               "12-01.ent": {41, 43, 58, 67}, "13-01.ent": {7}}.items():
            self.assertTrue(ordinals <= {g["volume_ordinal"] for g in levels[file]["gates"]})
        quay = next(x for x in levels["1-01.ent"]["excluded"] if x["volume_ordinal"] == 252)
        self.assertEqual(len(quay["scripts"]), 2)
        alarm = next(x for x in levels["3-01.ent"]["excluded"] if x["volume_ordinal"] == 7)
        self.assertTrue(any(x.get("selection", {}).get("kind") == "condition" for x in alarm["examined_paths"]))


if __name__ == "__main__":
    unittest.main()

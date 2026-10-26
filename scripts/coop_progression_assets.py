"""Derive co-op rendezvous gates from exact, local campaign ENT scripts.

This is a build-time census, not a replacement script interpreter. Only PLAYER
volume-enter groups whose decoded commands operate a checkpoint, install a path
barrier/teleport/cinematic, or remove a checkpoint are selected. ScriptController
startup and satisfiable condition groups follow the original bounded selector.
Ordinary pickups, combat waves and hazards are not gates. Unproved paths are
reported; the runtime leaves every original condition and command intact.
"""
from pathlib import Path
import ast
import hashlib
import json
import struct
import sys

CAMPAIGN_LEVELS = (1, 2, 3, 4, 5, 6, 8, 9, 11, 12, 13, 21, 22, 23, 24, 25, 26)
ONE_ARGUMENT = frozenset((5, 6, 10, 11, 16, 17, 21, 22, 23, 43, 52, 53, 60, 61))
NO_ARGUMENT = frozenset((18, 19, 20, 47))
# Spawning a door also populates scenes; only an explicit door operation proves
# a rendezvous. Delaying a population script can leave an entire room empty.
ENSURE_PROGRESS = frozenset(("SolidCollisionBlock", "PlayerTeleport",
                              "SimpleCutscene", "SimpleCameraCutscene"))
# Opcode 23 enables opening; 43 locks. Retirement roots also remove ladders and
# earlier areas, so keep their rendezvous until that cleanup is proved safe.
DOOR_PROGRESS_OPERATIONS = frozenset((16, 17, 43))
# The original selector, condition scheduler and controller startup are the
# contracts used below. The runtime preparer separately pins command decoding
# and the custom-trigger append routine.
BRANCH_PINS = (
    (0x2d7fcc, 34, "10496cd5d5b872ffc9aee2d8eb2f2714d49a5aa3c619bdc8a26f160a3a40815d"),
    (0x216f20, 85, "2392d6a5f6c6018f8c691e40ca327cc28d62f6ff6d5331ceb7fbb72474af9993"),
    (0x217840, 572, "fd2e9f8b7ad79f248cf18f35d7cfe7becf3ca3ddf47b614542d79fa44dfed712"),
    (0x120b90, 194, "bfd240efa16964424920eebd60e8a99ca7a17d62b549d8f2411c3463d552d084"),
    (0x120d50, 140, "148a02846b76e699b4674a6c7c1d99870d77eb207af326784b4a8c72785a75a0"),
    # Door unlock/lock commands, retirement and the original can-open predicate.
    (0x217ee3, 98, "ca2d8a09e58ac531fc3675b8d79a8bcac66e1cb1f3ff9c4af71969fe6bb92c6f"),
    (0x217f45, 94, "7b117988ecd0a4185c582e712831e96b0ee21a3efc095c57c803d1279b7fff34"),
    (0x217e84, 95, "84a5d3652c1aeade98c2d1e7fa28213c2ce5f9a80c87d994d7adbe69f675e71a"),
    (0x20a140, 38, "90ace56f24034b7e7014d411b752ed9e0e22bb23243aa13b80d243a7c6bcdb22"),
    (0x1b05d0, 89, "c813db155dd2c02a949e3dd507777c684add431ddb38a90515b8fe49b718fd9b"),
)


class DecodeError(ValueError):
    pass


def words(data):
    if len(data) % 4:
        raise DecodeError("unaligned integer list")
    return list(struct.unpack("<" + "I" * (len(data) // 4), data))


def fnv1a64(data):
    value = 0xcbf29ce484222325
    for byte in data:
        value = ((value ^ byte) * 0x100000001b3) & 0xffffffffffffffff
    return value


def decode(data):
    """Walk the original command widths; unknown commands fail at their offset."""
    at = 0
    result = []

    def take(size):
        nonlocal at
        if size < 0 or size > len(data) - at:
            raise DecodeError(f"truncated command at {at:#x}")
        value = data[at:at + size]
        at += size
        return value

    def integer():
        return struct.unpack("<I", take(4))[0]

    def blob():
        return take(integer())

    while at < len(data):
        offset = at
        op = integer()
        command = {"offset": offset, "opcode": op}
        if op == 0xffffffff:
            if at != len(data):
                raise DecodeError(f"bytes after script end at {at:#x}")
            return result
        if op in ONE_ARGUMENT:
            command["argument"] = integer()
        elif op in NO_ARGUMENT:
            pass
        elif op in (25, 26, 27):
            command["boolean"] = integer()
            command["strings"] = [blob().hex(), blob().hex()]
            if op == 26:
                command["entity"] = integer()
            elif op == 27:
                command["position"] = take(12).hex()
        elif op in (42, 57):
            command["blob"] = blob().hex()
        else:
            raise DecodeError(f"unproved opcode {op} at {offset:#x}")
        result.append(command)
    raise DecodeError("missing script end")


def trigger_groups(data):
    values = words(data)
    at = 0

    def take(count):
        nonlocal at
        if count < 0 or count > len(values) - at:
            raise DecodeError("truncated script records")
        result = values[at:at + count]
        at += count
        return result

    conditions = take(1)[0]
    for _ in range(conditions):
        take(take(1)[0] * 2)
        take(1)
        take(take(1)[0] * 3)
        take(1)
    groups = []
    for _ in range(take(1)[0]):
        rows = take(take(1)[0] * 3)
        groups.append((list(zip(rows[::3], rows[1::3], rows[2::3])), take(1)[0]))
    if at != len(values):
        raise DecodeError("trailing script records")
    return conditions, groups


def bindings(data):
    values = words(data)
    if not values or len(values) != 1 + 2 * values[0]:
        raise DecodeError("invalid trigger event bindings")
    return list(zip(values[1::2], values[2::2]))


def entity_groups(entity):
    result = []
    for raw, priority in entity["records2"]:
        values = words(bytes.fromhex(raw))
        if len(values) % 3:
            raise DecodeError("invalid entity group rows")
        result.append((list(zip(values[::3], values[1::3], values[2::3])), priority))
    return result


def possible_scripts(groups, index, count):
    if index >= len(groups):
        raise DecodeError("missing event group")
    rows, _priority = groups[index]
    if not rows:
        raise DecodeError("empty event group")
    # Original 216F20 uses row zero if no strict signed range matches, otherwise
    # the first match for rand()%100. Enumerate all 100 possible draws rather
    # than treating unreachable rows as executable scripts.
    def signed(word):
        return word if word < 0x80000000 else word - 0x100000000
    selected = set()
    for draw in range(100):
        row = rows[0]
        if len(rows) > 1:
            row = next((r for r in rows if signed(r[1]) < draw < signed(r[2])), row)
        if row[0] == 0xffffffff:
            continue  # The authored empty-script sentinel cancels this branch.
        if row[0] >= count:
            raise DecodeError("selected script index out of range")
        selected.add(row[0])
    return sorted(selected)


def controller_scripts(entity, flag_count):
    """Enumerate bounded, potentially selected original controller branches.

    This does not evaluate live flags or reorder/replace the original scheduler.
    A satisfiable condition is conservative: rendezvous occurs before the root
    event even when that condition will not happen on this particular run.
    """
    properties = words(bytes.fromhex(entity["payload"]))
    if len(properties) != 2 or properties[0] not in (0, 1):
        raise DecodeError("unproved ScriptController properties")
    count = len(entity["names"])
    candidates = []
    if properties[1] != 0xffffffff:
        candidates += [(i, {"kind": "startup", "group": properties[1]})
                       for i in possible_scripts(entity_groups(entity), properties[1], count)]
    for ordinal, (pairs, _repeat, triples, priority) in enumerate(entity["records"]):
        tests = words(bytes.fromhex(pairs))
        if len(tests) % 2:
            raise DecodeError("invalid condition tests")
        if not tests:
            continue  # Original 217840: an empty condition never matches.
        expected = {}
        possible = True
        for flag, value in zip(tests[::2], tests[1::2]):
            if flag >= flag_count:
                raise DecodeError("condition flag index out of range")
            value = bool(value)
            if flag in expected and expected[flag] != value:
                possible = False
            expected[flag] = value
        if not possible:
            continue
        rows = words(bytes.fromhex(triples))
        if len(rows) % 3:
            raise DecodeError("invalid condition script rows")
        group = [(list(zip(rows[::3], rows[1::3], rows[2::3])), priority)]
        candidates += [(i, {"kind": "condition", "record": ordinal,
                            "tests": sorted(expected.items())})
                       for i in possible_scripts(group, 0, count)]
    return candidates


def read_level(path):
    local = Path(__file__).resolve().parent
    sys.path.insert(0, str(local / "native_level_checks"))
    from inspect_level import load
    tree = ast.parse((local / "native_level_logic_checks/run.py").read_text())
    node = next(n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == "reference")
    namespace = {}
    exec(compile(ast.Module(body=[node], type_ignores=[]), "<original-ent-tail>", "exec"), namespace)
    reader = load(path)
    summary = reader.level()
    triggers, _ = namespace["reference"](reader, summary)
    return reader, triggers


def census_level(path):
    reader, triggers = read_level(path)
    entities = reader.entities
    flag_count = struct.unpack_from("<I", reader.data, 4)[0]
    total = len(entities) + sum(bool(t["flags"] & 32) for t in triggers)
    report = {"file": path.name, "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
              "payload_sha256": hashlib.sha256(reader.data).hexdigest(),
              "entities": len(entities), "triggers": len(triggers),
              "player_custom_volumes": sum(t["kind"] == 37 and bool(t["flags"] & 32) for t in triggers),
              "enter_groups": 0,
              "provider_slot_count": total, "gates": [], "excluded": [], "unproved": []}

    visited = []

    def trace(scripts, script_index, lineage, seen, selection=None):
        key = (tuple(lineage[-1]), script_index)
        if key in seen:
            return [], [{"lineage": lineage, "reason": "script cycle"}]
        seen = seen | {key}
        name, raw = scripts[script_index]
        raw = bytes.fromhex(raw) if isinstance(raw, str) else raw
        proof = {"lineage": lineage, "script_index": script_index, "script_name": name,
                 "script_sha256": hashlib.sha256(raw).hexdigest()}
        if selection is not None:
            proof["selection"] = selection
        visited.append(proof)
        try:
            commands = decode(raw)
        except DecodeError as error:
            return [], [{**proof, "reason": str(error)}]
        reasons, unproved = [], []
        for command in commands:
            op, target = command["opcode"], command.get("argument", 0xffffffff)
            if op == 20:
                # Original 217345..217397 cancels the current event and returns
                # -1. Later bytes belong to an unreachable continuation.
                break
            if op == 5 and target != 0xffffffff:
                if target >= len(scripts):
                    unproved.append({**proof, "reason": "switch-script target out of range"})
                else:
                    a, b = trace(scripts, target, lineage, seen, selection)
                    reasons += a
                    unproved += b
                # Original switch-script transfers, so later bytes are not reachable.
                break
            if op not in (16, 17, 22, 23, 43) or target == 0xffffffff:
                continue
            if target >= len(entities):
                unproved.append({**proof, "reason": f"entity target {target} out of range"})
                continue
            entity = entities[target]
            kind = entity["kind"]
            reason = None
            if op == 16 and kind == "RestartPoint":
                reason = "operate-checkpoint"
            elif op == 17 and kind == "RestartPoint":
                reason = "remove-checkpoint"
            elif op == 22 and kind in ENSURE_PROGRESS:
                reason = "ensure-" + kind
            elif op in DOOR_PROGRESS_OPERATIONS and kind == "DoorProp":
                reason = "door-operation"
            if reason:
                reasons.append({**proof, "command": command, "target": target,
                                "target_class": kind, "target_name": entity["name"], "reason": reason})
            if op == 22 and kind == "ScriptController":
                try:
                    for index, branch in controller_scripts(entity, flag_count):
                        a, b = trace(entity["names"], index, lineage + [("entity", target)], seen, branch)
                        reasons += a
                        unproved += b
                except DecodeError as error:
                    unproved.append({**proof, "target": target, "reason": str(error)})
        return reasons, unproved

    source_index = len(entities)
    for ordinal, trigger in enumerate(triggers):
        if not trigger["flags"] & 32:
            continue
        source = source_index
        source_index += 1
        if trigger["kind"] != 37 or not trigger["scripts"]:
            continue
        descriptor = {"volume_ordinal": ordinal, "source_index": source}
        try:
            _conditions, groups = trigger_groups(trigger["records"])
            enters = [(kind, group) for kind, group in bindings(trigger["properties"]) if kind == 1]
            report["enter_groups"] += len(enters)
            for _kind, group in enters:
                visited.clear()
                starts = possible_scripts(groups, group, len(trigger["scripts"]))
                reasons, unproved = [], []
                for start in starts:
                    a, b = trace(trigger["scripts"], start, [("trigger", ordinal)], set())
                    reasons += a
                    unproved += b
                if unproved:
                    report["unproved"].append({**descriptor, "enter_group": group, "paths": unproved})
                if not reasons:
                    report["excluded"].append({**descriptor, "enter_group": group,
                                               "scripts": [trigger["scripts"][i][0] for i in starts],
                                               "examined_paths": list(visited),
                                               "reason": "no proved progression operation"})
                    continue
                proofs = [{"index": index, "byte_count": len(raw), "hash": fnv1a64(raw),
                           "sha256": hashlib.sha256(raw).hexdigest()}
                          for index, (_name, raw) in enumerate(trigger["scripts"])]
                report["gates"].append({**descriptor, "enter_group": group,
                                        "shape": trigger["shape"], "box_count": len(trigger["boxes"]) // 24,
                                        "provider_slot_count": total, "script_count": len(proofs),
                                        "proofs": proofs, "reasons": reasons,
                                        "examined_paths": list(visited),
                                        "trigger_sha256": hashlib.sha256(reader.data[trigger["offset"]:trigger["end"]]).hexdigest()})
        except DecodeError as error:
            report["unproved"].append({**descriptor, "reason": str(error)})
    assert source_index == total
    return report


def build(area):
    """Return header/gates/proofs/report using local original assets only.

    Fixture preparers may call read_level() for exact source bytes. The generated
    header contains only noncreative metadata and fingerprints, no game scripts.
    """
    area = Path(area)
    xbe = (area / "assets/Default.xbe").read_bytes()
    base = struct.unpack_from("<I", xbe, 0x104)[0]
    count, table = struct.unpack_from("<II", xbe, 0x11c)
    sections = [struct.unpack_from("<6I", xbe, table - base + i * 56) for i in range(count)]
    for va, size, digest in BRANCH_PINS:
        section = next(s for s in sections if s[1] <= va and va + size <= s[1] + s[4])
        at = section[3] + va - section[1]
        if hashlib.sha256(xbe[at:at + size]).hexdigest() != digest:
            raise DecodeError(f"original branch contract changed at {va:#x}")
    levels = [census_level(area / "assets" / f"{level}-01.ent") for level in CAMPAIGN_LEVELS]
    proofs, gates = [], []
    for level in levels:
        for gate in level["gates"]:
            begin = len(proofs)
            proofs.extend(gate["proofs"])
            gates.append((gate, begin))
    header = ["// Generated from exact original campaign ENT scripts; see coop_progression_assets.py.",
              "#pragma once", "#include <array>", "#include <cstdint>", "namespace coop_progression_assets {",
              "struct ScriptProof { uint32_t index, byte_count; uint64_t hash; };",
              "struct Gate { uint32_t provider_slot_count, source_index, volume_ordinal, enter_group; uint32_t script_count, proof_begin, proof_count; };",
              f"inline constexpr std::array<ScriptProof,{len(proofs)}> kProofs={{{{"]
    header += [f'    {{{p["index"]}u,{p["byte_count"]}u,0x{p["hash"]:016x}ull}},' for p in proofs]
    header += ["}};", f"inline constexpr std::array<Gate,{len(gates)}> kGates={{{{"]
    header += ["    {%du,%du,%du,%du,%du,%du,%du}," % (
        g["provider_slot_count"], g["source_index"], g["volume_ordinal"], g["enter_group"],
        g["script_count"], begin, len(g["proofs"])) for g, begin in gates]
    header += ["}};", "}", ""]
    local = Path(__file__).resolve().parent
    sources = (Path(__file__).resolve(), local / "native_level_checks/inspect_level.py",
               local / "native_level_logic_checks/run.py")
    report = {"scope": __doc__, "sources": {p.relative_to(local).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in sources},
              "branch_contracts": [{"address": hex(va), "size": size, "sha256": digest}
                                   for va, size, digest in BRANCH_PINS],
              "levels": levels, "gate_count": len(gates), "proof_count": len(proofs),
              "limitations": ["Static command lineage; no mission-playthrough claim.",
                              "Potentially selected random/conditional branches are enumerated conservatively; actual live conditions remain unchanged.",
                              "Indirect flag-driven events and non-ScriptController creation side effects are outside this census; no universal traversal proof is claimed.",
                              "Only PLAYER enter events are admitted; non-PLAYER, startup/exit, ordinary combat/pickup/hazard paths are unchanged.",
                              "The presence of a proved progression operation admits a root even if a separate child branch is unproved; such branches remain explicit in the report."]}
    return {"header": "\n".join(header), "gates": [dict(g, proof_begin=i, proof_count=len(g["proofs"])) for g, i in gates],
            "proofs": proofs, "report": report}


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("area", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    result = build(args.area)
    report = result["report"]
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / "xbe_coop_progression_assets.h").write_text(result["header"])
    (args.output / "census.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"gates": report["gate_count"], "proofs": report["proof_count"],
                      "levels": [{"file": r["file"], "gates": len(r["gates"]),
                                  "unproved": len(r["unproved"])} for r in report["levels"]]}, indent=2))

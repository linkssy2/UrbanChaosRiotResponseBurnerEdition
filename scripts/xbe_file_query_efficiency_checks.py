"""Qualify the bounded file-query change against extracted production routines.

Default checks source shape and caller inventory without compilation. --prepare
stages a private fixture. --run requires the project's game-closed heavy-work
guard, then compiles/runs semantic parity and an old-source failing control.
"""
from pathlib import Path
import argparse
import datetime
import hashlib
import json
import os
import re
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
sys.path.insert(0, str(HERE))
import prepare_xbe_file_query_efficiency as patch

SDK = ROOT / "build/upstream-review-xboxxbe-20261014"
TRIAL = ROOT / "build/xbe-full-integration-20261014"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def between(source, start, end):
    if source.count(start) != 1:
        raise ValueError("Expected one production extraction anchor: " + start)
    at = source.index(start)
    return source[at:source.index(end, at)]


def original_source(source):
    """Allow qualification against the future integrated output as well."""
    body = patch.query_body(source)
    if patch.OLD_METADATA in body:
        return source
    for old, new in ((patch.OLD_METADATA, patch.NEW_METADATA),
                     (patch.OLD_STANDARD, patch.NEW_STANDARD),
                     (patch.OLD_STANDARD_END, patch.NEW_STANDARD_END)):
        if body.count(new) != 1 or old in body:
            raise ValueError("Unrecognized integrated query shape")
        body = body.replace(new, old, 1)
    restored = source.replace(patch.query_body(source), body, 1)
    if patch.patch_source(restored) != source:
        raise ValueError("Integrated query does not reverse exactly")
    return restored


def caller_inventory(area):
    result = []
    for path in sorted((area / "generated/game").glob("x86_recomp.*.cpp")):
        source = path.read_text(encoding="utf-8")
        for match in re.finditer(r"call \[0x0044b3bc\]", source, re.I):
            prefix = source[:match.start()]
            function = re.findall(r"X86_FUNC\(([^)]+)\)", prefix)[-1]
            # The title pushes class and length before the three address/handle
            # parameters. Keep actual nearby x86 comments, not inferred counts.
            context = "\n".join(prefix.splitlines()[-36:])
            pushes = re.findall(r"// ([0-9A-F]{8}) push (0x[0-9a-f]+)", context, re.I)
            result.append({"file": str(path.relative_to(ROOT)), "function": function,
                           "call_line": source[:match.start()].count("\n") + 1,
                           "nearby_immediate_pushes": pushes})
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prepare", action="store_true")
    parser.add_argument("--run", action="store_true")
    parser.add_argument("--area", type=Path, default=TRIAL)
    parser.add_argument("--sdk", type=Path, default=SDK)
    args = parser.parse_args()
    if args.run and os.environ.get("UCRR_SERVICES_HEAVY_MUTEX") != "owned":
        raise RuntimeError("Use scripts/run_animation_resume.ps1; no compilation while the game is running")
    path = args.area / "src/compat/files.cpp"
    inputs = [path, args.sdk / "runtime/kernel/files.cpp", args.sdk / "runtime/kernel/disc.h",
              HERE / "xbe_file_query_efficiency_checks.cpp", Path(patch.__file__), Path(__file__)]
    hashes = {str(p.resolve()): digest(p) for p in inputs}
    raw = path.read_text(encoding="utf-8")
    source = original_source(raw)
    fixed = patch.patch_source(source)
    sdk_source = (args.sdk / "runtime/kernel/files.cpp").read_text(encoding="utf-8")
    if patch.query_body(source) != patch.query_body(sdk_source):
        raise ValueError("Negative witness must match the qualified SDK query, not a reconstructed different baseline")
    assert original_source(fixed) == source
    assert source.replace(patch.query_body(source), patch.query_body(fixed), 1) == fixed
    assert "case 16:" in patch.query_body(source), "UC mode compatibility must be retained"
    for bad in (fixed, source.replace("    std::error_code ec;\n    uint64_t size = f->dir", "    changed();\n    uint64_t size = f->dir", 1),
                source + patch.BEGIN):
        try:
            patch.patch_source(bad)
        except ValueError:
            pass
        else:
            raise AssertionError("Unqualified source accepted")
    inventory = caller_inventory(args.area)
    if len(inventory) != 9:
        raise ValueError(f"Expected nine direct ordinal-211 title callers, got {len(inventory)}")
    for caller in inventory:
        caller_path = ROOT / caller["file"]
        hashes[str(caller_path.resolve())] = digest(caller_path)
    class_lengths = [tuple(int(value, 16) for _, value in item["nearby_immediate_pushes"][-2:]) for item in inventory]
    if class_lengths.count((14, 8)) != 3 or class_lengths.count((34, 56)) != 6:
        raise ValueError("Direct query callers changed class/length; review the new guest contract")
    report = {"source_checks": True, "compiled_and_executed": False, "sources": hashes,
              "direct_title_callers": inventory, "steps": [],
              "limitations": ["No live FPS or hitch reduction is established by these work counts",
                  "Guest memory, object registry and mounted volume services are fixtures",
                  "Full production File, query, metadata and IOSB helper bodies are extracted",
                  "Host filesystem/stdio are real; the flush-error case injects EOF at the flush boundary",
                  "Host timestamps are checked for consistency and bounded old/new drift; other guest bytes match exactly",
                  "No new locking, output-length validation or unsupported-class behavior is introduced",
                  "Linux/Proton execution is not covered by this Windows fixture"]}
    if not (args.prepare or args.run):
        print(json.dumps(report, indent=2))
        return
    out = ROOT / "build/xbe-file-query-efficiency-checks" / datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%d-%H%M%S-%f")
    out.mkdir(parents=True)
    routines = [("struct File : Object {", "\nstruct Link : Object {"),
                ("uint64_t ToFileTime(", "\nbool WildMatch("),
                ("void IoStatus(", "\nstd::string ObjectName(")]
    template = (HERE / "xbe_file_query_efficiency_checks.cpp").read_text(encoding="utf-8")
    for name, variant in (("ORIGINAL", source), ("FIXED", fixed)):
        extracted = "\n".join(between(variant, a, b) for a, b in routines) + "\n" + patch.query_body(variant)
        # Only instrument the C stdio boundary. The filesystem namespace proxy
        # invokes real filesystem operations and counts them without substitution.
        assert extracted.count("std::fflush(f->fp)") == 1
        extracted = extracted.replace("std::fflush(f->fp)", "FixtureFlush(f->fp)")
        marker = "// EXTRACTED_" + name
        assert template.count(marker) == 1
        template = template.replace(marker, extracted, 1)
    fixture = out / "query_checks.cpp"
    fixture.write_text(template, encoding="utf-8")
    (out / "original-files.cpp").write_text(source, encoding="utf-8")
    (out / "fixed-files.cpp").write_text(fixed, encoding="utf-8")
    report["fixture_sha256"] = digest(fixture)
    report["original_query_sha256"] = hashlib.sha256(patch.query_body(source).encode()).hexdigest()
    report["fixed_query_sha256"] = hashlib.sha256(patch.query_body(fixed).encode()).hexdigest()
    try:
        if args.run:
            vc = Path("C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.44.35207")
            compiler = Path("C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/Llvm/x64/bin/clang-cl.exe")
            kit = Path("C:/Program Files (x86)/Windows Kits/10")
            version = "10.0.26100.0"
            env = {k.upper(): v for k, v in os.environ.items() if not k.upper().startswith("UCRR_")}
            env["INCLUDE"] = ";".join(map(str, [vc / "include"] + [kit / "Include" / version / p for p in ("ucrt", "shared", "um", "winrt")]))
            env["LIB"] = ";".join(map(str, [vc / "lib/x64", kit / "Lib" / version / "um/x64", kit / "Lib" / version / "ucrt/x64"]))
            env["PATH"] = str(vc / "bin/Hostx64/x64") + os.pathsep + env.get("PATH", "")

            def run(name, command, expected=0):
                proc = subprocess.run(list(map(str, command)), cwd=out, env=env, capture_output=True, text=True, timeout=60)
                (out / (name + ".log")).write_text(proc.stdout + proc.stderr, encoding="utf-8")
                step = {"name": name, "exit": proc.returncode, "expected_exit": expected}
                report["steps"].append(step)
                if proc.returncode != expected:
                    raise RuntimeError((proc.stdout + proc.stderr)[-4000:])
                if name != "compile":
                    step["result"] = json.loads(proc.stdout.strip().splitlines()[-1])

            exe = out / "query_checks.exe"
            run("compile", [compiler, "/nologo", "/std:c++20", "/EHsc", "/MD", "/O2", "/W4",
                            "/D_CRT_SECURE_NO_WARNINGS", "/DNOMINMAX", f"/I{args.sdk / 'runtime'}", fixture, f"/Fe{exe}"])
            run("semantic-parity-and-work-counts", [exe])
            run("old-source-negative-witness", [exe, "--zero-work-original"], expected=77)
            run("fixed-zero-work", [exe, "--zero-work-fixed"])
            report["compiled_and_executed"] = True
    finally:
        report["sources_unchanged"] = all(digest(Path(p)) == h for p, h in hashes.items())
        (out / "results.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print(out / "results.json")


if __name__ == "__main__":
    main()

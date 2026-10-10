"""Show the mouse orders cue whenever an original NPC command is available."""
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BODY_SHA256 = '2e03ff0d9d1db6279361b16162c50afbf704a127b0993ebf453cf374d535a30f'
MARKER = 'loc_00158D1B: ;'
INSERT = '\n    /* Targeted prompts need the idle mouse cue too, so players can discover\n     * the hold/drag binding before pressing it. Availability remains original. */\n    { extern int xbox_KeyboardMode(void);\n      if (xbox_KeyboardMode()) goto loc_00158D24; }\n'


def body(source):
    start = source.index('void sub_00158510(void)')
    return source[start:source.index('\n}\n', start) + 2]

def transform(source):
    original = body(source)
    if hashlib.sha256(original.encode()).hexdigest() != BODY_SHA256:
        raise ValueError('Unknown or already modified original orders update')
    if original.count(MARKER) != 1:
        raise ValueError('Unknown orders visibility boundary')
    return source.replace(original, original.replace(MARKER, MARKER + INSERT, 1), 1)

def self_check():
    source = (ROOT/'src/recomp/gen/recomp_0012.c').read_text()
    changed = transform(source)
    assert changed.replace(INSERT, '', 1) == source
    # Other selected chunk12 candidates remain byte-for-byte untouched.
    unrelated = source.replace('void sub_001400A0(void)', 'void unrelated_selected_candidate(void)', 1)
    assert transform(unrelated).replace(INSERT, '', 1) == unrelated
    for bad in (changed, source.replace(MARKER, MARKER + ' /* drift */', 1)):
        try:
            transform(bad)
        except ValueError:
            pass
        else:
            raise AssertionError('Accepted duplicate/changed orders update')
    return {'passed': True, 'original_function_sha256': BODY_SHA256,
            'hook': '158D1B: after original command-availability gate, before target-specific pad suppression',
            'scope': 'original pad visibility only; original input, NPC eligibility and command dispatch remain unchanged'}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    report = self_check()
    if args.input or args.output:
        if not (args.input and args.output):
            parser.error('Input/output paired')
        if args.input.resolve() == args.output.resolve():
            raise ValueError('In-place output forbidden')
        source = args.input.read_text()
        result = transform(source)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(result)
        report.update(input=str(args.input.resolve()), output=str(args.output.resolve()),
                      input_sha256=hashlib.sha256(source.encode()).hexdigest(),
                      output_sha256=hashlib.sha256(result.encode()).hexdigest())
        args.output.with_suffix('.manifest.json').write_text(json.dumps(report, indent=2)+'\n')
    elif not args.check:
        parser.error('Use --check or input/output')
    if args.check:
        print(json.dumps(report, indent=2))

if __name__ == '__main__':
    main()

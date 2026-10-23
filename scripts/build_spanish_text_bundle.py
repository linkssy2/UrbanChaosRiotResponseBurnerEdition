"""Extract distributable Spanish string changes, never whole game archives."""
from pathlib import Path
import argparse
import hashlib
import json
import sys
import tempfile
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parent))
from analyze_frontend import parse_archive, unpack_entry
from convert_zarf_spanish import blocks, strings, convert

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'launcher/burner-xbe'))
import spanish_text


def create(original, mod, output, expected=None):
    records = []
    with tempfile.TemporaryDirectory(prefix='spanish-bundle-') as directory:
        temporary = Path(directory)
        for name in spanish_text.FILES:
            source_path = original / name
            target_path = temporary / name
            report = convert(source_path, mod / name, target_path)
            source, entries = parse_archive(source_path)
            target, final_entries = parse_archive(target_path)
            if expected is not None and target != (expected / name).read_bytes():
                raise ValueError(f'{name}: converter differs from the accepted Spanish pack')
            entry_name = source_path.stem + '.exttxt'
            source_text = unpack_entry(source, next(e for e in entries if e['name'] == entry_name))
            target_text = unpack_entry(target, next(e for e in final_entries if e['name'] == entry_name))
            source_header, source_banks = blocks(source_text)
            target_header, target_banks = blocks(target_text)
            before = strings(source_header, source_banks[0])
            after = strings(target_header, target_banks[0])
            changes = []
            for key, values in before.items():
                for (at, old), (new_at, new) in zip(values, after[key], strict=True):
                    if at != new_at:
                        raise ValueError('Converter changed the original lookup layout')
                    if old != new:
                        changes.append([at, new.decode('latin1')])
            if spanish_text._translate(source_text, sorted(changes)) != target_text:
                raise ValueError(f'{name}: text-only conversion differs from full converter')
            records.append({'file': name, 'source_size': len(source),
                            'source_sha256': hashlib.sha256(source).hexdigest(),
                            'size': report['size'], 'sha256': report['sha256'],
                            'changes': sorted(changes)})
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps({'format': 1, 'files': records}, ensure_ascii=True,
                                    separators=(',', ':')) + '\n', encoding='utf-8')
        spanish_text.load_bundle(output)
        rebuilt = temporary / 'rebuilt'
        generated = spanish_text.build(original, output, rebuilt)
        for item in generated:
            if (rebuilt / item['file']).read_bytes() != (temporary / item['file']).read_bytes():
                raise ValueError('Text-only archive differs from existing converter')
    return {'files': len(records), 'translated_strings': sum(len(r['changes']) for r in records),
            'bundle_bytes': output.stat().st_size,
            'bundle_sha256': hashlib.sha256(output.read_bytes()).hexdigest(),
            'generated_bytes': sum(r['size'] for r in records),
            'original_bytes': sum(r['source_size'] for r in records),
            'zlib_version': zlib.ZLIB_RUNTIME_VERSION,
            'all_outputs_byte_identical_to_converter': True,
            'all_outputs_byte_identical_to_accepted_pack': expected is not None,
            'contains_archives_or_audio': False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--original', required=True, type=Path)
    parser.add_argument('--mod', required=True, type=Path)
    parser.add_argument('--output', default=ROOT / 'packaging/spanish/text.json', type=Path)
    parser.add_argument('--expected', type=Path)
    args = parser.parse_args()
    print(json.dumps(create(args.original, args.mod, args.output, args.expected), indent=2))


if __name__ == '__main__':
    main()

"""Merge ZARF's beta text into local Xbox archives without replacing voice data."""
from pathlib import Path
import argparse, hashlib, json, re, struct, zlib
from analyze_frontend import parse_archive, unpack_entry


def blocks(data, single=False):
    languages, version, keys, lookups, texts, audio, count = struct.unpack_from('<HH5I', data)
    size = keys + lookups + texts + audio
    if version != 32 or keys != count * 56 or lookups % 12 or not size:
        raise ValueError('Unsupported text bank layout')
    actual = 1 if single else languages
    if len(data) != 24 + actual * size:
        raise ValueError('Text bank size mismatch')
    return (languages, version, keys, lookups, texts, audio, count), [data[24+i*size:24+(i+1)*size] for i in range(actual)]


def strings(header, block):
    _, _, keys, lookups, texts, audio, count = header
    result = {}
    for i in range(count):
        record = block[i*56:(i+1)*56]
        key = bytes(record[:40]).split(b'\0')[0]
        variants = record[49]
        start = struct.unpack_from('<I', record, 52)[0]
        if not key or key in result or start % 12 or start + variants*12 > lookups:
            raise ValueError('Invalid text key or variant range')
        values = []
        for j in range(variants):
            at = keys + start + j*12
            offset = struct.unpack_from('<I', block, at)[0]
            if offset >= texts:
                raise ValueError('Text offset outside bank')
            begin = keys + lookups + offset
            end = block.find(b'\0', begin, keys+lookups+texts)
            if end < 0:
                raise ValueError('Unterminated text')
            values.append((at, block[begin:end]))
        result[key] = values
    return result


def mod_bank(data):
    # Beta headers are stale in some missions; validate the actual contiguous
    # key/variant tables and recover only strings, never its phoneme data.
    count = 0
    while count < 10000:
        record = data[24+count*56:24+(count+1)*56]
        if len(record) != 56 or not re.fullmatch(rb'[a-zA-Z0-9_-]+', record[:40].split(b'\0')[0]):
            break
        count += 1
    if not count: raise ValueError('No translation keys')
    keys = count*56
    lookup_size = 0
    for i in range(count):
        record = data[24+i*56:24+(i+1)*56]
        offset = struct.unpack_from('<I', record, 52)[0]
        if offset != lookup_size or not record[49]:
            raise ValueError('Non-contiguous translation variants')
        lookup_size += record[49]*12
    text_start = 24+keys+lookup_size
    end = text_start
    for i in range(0, lookup_size, 12):
        offset = struct.unpack_from('<I', data, 24+keys+i)[0]
        begin = text_start+offset
        if begin >= len(data): raise ValueError('Translation offset outside stream')
        stop = data.find(b'\0', begin, min(len(data), begin+65536))
        if stop < 0: raise ValueError('Unterminated translation')
        end = max(end, stop+1)
    header = (1,32,keys,lookup_size,end-text_start,0,count)
    block = data[24:end]
    strings(header,block)
    return header, block


def merge(original, translated):
    header, banks = blocks(original)
    mod_header, mod_block = mod_bank(translated)
    source = strings(header, banks[0]); target = strings(mod_header, mod_block)
    _, _, keys, lookups, text_size, audio, _ = header
    table = bytearray(banks[0][:keys+lookups]); text = bytearray()
    additions = json.loads(Path(__file__).with_name('spanish_original.json').read_text(encoding='utf-8'))
    additions = {a.encode('cp1252'): b.encode('cp1252') for a,b in additions.items()}
    for before,after in additions.items():
        if re.findall(rb'%[a-zA-Z]|[\x01-\x1f]',before) != re.findall(rb'%[a-zA-Z]|[\x01-\x1f]',after):
            raise ValueError('Supplement changes formatting tokens')
    changed = fallback = supplemented = 0
    for key, values in source.items():
        replacement = target.get(key)
        if replacement is None or len(replacement) != len(values):
            replacement = values; fallback += 1
        for (at, before), (_, after) in zip(values, replacement):
            if after == before and before in additions:
                after = additions[before]; supplemented += 1
            # Formatting tokens are game bytecodes, not Unicode text.
            struct.pack_into('<I', table, at, len(text))
            text.extend(after + b'\0'); changed += before != after
    width = max(text_size, len(text))
    new_header = list(header); new_header[4] = width
    result = bytearray(struct.pack('<HH5I', *new_header))
    result.extend(table + text + bytes(width-len(text)) + banks[0][-audio:] if audio else table + text + bytes(width-len(text)))
    for bank in banks[1:]:
        result.extend(bank[:keys+lookups+text_size] + bytes(width-text_size) + bank[keys+lookups+text_size:])
    h, verified = blocks(result)
    for bank in verified: strings(h, bank)
    for old, new in zip(banks, verified):
        assert old[:keys] == new[:keys]
        assert old[keys+lookups+text_size:] == new[keys+lookups+width:]
        for at in range(keys, keys+lookups, 12):
            assert old[at+4:at+12] == new[at+4:at+12]
    for old, new in zip(banks[1:], verified[1:]):
        assert strings(header, old) == strings(h, new)
    return bytes(result), {'changed_strings': changed, 'fallback_keys': fallback, 'supplemented_strings': supplemented,
                           'unused_mod_keys': len(target.keys()-source.keys())}


def convert(original_path, mod_path, output):
    original, entries = parse_archive(original_path)
    mod, mod_entries = parse_archive(mod_path)
    name = original_path.stem + '.exttxt'
    entry = next(e for e in entries if e['name'] == name)
    replacement = next(e for e in mod_entries if e['name'] == name)
    at, size = replacement['data_offset'], replacement['packed_size']
    decoder = zlib.decompressobj()
    translation = decoder.decompress(mod[at:at+size], 16*1024*1024)
    if not decoder.eof or decoder.unconsumed_tail:
        raise ValueError('Incomplete or oversized translation stream')
    data, report = merge(unpack_entry(original, entry), translation)
    packed = zlib.compress(data, 9)
    result = bytearray(original)
    cursor = 0xc8
    for e in entries:
        length = struct.unpack_from('<H', original, cursor)[0]
        cursor += 2 + length
        if e == entry:
            struct.pack_into('<III', result, cursor, len(original), len(data), len(packed))
        cursor += 16
    result.extend(packed)
    output.write_bytes(result)
    check, checked_entries = parse_archive(output)
    for a, b in zip(entries, checked_entries):
        if a == entry:
            assert unpack_entry(check, b) == data
        else:
            assert a == b
            assert original[a['data_offset']:a['data_offset']+a['packed_size']] == check[b['data_offset']:b['data_offset']+b['packed_size']]
    return dict(file=output.name, size=len(result), sha256=hashlib.sha256(result).hexdigest(), **report)


def main():
    p=argparse.ArgumentParser();p.add_argument('--original',type=Path,required=True);p.add_argument('--mod',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    args=p.parse_args()
    args.output.mkdir(parents=True,exist_ok=False)
    report=[convert(args.original/f.name,f,args.output/f.name) for f in sorted(args.mod.glob('*.ent'))]
    if len(report)!=27: raise ValueError('Expected the 27 ZARF beta level files')
    (args.output/'verification.json').write_text(json.dumps(report,indent=2))
    (args.output/'manifest.txt').write_text(''.join(f"{r['file']} {r['size']}\n" for r in report))
    print(json.dumps({'files':len(report),'changed_strings':sum(r['changed_strings'] for r in report),'fallback_keys':sum(r['fallback_keys'] for r in report)}))

if __name__=='__main__':main()


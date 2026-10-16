#!/usr/bin/env python3
"""Inventory Urban Chaos: Riot Response front-end archives.

The Xbox build stores menu resources in a small archive format used by .lst and
.ent files.  This tool records the directory and safe structural metadata; it
does not extract or modify the original game data.
"""

from __future__ import annotations

import argparse
import collections
import json
import struct
import zlib
from pathlib import Path


ARCHIVE_HEADER_SIZE = 0xC8


def read_u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def read_u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def parse_archive(path: Path) -> tuple[bytes, list[dict[str, int | str]]]:
    data = path.read_bytes()
    if len(data) < ARCHIVE_HEADER_SIZE:
        raise ValueError(f"{path} is too small to be a resource archive")

    directory_size = read_u32(data, 0xC0)
    file_count = read_u32(data, 0xC4)
    cursor = ARCHIVE_HEADER_SIZE
    entries: list[dict[str, int | str]] = []

    for index in range(file_count):
        name_length = read_u16(data, cursor)
        cursor += 2
        name_bytes = data[cursor : cursor + name_length]
        cursor += name_length
        data_offset, unpacked_size, packed_size, bank = struct.unpack_from(
            "<IIII", data, cursor
        )
        cursor += 16
        name = name_bytes.rstrip(b"\0").decode("latin-1")
        extension = Path(name).suffix.lower() or "<none>"
        entries.append(
            {
                "index": index,
                "name": name,
                "extension": extension,
                "data_offset": data_offset,
                "unpacked_size": unpacked_size,
                "packed_size": packed_size,
                "bank": bank,
            }
        )

    # directory_size includes the four-byte file-count field at 0xC4.
    expected_end = 0xC4 + directory_size
    if cursor != expected_end:
        raise ValueError(
            f"{path}: parsed directory ends at 0x{cursor:X}, expected "
            f"0x{expected_end:X}"
        )
    return data, entries


def unpack_entry(data: bytes, entry: dict[str, int | str]) -> bytes:
    unpacked_size = int(entry["unpacked_size"])
    packed_size = int(entry["packed_size"])
    start = int(entry["data_offset"])
    if not 0 <= unpacked_size <= 256 * 1024 * 1024 or packed_size < 0:
        raise ValueError(f"{entry['name']}: invalid or excessive payload size")
    if not 0 <= start <= len(data) or packed_size > len(data) - start:
        raise ValueError(f"{entry['name']}: payload outside archive")
    if packed_size == 0:
        if unpacked_size:
            raise ValueError(f"{entry['name']}: unsupported external/raw slice")
        return b""
    payload = data[start : start + packed_size]
    # Original 8DCC0 always inflates this path, even when both lengths match.
    decoder = zlib.decompressobj()
    result = decoder.decompress(payload, unpacked_size + 1)
    if (len(result) != unpacked_size or not decoder.eof or
            decoder.unused_data or decoder.unconsumed_tail):
        raise ValueError(
            f"{entry['name']}: unpacked {len(result)} bytes, expected {unpacked_size}"
        )
    return result


def printable_strings(data: bytes, minimum: int = 4) -> list[str]:
    strings: list[str] = []
    start = 0
    for end, value in enumerate(data + b"\0"):
        if value in (9, 10, 13) or 0x20 <= value <= 0x7E:
            continue
        if end - start >= minimum:
            strings.append(data[start:end].decode("latin-1"))
        start = end + 1
    return strings


def parse_exttxt(data: bytes) -> dict[str, object]:
    """Decode the game's fixed-record multilingual text-bank format."""
    if len(data) < 24:
        raise ValueError("text bank is smaller than its header")

    language_count, format_value = struct.unpack_from("<HH", data, 0)
    key_bytes, lookup_bytes, text_bytes, audio_bytes, entry_count = (
        struct.unpack_from("<IIIII", data, 4)
    )
    block_bytes = key_bytes + lookup_bytes + text_bytes + audio_bytes
    if 24 + language_count * block_bytes != len(data):
        raise ValueError("text-bank component sizes do not match the payload")
    if entry_count == 0 or key_bytes % entry_count or lookup_bytes % entry_count:
        raise ValueError("text-bank tables do not divide into fixed records")

    key_record_size = key_bytes // entry_count
    lookup_record_size = lookup_bytes // entry_count
    if key_record_size < 9 or lookup_record_size < 4:
        raise ValueError("text-bank records are unexpectedly small")

    languages: list[dict[str, str]] = []
    language_metadata: list[dict[str, int]] = []
    for language_index in range(language_count):
        block_start = 24 + language_index * block_bytes
        lookup_start = block_start + key_bytes
        text_start = lookup_start + lookup_bytes
        translations: dict[str, str] = {}

        for entry_index in range(entry_count):
            key_start = block_start + entry_index * key_record_size
            # The final eight bytes are lookup metadata, leaving a fixed key buffer.
            key_buffer = data[key_start : key_start + key_record_size - 8]
            key = key_buffer.split(b"\0", 1)[0].decode(
                "cp1252", errors="replace"
            )
            lookup_start_for_entry = lookup_start + entry_index * lookup_record_size
            string_offset = read_u32(data, lookup_start_for_entry)
            value_start = text_start + string_offset
            value_end = data.find(b"\0", value_start, text_start + text_bytes)
            if value_end < 0:
                raise ValueError(f"{key}: text is not null terminated")
            translations[key] = data[value_start:value_end].decode(
                "cp1252", errors="replace"
            )

        languages.append(translations)
        language_metadata.append(
            {
                "index": language_index,
                "block_offset": block_start,
                "text_offset": text_start,
            }
        )

    keys = list(languages[0])
    catalog = [
        {
            "key": key,
            "translations": [language.get(key, "") for language in languages],
        }
        for key in keys
    ]
    return {
        "language_count": language_count,
        "format_value": format_value,
        "entry_count": entry_count,
        "key_record_size": key_record_size,
        "lookup_record_size": lookup_record_size,
        "key_table_bytes": key_bytes,
        "lookup_table_bytes": lookup_bytes,
        "text_table_bytes": text_bytes,
        "audio_table_bytes": audio_bytes,
        "language_blocks": language_metadata,
        "catalog": catalog,
    }


def summarize_archive(path: Path) -> dict[str, object]:
    data, entries = parse_archive(path)
    extensions = collections.Counter(str(entry["extension"]) for entry in entries)
    menu_terms = (
        "frontend",
        "menu",
        "profile",
        "arrow",
        "button",
        "cursor",
        "select",
        "logo",
        "mask",
        "font",
        "scroll",
        "bar",
        "panel",
        "splash",
    )

    special_entries: list[dict[str, object]] = []
    for entry in entries:
        extension = str(entry["extension"])
        if extension not in {".ent", ".exttxt", ".bin", ".icn"}:
            continue
        unpacked = unpack_entry(data, entry)
        strings = printable_strings(unpacked)
        special_entries.append(
            {
                **entry,
                "printable_string_count": len(strings),
                "representative_strings": strings[:80],
                **(
                    {"exttxt": parse_exttxt(unpacked)}
                    if extension == ".exttxt"
                    else {}
                ),
            }
        )

    menu_named = [
        entry["name"]
        for entry in entries
        if any(term in str(entry["name"]).lower() for term in menu_terms)
    ]
    shader_dependencies: list[dict[str, object]] = []
    texture_resources: list[dict[str, object]] = []
    for entry in entries:
        extension = str(entry["extension"])
        if extension not in {".shader", ".tga"}:
            continue
        unpacked = unpack_entry(data, entry)
        if extension == ".shader":
            strings = printable_strings(unpacked)
            shader_dependencies.append(
                {
                    "name": entry["name"],
                    "unpacked_size": len(unpacked),
                    "texture_dependencies": [
                        value for value in strings if value.lower().endswith(".tga")
                    ],
                    "contains_xbox_pixel_shader": b"PSB0" in unpacked,
                }
            )
        elif len(unpacked) >= 28:
            texture_resources.append(
                {
                    "name": entry["name"],
                    "unpacked_size": len(unpacked),
                    "resource_type": read_u32(unpacked, 0),
                    "width": read_u32(unpacked, 4),
                    "height": read_u32(unpacked, 8),
                    "levels": read_u32(unpacked, 12),
                    "format_index": read_u32(unpacked, 16),
                    "flags": read_u32(unpacked, 20),
                    "payload_size": read_u32(unpacked, 24),
                    "payload_size_matches": read_u32(unpacked, 24)
                    == len(unpacked) - 28,
                }
            )
    return {
        "path": str(path.resolve()),
        "archive_size": len(data),
        "directory_size": read_u32(data, 0xC0),
        "file_count": len(entries),
        "extensions": dict(sorted(extensions.items())),
        "menu_named_count": len(menu_named),
        "menu_named_entries": menu_named,
        "special_entries": special_entries,
        "shader_dependencies": shader_dependencies,
        "texture_resources": texture_resources,
        "entries": entries,
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("paths", nargs="+", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    report = {"archives": [summarize_archive(path) for path in args.paths]}
    rendered = json.dumps(report, indent=2, ensure_ascii=False)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered + "\n", encoding="utf-8")
    else:
        print(rendered)


if __name__ == "__main__":
    main()

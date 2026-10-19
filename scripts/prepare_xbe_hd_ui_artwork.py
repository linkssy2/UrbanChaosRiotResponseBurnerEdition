"""Compile the same approved artwork identities used by the release packager."""
from pathlib import Path
import importlib.util
import json

ROOT = Path(__file__).resolve().parents[1]


def prepare(output):
    spec = importlib.util.spec_from_file_location('hd_ui_packager', ROOT / 'launcher/burner-xbe/package.py')
    packager = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(packager)
    directory = ROOT / 'artwork/hd-ui'
    packager.artwork_sources(directory)
    rows = json.loads((directory / 'manifest.json').read_text())['assets']
    lines = ['#pragma once', '#include "xbe_hd_ui_restored.h"', '',
             '// Generated from artwork/hd-ui/manifest.json; originals are supplied by the user.',
             'namespace uc_hd_ui::restored {',
             f'inline constexpr std::array<Artwork,{len(rows)}> kArtwork{{{{']
    for row in rows:
        values = [json.dumps(row['name']), str(row['width']), str(row['height']), str(row['format']),
                  str(row['source_bytes']), json.dumps(row['source_sha256']), json.dumps(row['png']),
                  json.dumps(row['png_sha256']), 'Crop{' + ','.join(map(str, row['crop'])) + '}',
                  'AlphaPolicy::' + ('Source' if row['alpha'] == 'source' else 'Png'),
                  'Placement::' + ('SourceBounds' if row.get('placement') == 'source_bounds' else 'Canvas'),
                  'ColorPolicy::' + ('SourceChroma' if row.get('color') == 'source_chroma' else 'Png')]
        lines.append('    {' + ', '.join(values) + '},')
    lines += ['}};', '}', '']
    Path(output).write_text('\n'.join(lines))
    return len(rows)

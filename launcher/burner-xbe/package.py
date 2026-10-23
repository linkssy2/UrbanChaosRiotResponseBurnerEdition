"""Assemble a tested release from an explicit runtime/restored-art allowlist."""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import re
import shutil
import struct
import sys

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
VERSION = '0.07'
SDK = REPO / 'build/upstream-review-xboxxbe-20261014'
RUNTIME_NAME = 'UrbanChaosXbeTrial.exe'
RUNTIME_FILES = (RUNTIME_NAME, 'zlib1.dll', 'libusb-1.0.dll', 'msvcp140.dll', 'vcruntime140.dll', 'vcruntime140_1.dll')
VS = Path('C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools')
CRT = VS / 'VC/Redist/MSVC/14.44.35112/x64/Microsoft.VC143.CRT'
PYTHON_FILES = ('python.exe', 'python312.dll', 'python3.dll', 'python312.zip',
                'vcruntime140.dll', 'vcruntime140_1.dll', '_hashlib.pyd', 'libcrypto-3.dll', 'LICENSE.txt')
LAUNCHERS = {'Burner Edition Setup.exe', 'Play Burner Edition.exe'}
ARTWORK = REPO / 'artwork/hd-ui'
ARTWORK_LIMIT = 32 * 1024 * 1024
SHADER_SEED = REPO / 'packaging/shader-seed'
SPANISH = REPO / 'packaging/spanish'
SPANISH_FILES = {'spanish/text.json', 'spanish/credits.txt'}


def shader_seed_validator():
    # This packager is also loaded by path by the source/artwork build tools.
    spec = importlib.util.spec_from_file_location('burner_shader_seed', HERE / 'shader_seed.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def shader_seed_sources(directory):
    validator = shader_seed_validator()
    manifest = directory / 'manifest.json'
    validator.unlinked(manifest.absolute())
    if not manifest.is_file() or manifest.stat().st_size > 256 * 1024:
        raise ValueError('Invalid bundled shader seed manifest.')
    data = json.loads(manifest.read_text())
    if (not isinstance(data, dict) or set(data) != {'version', 'files'} or
            type(data['version']) is not int or data['version'] != 1):
        raise ValueError('Invalid bundled shader seed schema.')
    files = validator.validate(directory, data['files'])
    if {path.name for path in directory.iterdir()} != {'manifest.json', *files}:
        raise ValueError('Unlisted files in the shader seed directory.')
    return {'runtime/shader-seed/' + name: directory / name for name in files}, files


def spanish_text_validator():
    spec = importlib.util.spec_from_file_location('burner_spanish_text', HERE / 'spanish_text.py')
    module = importlib.util.module_from_spec(spec)
    old_path = sys.path[:]
    try:
        sys.path.insert(0, str(HERE))
        spec.loader.exec_module(module)
    finally:
        sys.path[:] = old_path
    return module


def spanish_sources(directory):
    """Ship translated strings; rebuild the game archives from the user's ISO."""
    for name in ('text.json', 'credits.txt'):
        shader_seed_validator().unlinked((directory / name).absolute())
    if {path.name for path in directory.iterdir()} != {'text.json', 'credits.txt'}:
        raise ValueError('Unexpected files in the bundled Spanish directory.')
    spanish_text_validator().load_bundle(directory / 'text.json')
    credits = directory / 'credits.txt'
    if not credits.is_file() or not 0 < credits.stat().st_size <= 16 * 1024:
        raise ValueError('Invalid bundled Spanish credits.')
    credits.read_text(encoding='utf-8')
    plan = {'runtime/spanish/' + name: directory / name for name in ('text.json', 'credits.txt')}
    return plan, {name[8:]: sha(path) for name, path in plan.items()}


def valid_hash(value):
    return isinstance(value, str) and re.fullmatch(r'[0-9a-f]{64}', value)


def artwork_name(value):
    if (not isinstance(value, str) or not 0 < len(value) < 128 or '..' in value or
            not re.fullmatch(r'[A-Za-z0-9_.-]+\.png', value)):
        raise ValueError('Invalid approved artwork filename')
    if value.split('.')[0].upper() in {'CON', 'PRN', 'AUX', 'NUL',
                                     *[f'COM{i}' for i in range(10)], *[f'LPT{i}' for i in range(10)]}:
        raise ValueError('Reserved approved artwork filename')
    return value


def artwork_sources(directory):
    """Only the canonical approved manifest can add art to the release allowlist."""
    if directory.is_symlink() or directory.is_junction():
        raise ValueError('Artwork source directory cannot be a link')
    manifest = directory / 'manifest.json'
    if manifest.is_symlink() or manifest.stat().st_size > 1024 * 1024:
        raise ValueError('Invalid approved artwork manifest')
    data = json.loads(manifest.read_text())
    if (not isinstance(data, dict) or set(data) != {'version', 'assets'} or type(data['version']) is not int or
            data['version'] != 1 or not isinstance(data['assets'], list) or len(data['assets']) > 512):
        raise ValueError('Invalid approved artwork schema')
    fields = {'name', 'width', 'height', 'format', 'source_bytes', 'source_sha256',
              'png', 'png_sha256', 'crop', 'alpha'}
    plan, identities, seen = {}, {}, {}
    for row in data['assets']:
        if not isinstance(row, dict) or not fields <= set(row) or set(row) - fields - {'placement', 'color'}:
            raise ValueError('Invalid approved artwork record')
        name = row['name']
        if (not isinstance(name, str) or not name or len(name) > 512 or name != name.lower() or
                not name.startswith(('hud\\', 'frontend\\', 'font\\', 'fonts\\')) or
                any(ord(c) < 32 or ord(c) > 126 or c == '/' for c in name) or
                any(part in ('', '.', '..') for part in name.split('\\'))):
            raise ValueError('Invalid original UI identity name')
        if (any(type(row[k]) is not int for k in ('width', 'height', 'format', 'source_bytes')) or
                not 0 < row['width'] <= 1024 or not 0 < row['height'] <= 1024 or row['format'] not in (12, 15) or
                not 0 < row['source_bytes'] <= 2 * 1024 * 1024 or not valid_hash(row['source_sha256']) or
                not valid_hash(row['png_sha256']) or row['alpha'] not in ('png', 'source')):
            raise ValueError('Invalid original UI identity or artwork hash')
        png = artwork_name(row['png'])
        placement = row.get('placement', 'canvas')
        bound_sprite = name.startswith('hud\\') or name in {
            'frontend\\frt_content\\' + leaf for leaf in ('lc_medal01.tga', 'lc_medal02.tga',
                'lc_medal03.tga', 'lc_medal04.tga', 'lc_medal01smg.tga', 'reward_star.tga')}
        if (placement not in ('canvas', 'source_bounds') or
                (placement == 'source_bounds' and (row['alpha'] != 'png' or not bound_sprite))):
            raise ValueError('Invalid approved artwork placement')
        color = row.get('color', 'png')
        if color not in ('png', 'source_chroma') or (color == 'source_chroma' and not (
                row['alpha'] == 'source' and row['width'] == row['height'] == 32 and
                name.startswith(('frontend\\frt_content\\level_image_', 'frontend\\frt_content\\reward_image_')) and
                name.endswith('smg.tga'))):
            raise ValueError('Invalid approved artwork color policy')
        if png.lower() in seen and seen[png.lower()] != (png, row['png_sha256']):
            raise ValueError('Conflicting approved artwork filename')
        crop = row['crop']
        if not isinstance(crop, list) or len(crop) != 4 or any(type(v) is not int or v < 0 for v in crop):
            raise ValueError('Invalid approved artwork crop')
        identity = tuple(row[k] for k in ('width', 'height', 'format', 'source_bytes', 'source_sha256'))
        replacement = (png, row['png_sha256'], tuple(crop), row['alpha'], placement, color)
        if identity in identities and identities[identity] != replacement:
            raise ValueError('Conflicting artwork for an identical original texture')
        path = directory / png
        if path.is_symlink() or not path.is_file() or not 0 < path.stat().st_size <= ARTWORK_LIMIT or sha(path) != row['png_sha256']:
            raise ValueError('Missing or modified approved artwork: ' + png)
        with path.open('rb') as file:
            header = file.read(24)
        if len(header) != 24 or header[:16] != b'\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR':
            raise ValueError('Approved artwork is not a PNG: ' + png)
        width, height = struct.unpack('>II', header[16:24])
        if not 0 < width <= 8192 or not 0 < height <= 8192 or width * height * 4 > 64 * 1024 * 1024:
            raise ValueError('Approved artwork dimensions exceed the loader bounds')
        x, y, w, h = (0, 0, width, height) if crop == [0, 0, 0, 0] else crop
        if (not w or not h or x + w > width or y + h > height or
                w < row['width'] * 2 or h < row['height'] * 2 or
                (placement != 'source_bounds' and w * row['height'] != h * row['width'])):
            raise ValueError('Approved artwork crop does not preserve the source aspect')
        seen[png.lower()] = (png, row['png_sha256'])
        identities[identity] = replacement
        plan['runtime/hd-ui/' + png] = path
    return plan, {'hd-ui/' + png: digest for png, digest in seen.values()}


def sha(path):
    with path.open('rb') as file:
        return hashlib.file_digest(file, 'sha256').hexdigest()


def sources(runtime):
    result = {
        'tools/import_game.py': HERE / 'import_game.py',
        'tools/disc.py': HERE / 'disc.py',
        'tools/world_textures.py': HERE / 'world_textures.py',
        'tools/shader_seed.py': HERE / 'shader_seed.py',
        'tools/spanish_text.py': HERE / 'spanish_text.py',
        'tools/disc-manifest.json': HERE.parent / 'burner/disc-manifest.json',
        'tools/xdvdfs.py': REPO / 'third_party/xboxrecomp/tools/xiso/xdvdfs.py',
        'runtime/' + RUNTIME_NAME: runtime,
        'runtime/zlib1.dll': runtime.parent / 'zlib1.dll',
        'runtime/libusb-1.0.dll': runtime.parent / 'libusb-1.0.dll',
        'licenses/libusb-COPYING.txt': REPO / '.tools/libusb-1.0.29/COPYING',
        'licenses/libusb-1.0.29.tar.bz2': REPO / '.tools/libusb-1.0.29/libusb-1.0.29.tar.bz2',
        'cover.png': HERE.parent / 'burner/cover.png',
        'README.txt': HERE / 'README-release.txt',
        'licenses/XboxXBERecomp.txt': SDK / 'LICENSE',
        'licenses/XboxXBERecomp-third-party.txt': SDK / 'THIRD_PARTY.txt',
        'licenses/xboxrecomp.txt': REPO / 'third_party/xboxrecomp/LICENSE',
        'licenses/xboxrecomp-notice.txt': REPO / 'third_party/xboxrecomp/NOTICE',
        'licenses/zlib.txt': Path('C:/Users/Administrator/.cache/codex-runtimes/codex-primary-runtime/dependencies/native/git/mingw64/share/licenses/zlib/LICENSE'),
        'licenses/Microsoft-Visual-Cpp-Runtime-license.rtf': HERE / 'Microsoft-Visual-Cpp-Runtime-license.rtf',
        'licenses/Microsoft-Visual-Studio-Redist.txt': VS / 'Licenses/1033/Redist.txt',
        'licenses/Microsoft-Visual-Cpp-Redistribution.txt': HERE / 'Microsoft-Visual-Cpp-Redistribution.txt',
    }
    for name in ('msvcp140.dll', 'vcruntime140.dll', 'vcruntime140_1.dll'):
        result['runtime/' + name] = CRT / name
    for name in PYTHON_FILES:
        result['tools/python/' + name] = REPO / '.tools/python' / name
    for path in (REPO / 'third_party/xboxrecomp/LICENSES').glob('*.txt'):
        result['licenses/' + path.name] = path
    return result


def audit(package, allowlist, require_launchers=True):
    """Exact file allowlist plus full disc-payload hashes; writes nothing in package."""
    expected = set(allowlist)
    if require_launchers:
        expected |= LAUNCHERS
    paths = list(package.rglob('*'))
    if any(path.is_symlink() or path.is_junction() for path in paths):
        raise ValueError('Package contains a linked file or directory')
    actual = {p.relative_to(package).as_posix() for p in paths if p.is_file()}
    if actual != expected:
        raise ValueError('Unexpected/missing package files: ' + repr(sorted(actual ^ expected)))
    manifest = json.loads((package / 'tools/disc-manifest.json').read_text())
    forbidden_hashes = {row['sha256'] for row in manifest['files'].values()}
    forbidden_hashes.add('c6f0c99ad57eafa2fc9caedb5ab2a7b192ff5fdde7e5faddc5e62a1730f044d1')
    files = {}
    forbidden = {'.iso', '.xbe', '.ent', '.geo', '.saf', '.lst', '.xmv', '.wmv', '.wav',
                 '.ucmat', '.pdb', '.log', '.dmp', '.pyc', '.csv'}
    for name in sorted(actual):
        path = package / name
        if path.suffix.lower() in forbidden or path.is_symlink():
            raise ValueError('Forbidden release payload: ' + name)
        digest = sha(path)
        if digest in forbidden_hashes:
            raise ValueError('Original disc file in release: ' + name)
        files[name] = digest
    info = json.loads((package / 'tools/runtime-manifest.json').read_text())
    if not isinstance(info, dict) or not isinstance(info.get('files'), dict) or set(info['files']) != set(RUNTIME_FILES):
        raise ValueError('Runtime package file list changed')
    for name, digest in info['files'].items():
        if files['runtime/' + name] != digest:
            raise ValueError('Runtime package identity changed')
    art = info.get('artwork', {})
    if not isinstance(art, dict) or len(art) > 512:
        raise ValueError('Invalid runtime artwork map')
    seen = set()
    for name, digest in art.items():
        if not isinstance(name, str) or not name.startswith('hd-ui/'):
            raise ValueError('Invalid runtime artwork path')
        artwork_name(name[6:])
        if (name.lower() in seen or not valid_hash(digest) or files.get('runtime/' + name) != digest or
                not 0 < (package / 'runtime' / name).stat().st_size <= ARTWORK_LIMIT):
            raise ValueError('Runtime artwork identity changed')
        seen.add(name.lower())
    if {name for name in actual if name.startswith('runtime/hd-ui/')} != {'runtime/' + name for name in art}:
        raise ValueError('Unlisted restored artwork in package')
    seed = shader_seed_validator().validate(package / 'runtime/shader-seed', info.get('shader_seed', {}))
    if {name for name in actual if name.startswith('runtime/shader-seed/')} != {
            'runtime/shader-seed/' + name for name in seed}:
        raise ValueError('Unlisted shader seed in package')
    spanish = info.get('spanish', {})
    if not isinstance(spanish, dict) or (spanish and set(spanish) != SPANISH_FILES):
        raise ValueError('Invalid bundled Spanish file map')
    if {name for name in actual if name.startswith('runtime/spanish/')} != {
            'runtime/' + name for name in spanish}:
        raise ValueError('Unlisted Spanish payload in package')
    if spanish:
        _, expected_spanish = spanish_sources(package / 'runtime/spanish')
        if (any(not valid_hash(value) for value in spanish.values()) or
                spanish != expected_spanish):
            raise ValueError('Bundled Spanish identity changed')
    return {'version': VERSION, 'strict_allowlist': True, 'files': files,
            'disc_payload_hashes_checked': len(forbidden_hashes),
            'original_payload_files_in_package': 0,
            'restored_artwork_files': len(art),
            'shader_seed_files': len(seed),
            'spanish_text_files': len(spanish),
            'note': 'Generated runtime code, custom edition artwork, approved restored PNGs, translated Spanish strings and the validated portable shader seed are permitted. No original disc payloads, imported assets, saves, settings, driver-specific pipeline caches, logs or diagnostic executables are packaged. Live appearance is a separate check.'}


def assemble(runtime, expected_sha256, output, shader_seed=None):
    if sha(runtime) != expected_sha256:
        raise ValueError('Runtime does not match the explicitly selected release candidate')
    if output.exists():
        raise ValueError('Choose a fresh package directory; existing packages are never overwritten.')
    plan = sources(runtime)
    restored, artwork = artwork_sources(ARTWORK)
    plan.update(restored)
    spanish_plan, spanish = spanish_sources(SPANISH)
    plan.update(spanish_plan)
    seed = {}
    if shader_seed is None and SHADER_SEED.exists():
        shader_seed = SHADER_SEED
    if shader_seed is not None:
        seed_plan, seed = shader_seed_sources(shader_seed)
        plan.update(seed_plan)
    for name, source in plan.items():
        if not source.is_file():
            raise ValueError('Missing package dependency: ' + str(source))
    output.mkdir(parents=True)
    for name, source in plan.items():
        dest = output / name
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, dest)
    (output / 'tools/python/python312._pth').write_text('python312.zip\n.\n..\n')
    info = {'version': VERSION, 'files': {
        name: sha(plan['runtime/' + name]) for name in RUNTIME_FILES}, 'artwork': artwork,
        'shader_seed': seed, 'spanish': spanish}
    (output / 'tools/runtime-manifest.json').write_text(json.dumps(info, indent=2) + '\n')
    (output / 'licenses/DEPENDENCIES.txt').write_text(
        'XboxXBERecomp by TGP482: https://github.com/TGP482/XboxXBERecomp (MIT).\n'
        'Earlier port and retained import/input helpers: xboxrecomp by sp00nz (MIT); associated notices included.\n'
        'SDL3, Dear ImGui and toml++ notices: XboxXBERecomp-third-party.txt.\n'
        'libusb 1.0.29 (LGPL-2.1-or-later): unmodified upstream Windows DLL; license and corresponding source archive included. https://github.com/libusb/libusb/releases/tag/v1.0.29\n'
        'Python3.12 and included OpenSSL: tools/python/LICENSE.txt. zlib: zlib.txt.\n'
        'Microsoft Visual C++2015-2022 runtime: app-local DLLs from the official VS2022 Redist folder; Microsoft-Visual-Cpp-Redistribution.txt and accompanying Microsoft notices.\n'
        'Direct3D12, XAudio2 and other Windows components are supplied by Windows.\n'
        'The launcher cover/icon is custom Burner Edition artwork. Approved restored HD UI PNGs are included under runtime/hd-ui; original disc files are still supplied by the user.\n'
        'Spanish game-text translations by ZARF with supplemental Burner Edition translations are bundled as strings under runtime/spanish; see runtime/spanish/credits.txt. The launcher rebuilds translated archives from the user-provided ISO. English voices are retained.\n')
    allowlist = set(plan) | {'tools/python/python312._pth', 'tools/runtime-manifest.json',
                            'licenses/DEPENDENCIES.txt'}
    result = audit(output, allowlist, require_launchers=False)
    evidence = output.parent / (output.name + '-allowlist.json')
    evidence.write_text(json.dumps({'files': sorted(allowlist)}, indent=2) + '\n')
    (output.parent / (output.name + '-asset-audit.json')).write_text(json.dumps(result, indent=2) + '\n')
    print(output)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--runtime', type=Path)
    parser.add_argument('--expected-runtime-sha256')
    parser.add_argument('--shader-seed', type=Path, help='Validated seed directory; defaults to packaging/shader-seed when present')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--audit', action='store_true')
    args = parser.parse_args()
    if args.audit:
        allowlist = json.loads((args.output.parent / (args.output.name + '-allowlist.json')).read_text())['files']
        result = audit(args.output, allowlist)
        (args.output.parent / (args.output.name + '-asset-audit.json')).write_text(json.dumps(result, indent=2) + '\n')
        print('PASS: exact package allowlist and original disc payload scan')
    else:
        if args.runtime is None or not args.expected_runtime_sha256:
            parser.error('--runtime and --expected-runtime-sha256 are required for assembly')
        assemble(args.runtime.resolve(), args.expected_runtime_sha256, args.output.resolve(), args.shader_seed)


if __name__ == '__main__':
    main()

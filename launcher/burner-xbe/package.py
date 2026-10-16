"""Assemble the tested D3D12 release from an explicit asset-free allowlist."""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
import sys

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
VERSION = '0.04'
SDK = REPO / 'build/upstream-review-xboxxbe-20261014'
RUNTIME_NAME = 'UrbanChaosXbeTrial.exe'
RUNTIME_FILES = (RUNTIME_NAME, 'zlib1.dll', 'libusb-1.0.dll', 'msvcp140.dll', 'vcruntime140.dll', 'vcruntime140_1.dll')
VS = Path('C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools')
CRT = VS / 'VC/Redist/MSVC/14.44.35112/x64/Microsoft.VC143.CRT'
PYTHON_FILES = ('python.exe', 'python312.dll', 'python3.dll', 'python312.zip',
                'vcruntime140.dll', 'vcruntime140_1.dll', '_hashlib.pyd', 'libcrypto-3.dll', 'LICENSE.txt')
LAUNCHERS = {'Burner Edition Setup.exe', 'Play Burner Edition.exe'}


def sha(path):
    with path.open('rb') as file:
        return hashlib.file_digest(file, 'sha256').hexdigest()


def sources(runtime):
    result = {
        'tools/import_game.py': HERE / 'import_game.py',
        'tools/disc.py': HERE / 'disc.py',
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
    actual = {p.relative_to(package).as_posix() for p in package.rglob('*') if p.is_file()}
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
    for name, digest in info['files'].items():
        if files['runtime/' + name] != digest:
            raise ValueError('Runtime package identity changed')
    return {'version': VERSION, 'strict_allowlist': True, 'files': files,
            'disc_payload_hashes_checked': len(forbidden_hashes),
            'original_payload_files_in_package': 0,
            'note': 'Runtime generated code and custom edition artwork are permitted. No imported assets, saves, settings, caches, logs or diagnostic executables are packaged. Live startup and audible/visual acceptance are separate checks.'}


def assemble(runtime, expected_sha256, output):
    if sha(runtime) != expected_sha256:
        raise ValueError('Runtime does not match the explicitly selected release candidate')
    if output.exists():
        raise ValueError('Choose a fresh package directory; existing packages are never overwritten.')
    plan = sources(runtime)
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
        name: sha(plan['runtime/' + name]) for name in RUNTIME_FILES}}
    (output / 'tools/runtime-manifest.json').write_text(json.dumps(info, indent=2) + '\n')
    (output / 'licenses/DEPENDENCIES.txt').write_text(
        'XboxXBERecomp by TGP482: https://github.com/TGP482/XboxXBERecomp (MIT).\n'
        'Earlier port and retained import/input helpers: xboxrecomp by sp00nz (MIT); associated notices included.\n'
        'SDL3, Dear ImGui and toml++ notices: XboxXBERecomp-third-party.txt.\n'
        'libusb 1.0.29 (LGPL-2.1-or-later): unmodified upstream Windows DLL; license and corresponding source archive included. https://github.com/libusb/libusb/releases/tag/v1.0.29\n'
        'Python3.12 and included OpenSSL: tools/python/LICENSE.txt. zlib: zlib.txt.\n'
        'Microsoft Visual C++2015-2022 runtime: app-local DLLs from the official VS2022 Redist folder; Microsoft-Visual-Cpp-Redistribution.txt and accompanying Microsoft notices.\n'
        'Direct3D12, XAudio2 and other Windows components are supplied by Windows.\n'
        'The launcher cover/icon is the existing custom Burner Edition artwork, not an extracted disc file.\n')
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
        assemble(args.runtime.resolve(), args.expected_runtime_sha256, args.output.resolve())


if __name__ == '__main__':
    main()

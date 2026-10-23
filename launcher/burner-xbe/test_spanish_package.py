"""Check the bundled Spanish allowlist without shipping or loading a game archive."""
from pathlib import Path
import json
import shutil
import sys
import tempfile

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
import package as packager


def rejected(action):
    try:
        action()
    except (ValueError, FileNotFoundError):
        return
    raise AssertionError('Invalid Spanish package was accepted')


def main():
    plan, identities = packager.spanish_sources(packager.SPANISH)
    assert set(identities) == packager.SPANISH_FILES
    assert set(plan) == {'runtime/' + name for name in identities}
    assert all(path.suffix in ('.json', '.txt') for path in plan.values())
    with tempfile.TemporaryDirectory(prefix='spanish-package-', dir=packager.REPO / 'build') as temp:
        package = Path(temp)
        runtime = package / 'runtime'
        spanish = runtime / 'spanish'
        tools = package / 'tools'
        tools.mkdir()
        shutil.copytree(packager.SPANISH, spanish)
        (tools / 'disc-manifest.json').write_text(json.dumps({'files': {}}))
        for name in packager.RUNTIME_FILES:
            (runtime / name).write_bytes(('fixture-only-' + name).encode())
        info = {'version': packager.VERSION, 'files': {
            name: packager.sha(runtime / name) for name in packager.RUNTIME_FILES},
            'spanish': identities}
        manifest = tools / 'runtime-manifest.json'
        manifest.write_text(json.dumps(info))
        allowed = {path.relative_to(package).as_posix() for path in package.rglob('*') if path.is_file()}
        def audit():
            return packager.audit(package, allowed, require_launchers=False)
        assert audit()['spanish_text_files'] == 2
        before = (spanish / 'text.json').read_bytes()
        for broken in (None, [], {}, {'spanish/../text.json': '0' * 64},
                       dict(identities, **{'spanish/text.json': '0' * 64})):
            manifest.write_text(json.dumps(dict(info, spanish=broken)))
            rejected(audit)
        manifest.write_text(json.dumps(info))
        (spanish / 'text.json').write_bytes(b'null')
        rejected(audit)
        manifest.write_text(json.dumps(dict(info, spanish=dict(identities,
            **{'spanish/text.json': packager.sha(spanish / 'text.json')}))))
        rejected(audit)  # A matching hash cannot make a non-text bundle valid.
        (spanish / 'text.json').write_bytes(before)
        manifest.write_text(json.dumps(info))
        (spanish / 'credits.txt').write_bytes(b'altered credits')
        rejected(audit)
        shutil.copy2(packager.SPANISH / 'credits.txt', spanish / 'credits.txt')
        extra = spanish / 'original.ent'
        extra.write_bytes(b'forbidden archive fixture')
        rejected(lambda: packager.spanish_sources(spanish))
        rejected(lambda: packager.audit(package, allowed | {'runtime/spanish/original.ent'},
                                        require_launchers=False))
        extra.unlink()
        (spanish / 'credits.txt').unlink()
        rejected(audit)
        shutil.copy2(packager.SPANISH / 'credits.txt', spanish / 'credits.txt')
        assert audit()['original_payload_files_in_package'] == 0
    print('PASS: bundled Spanish exact files, text schema, hashes and original-archive rejection')


if __name__ == '__main__':
    main()

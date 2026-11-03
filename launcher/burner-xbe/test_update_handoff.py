"""Real Windows helper handoff and restart; synthetic package, no game or network."""
from pathlib import Path
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
import time
import unittest

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
sys.path.insert(0, str(HERE))
import release_update


@unittest.skipUnless(os.name == 'nt', 'Windows independent interpreter handoff')
class HandoffTests(unittest.TestCase):
    def test_child_waits_for_parent_then_applies_and_restarts(self):
        # Keep fixtures inside the workspace and never operate on an installed copy.
        base = REPO / 'build/update-handoff-fixtures'
        base.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=base) as temporary:
            root = Path(temporary)
            tools = root / 'tools'
            tools.mkdir()
            from test_release_update import package
            package(root, '0.09')
            import package as packager
            for name in packager.PYTHON_FILES:
                destination = tools / 'python' / name
                destination.parent.mkdir(exist_ok=True)
                shutil.copy2(REPO / '.tools/python' / name, destination)
            (tools / 'python/python312._pth').write_text('python312.zip\n.\n..\n')
            for name in ('release_check.py', 'release_download.py', 'release_update.py'):
                shutil.copy2(HERE / name, tools / name)
            source = root / 'restart.cs'
            source.write_text('using System;using System.IO;class P{static void Main(string[] a){File.WriteAllText(Path.Combine(AppDomain.CurrentDomain.BaseDirectory,"restarted.txt"),String.Join(" ",a));}}')
            subprocess.run([r'C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe',
                '/nologo', '/target:exe', '/out:' + str(root / 'Burner Edition Setup.exe'), str(source)], check=True, capture_output=True)
            marker = root / 'GameData/xbe-runtime/hdd/save.bin'
            marker.parent.mkdir(parents=True)
            marker.write_bytes(b'preserved player save')
            pointer = root / 'installation.json'
            pointer.write_text('{"version":1,"data":"GameData"}')
            files = {p.relative_to(root).as_posix(): release_update.digest(p) for p in root.rglob('*')
                     if p.is_file() and release_update.owned(p.relative_to(root).as_posix(), 'windows')
                     and p.name != 'package-manifest.json'}
            (root / 'package-manifest.json').write_text(json.dumps(dict(version='0.09', complete=True, files=files)))
            driver = root / 'driver.py'
            driver.write_text('''from pathlib import Path
import json, shutil, sys, time
sys.path.insert(0, str(Path(__file__).parent / 'tools'))
import release_update as u, release_download as d
root=Path(__file__).parent
def downloaded(current, platform, scratch, notify, cancel):
    new=scratch/'new'; new.mkdir()
    for name in u.package_files(root, 'windows'):
        dest=new/name; dest.parent.mkdir(parents=True,exist_ok=True); shutil.copy2(root/name,dest)
    (new/'tools/import_game.py').write_bytes(b'# updated fixture')
    info=json.loads((new/u.MANIFEST).read_text()); info['version']='0.10'
    info['files']['tools/import_game.py']=u.digest(new/'tools/import_game.py')
    (new/u.MANIFEST).write_text(json.dumps(info))
    return {'version':'0.10'}
d.prepare=downloaded
with u.exclusive(root/'.burner.lock'):
    u.prepare(root,'0.09','windows')
    u.launch_worker(root,root/'GameData',restart=True)
    time.sleep(.2)
    assert (root/'tools/import_game.py').read_bytes()==b'old'
''')
            result = subprocess.run([str(tools / 'python/python.exe'), '-B', str(driver)],
                                    cwd=root, capture_output=True, text=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stderr)
            deadline = time.monotonic() + 20
            while not (root / 'restarted.txt').exists() and time.monotonic() < deadline:
                time.sleep(.05)
            self.assertTrue((root / 'restarted.txt').is_file(), 'Helper did not finish/restart')
            self.assertEqual((root / 'restarted.txt').read_text(), '--complete-update')
            self.assertEqual((tools / 'import_game.py').read_bytes(), b'# updated fixture')
            self.assertEqual(marker.read_bytes(), b'preserved player save')
            self.assertEqual(pointer.read_text(), '{"version":1,"data":"GameData"}')
            self.assertEqual(release_update.transaction(root)['phase'], 'committed')
            # Windows can briefly retain the exiting helper's DLL handles.
            for attempt in range(100):
                release_update.cleanup(root)
                if not (root / release_update.STAGE).exists():
                    break
                time.sleep(.05)
            self.assertFalse((root / release_update.STAGE).exists())


if __name__ == '__main__':
    unittest.main()

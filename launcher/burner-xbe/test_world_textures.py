"""Small installer regression: valid install, rejected archives and replacement rollback."""
from pathlib import Path
import contextlib
import hashlib
import io
import stat
import struct
import sys
import tempfile
from types import SimpleNamespace
import unittest.mock as mock
import zipfile
HERE = Path(__file__).resolve().parent
sys.path[:0] = [str(HERE), str(HERE.parents[1] / 'third_party/xboxrecomp/tools/xiso')]
from world_textures import install


def main():
    with tempfile.TemporaryDirectory(prefix='burner-textures-') as temp:
        root = Path(temp);data = root/'GameData';cancel = root/'cancel'
        payload = b'UCWT0001'+struct.pack('<III',4,4,3)+bytes(48)
        digest = hashlib.sha256(payload).hexdigest()
        name = 'world-textures/'+digest+'.uctex'
        manifest = ('4 4 12 8 '+'a'*64+' '+digest+'\n').encode()
        def archive(entries):
            path = root/'pack.zip'
            with zipfile.ZipFile(path,'w') as z:
                for n,b in entries:z.writestr(n,b)
            return path
        good = [('world-textures/manifest.txt',manifest),(name,payload)]
        def apply(entries):
            with contextlib.redirect_stdout(io.StringIO()):install(archive(entries),data,cancel)
        apply(good)
        target=data/'xbe-runtime/world-textures'
        assert (target/'manifest.txt').read_bytes()==manifest
        before={f.name:f.read_bytes() for f in target.iterdir()}
        def unchanged():assert {f.name:f.read_bytes() for f in target.iterdir()}==before
        def reject(entries):
            try:apply(entries)
            except (ValueError,KeyError,zipfile.BadZipFile):pass
            else:raise AssertionError('invalid archive accepted')
            unchanged()
        reject(good+[('../outside.txt',b'x')])
        reject(good+[(name,payload)])
        reject(good[:1])
        reject([good[0],(name,payload[:-1])])
        reject([('world-textures/manifest.txt',manifest+manifest),good[1]])
        bad=b'UCWT0001'+struct.pack('<III',3,4,3)+bytes(48)
        bh=hashlib.sha256(bad).hexdigest()
        reject([('world-textures/manifest.txt',manifest.replace(digest.encode(),bh.encode())),('world-textures/'+bh+'.uctex',bad)])
        rename=Path.rename
        def fail_publish(self,dest):
            if self.name.startswith('.textures-') and not self.name.startswith('.textures-old-'):raise OSError('injected publication failure')
            return rename(self,dest)
        with mock.patch.object(Path,'rename',fail_publish):
            try:apply(good)
            except OSError:pass
            else:raise AssertionError('publication unexpectedly succeeded')
        unchanged()
        cancel.touch()
        try:apply(good)
        except Exception:pass
        else:raise AssertionError('cancel ignored')
        unchanged();cancel.unlink();apply(good);unchanged()
        assert not list((data/'xbe-runtime').glob('.textures-*'))
        original_stat = Path.lstat
        def redirected_stat(path, *args, **kwargs):
            if path == redirected:
                return SimpleNamespace(st_mode=stat.S_IFDIR,
                                       st_file_attributes=stat.FILE_ATTRIBUTE_REPARSE_POINT)
            return original_stat(path, *args, **kwargs)
        redirected = root
        with mock.patch.object(Path, 'lstat', redirected_stat):
            apply(good)
        unchanged()
        for redirected in (data, data/'xbe-runtime', target):
            with mock.patch.object(Path, 'lstat', redirected_stat):
                reject(good)
        unchanged()
    print('PASS optional texture install, traversal/checksum/duplicate/mipmap rejection, cancellation and rollback')

if __name__=='__main__':main()

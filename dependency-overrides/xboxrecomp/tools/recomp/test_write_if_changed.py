"""Exercise production helper and verify every translator writer uses it."""
import ast
import os
import tempfile
from pathlib import Path
from .translator import write_if_changed

def test_write_if_changed():
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / 'chunk.c'
        assert write_if_changed(path, 'a\n') is True
        assert path.read_text(encoding='utf-8') == 'a\n'
        os.utime(path, ns=(1000000000,1000000000))
        before = path.stat().st_mtime_ns
        assert write_if_changed(path, 'a\n') is False
        assert path.stat().st_mtime_ns == before
        assert write_if_changed(path, 'b\n') is True
        assert path.read_text(encoding='utf-8') == 'b\n'
        assert path.stat().st_mtime_ns != before
        path.write_bytes(b'b\r\n')
        before = path.stat().st_mtime_ns
        assert write_if_changed(path, 'b\n') is False
        assert path.stat().st_mtime_ns == before
        path.write_bytes(b'\xff')
        try:
            write_if_changed(path, 'replacement')
        except UnicodeError:
            pass
        else:
            raise AssertionError('must not overwrite unreadable source')
        assert path.read_bytes() == b'\xff'
    source = Path(__file__).with_name('translator.py').read_text(encoding='utf-8')
    tree = ast.parse(source)
    writer = next(node for node in tree.body if isinstance(node,ast.ClassDef) and node.name=='BatchTranslator')
    calls = [node for node in ast.walk(writer) if isinstance(node,ast.Call)]
    assert sum(isinstance(n.func,ast.Name) and n.func.id=='write_if_changed' for n in calls)==5
    assert not any(isinstance(n.func,ast.Name) and n.func.id=='open' and len(n.args)>1 and isinstance(n.args[1],ast.Constant) and 'w' in str(n.args[1].value) for n in calls)

if __name__ == '__main__':
    test_write_if_changed()
    print('PASS: generated text, mtimes, CRLF, read-failure preservation, five writer routes')

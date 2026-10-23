"""Portable language setting, Spanish availability and unrelated-settings preservation."""
from pathlib import Path
import tempfile
import sys
import contextlib
import io
import json
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parent))
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'third_party/xboxrecomp/tools/xiso'))
import import_game as setup

with tempfile.TemporaryDirectory() as folder:
    data=Path(folder)
    assert setup.language_settings(data)=={'language':0,'spanish_available':False}
    setup.language_settings(data,2)
    config=data/'xbe-runtime/settings.toml'
    config.write_text("# Keep this comment\nlanguage = 1\nfps_cap = 120\n[other]\nlanguage = 'leave alone'\n",encoding='utf8')
    setup.language_settings(data,3)
    assert config.read_text()=="# Keep this comment\nlanguage = 3\nfps_cap = 120\n[other]\nlanguage = 'leave alone'\n"
    before=config.read_bytes()
    for value in (-1,5,True,4):
        try:setup.language_settings(data,value)
        except ValueError:pass
        else:raise AssertionError(value)
        assert config.read_bytes()==before
    def command(*args):
        output=io.StringIO()
        with patch.object(sys,'argv',['import_game.py','language','--data',str(data),*args]), contextlib.redirect_stdout(output):
            code=setup.main()
        return code,json.loads(output.getvalue())
    assert command()==(0,{'language':3,'spanish_available':False})
    assert config.read_bytes()==before
    assert command('--language','2')==(0,{'language':2,'spanish_available':False})
    assert setup.language_settings(data)['language']==2
    before=config.read_bytes()
    assert command('--language','4')[0]==1
    assert config.read_bytes()==before
    pack=data/'xbe-runtime/translations/es-zarf';pack.mkdir(parents=True)
    for i in range(27):(pack/f'{i}-01.ent').write_bytes(bytes(200))
    manifest=''.join(f'{i}-01.ent 200\n' for i in range(27))
    (pack/'manifest.txt').write_text(manifest)
    assert setup.language_settings(data,4)['spanish_available']
    assert setup.language_settings(data)['language']==4
    (pack/'manifest.txt').write_text(manifest.replace('1-01.ent','0-01.ent'))
    assert not setup.language_settings(data)['spanish_available']
    (pack/'manifest.txt').write_text(manifest.replace('1-01.ent','../bad.ent'))
    assert not setup.language_settings(data)['spanish_available']
    config.write_text('broken = [')
    try:setup.language_settings(data,0)
    except ValueError:pass
    else:raise AssertionError('Malformed settings must not be overwritten')
    assert config.read_text()=='broken = ['
print('PASS: portable language selection, missing/corrupt pack, path names, atomic settings preservation')

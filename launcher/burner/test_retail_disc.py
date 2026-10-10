"""Verify the retail variant through the packaged importer without touching user data."""
from pathlib import Path
import sys,json,io
sys.dont_write_bytecode=True
R=Path(__file__).resolve().parents[2]
P=R/'build/burner-edition-0.02/package'
sys.path.insert(0,str(P/'tools'))
import import_game as w
w.ROOT=P
m=json.loads((P/'tools/disc-manifest.json').read_text())
retail='c6f0c99ad57eafa2fc9caedb5ab2a7b192ff5fdde7e5faddc5e62a1730f044d1'
original=m['files']['Default.xbe']['sha256']
assert w.supported_hash('Default.xbe',original,original)
assert w.supported_hash('Default.xbe',retail,original)
assert not w.supported_hash('Default.xbe','0'*64,original)
assert not w.supported_hash('1-01.geo',retail,m['files']['1-01.geo']['sha256'])
data=P.parent/'retail-import-test'
w.import_iso(R/'Urban Chaos - Riot Response (USA, Europe) (En,Fr,De,Es,It).iso',data,data/'cancel')
assert w.ready(data)
assert w.sha(data/'installed/hybrid-assets/pc-hud.bin')==w.hud_import.PIXEL_SHA
for name,row in m['files'].items():
 assert w.supported_hash(name,w.sha(data/'installed/game_files'/name),row['sha256']),name
assert w.sha(data/'installed/game_files/Default.xbe')==retail
class Process:
 def __init__(self,*args,**kwargs):self.stderr=io.BytesIO()
 def wait(self):return 0
w.subprocess.Popen=Process
w.launch(data)
(P.parent/'retail-verification.json').write_text(json.dumps({'required_files_verified':len(m['files']),'full_import_and_video_conversion':True,'launch_validation_with_stub':True,'unknown_executable_rejected':True,'asset_hash_validation_preserved':True,'live_game_tested':False},indent=2))
print('PASS retail import, conversion, file hashes, launch validation and rejection checks')

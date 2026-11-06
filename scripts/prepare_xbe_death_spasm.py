"""Brief whole-body contractions on native dead/downed humanoid ragdolls."""
from pathlib import Path
import hashlib
import re
import struct

INCLUDE='#include "xbe_death_spasm.h"\n'
HOOKS=(('002A6119','TrialDeathSpasmActivated'),('002A5E7C','TrialDeathSpasmStep'))
PINS=(
 (71,0x2a5f00,546,'1f8d22ba5a873e38d8c5c269e87b4f28724b0755f3089b3e82578f664cd5cfeb','dbdab212c9879c01898963ed9f6418faaccd85deb4041c1c1c62d8ef2da52d64'),
 (71,0x2a5da0,231,'16aee95dbd1452a1ee4e3505e9357e4064e832050b5bd074142c4fa10e33892e','5f73b19b3dc7898c7025f25dc1e00956ff859ab0f62f40cd23045da10e0f92bb'),
 (82,0x2f5240,184,'ed0f92bacb492661d3aa14779363518b752caf3384bc38c742c36b8b846598b0','4e3e40126c4c92cece4f62f61b0241e4204b4a44b493b8a44e7442d6c8da64d9'),
 (52,0x20a690,49,'bd1ea24c7a91afcee86193c1682f01a64a2dd2436c5565f9bbb4bffc658398a1','396a63c90c200eb1a0dae76f17cc61f63b4c0adc78837664b974e21d88379097'),
 (81,0x2ecaa0,42,'f9bb2125c6c82525a32b615604a318754adc2a3034e8f3eb2b3b52a5691cf71a','1ba92af13bd1dbd800af469b371b775a9f0d5aa5868ca1253423f08e8531e0e7'),
)
def body(s,a):return re.search(rf'X86_FUNC\(__imp__sub_{a:08X}\) \{{.*?^\}}',s,re.M|re.S)[0]
def clean(s):
    s=s.replace(INCLUDE,'')
    for _,fn in HOOKS:s=s.replace(f'\t{fn}(ctx,base);\n','')
    return s
def unprepare(area):
    p=Path(area)/'generated/game/x86_recomp.71.cpp'
    if p.exists():
        s=p.read_text();t=clean(s)
        if s!=t:p.write_text(t)
def prepare(area):
    area=Path(area);local=Path(__file__).parent
    x=(area/'assets/Default.xbe').read_bytes();base=struct.unpack_from('<I',x,0x104)[0]
    count,table=struct.unpack_from('<II',x,0x11c)
    sections=[struct.unpack_from('<6I',x,table-base+i*56)for i in range(count)]
    def raw(a,n):
        s=next(s for s in sections if s[1]<=a and a+n<=s[1]+s[4]);o=s[3]+a-s[1];return x[o:o+n]
    originals={}
    sources={u:clean((area/f'generated/game/x86_recomp.{u}.cpp').read_text())for u,_,_,_,_ in PINS}
    for u,a,n,rh,sh in PINS:
        assert hashlib.sha256(raw(a,n)).hexdigest()==rh,hex(a)
        originals[a]=body(sources[u],a)
        assert hashlib.sha256(originals[a].encode()).hexdigest()==sh,hex(a)
    assert raw(0x44da54,16)==b'enemy of player\0'
    assert struct.unpack('<I',raw(0x49a8b8+0x64,4))[0]==0x2f5240
    s=sources[71]
    for address,fn in HOOKS:
        anchor=f'\t// {address} ';assert s.count(anchor)==2,address # Also the native resume entry.
        s=s.replace(anchor,f'\t{fn}(ctx,base);\n'+anchor)
    (area/'generated/game/x86_recomp.71.cpp').write_text(INCLUDE+s)
    for name in ('xbe_death_spasm.h','xbe_death_spasm.cpp'):
        (area/'src/common'/name).write_text((local/name).read_text())
    parts=[originals[a].replace('__imp__sub_','sub_',1)for a in (0x2f5240,0x20a690,0x2ecaa0)]
    fixture=(local/'test_xbe_death_spasm.cpp').read_text().replace('// NATIVE_BODIES','\n'.join(parts))
    (area/'src/compat/test_death_spasm.cpp').write_text(fixture)

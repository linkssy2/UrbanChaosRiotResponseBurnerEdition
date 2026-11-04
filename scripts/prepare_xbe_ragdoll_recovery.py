"""Bound ragdoll positional recovery to the existing 60 Hz solver rate."""
from pathlib import Path
import hashlib
import json
import re
import struct

INCLUDE='#include "xbe_ragdoll_recovery.h"\n'
HOOKS={0x2f4af0:4,0x31e840:4,0x34a3c0:20}
PINS=(
 (92,0x345ce0,12,'c1b3b8b55768c51a608f0d940d2584203df659af4a7f7c4d829a589d374c419d','5defeff29329828c8d65ca4e390652a5b1ba9e04eb004b32e6d087f4d9a55288'),
 (92,0x345cf0,15,'c65fc7cc4604212243ccf3e5213c17fa1e46f200b07f5dfc79887756270f5913','9fdf129321299bad6121bf98d74a77bba1a03ae9c14c1f5cf7af2753efd0893c'),
 (82,0x2f4af0,765,'e1cd791a52a7df9a9b0c34ab9e64b01834a75097aa9cd24bb7688d0ae0e8866a','5379e3310fe596f8f2f7c44c0101078367dfe6b356339094cb690d5c23c5fb19'),
 (87,0x31e840,826,'060bb7e6616d29ca8bae93617f6b8b6d749f1a73f6e67d4039ab4b77aaa078d0','349836b0b8c94da0d7269797691e9265a4ba858854b883913a328ac7ea1e0879'),
 (94,0x34a3c0,3099,'ee60bb9d246d238e8f458200c11495801a67a69b1c686ebab5a5f5a00a9e4bae','2fef5f7abaea3cdd530e0db562dfb10ad51cbe67fc9b4ad9ad6b83690fde5932'),
 (92,0x345d30,59,'d87e2ea9517d8aeb2b176ded8c02fe55f73b8843bcaad71f4887e4544a7ffd97','c447c82de7ef8a80880f69d848e5d133c30dd2da54b4a484787be36e28f0c436'),
 (92,0x345ef0,327,'2db715b0566144dba9b08fa367daa5d1cf9d45a6974141e220d8107eb3a382ea','bf83b9252092e122639c52466ba02d40a8b947084a41fd7b022ffdbfdccef3da'),
 (92,0x3463b0,348,'85f401d252f4755cb6640ea27731f06e251dde82aa98637308c5ff94fc5e7156','f3b847b40873e3387a2a9a13531bcfbf7261888456ec31a9b35102f5ac152755'),
 (92,0x346510,503,'9c0366305330cea9096761f79d20041a418833ae34ef3ac6eec0fe6ac6cd9bac','d4d2141c269e7323bfc81071efa53ab4edbd6c56f5aa26ba4b716546e282eacf'),
 (92,0x346040,400,'2997c2dd96358ea8a6fb573341256c134ddb0d5b3e5e859621556c410c918a0c','e91b6c81d5df49ff3f3eb4c0c71d30bf83f6c948f4de6f42c03f6f1136587a8c'),
 (86,0x31e450,536,'5a9367bd6a0066b318e72793a84b2a4c7dd4146855aa7ef0c4fa9b564479f312','60f3b84150b1b3271b00b41e45f147861cd07d081a7cb4dd027edca78571b73c'),
 (101,0x389510,146,'d1e48af1f0e7bc4393e7ba5dd7184dd1af13d26a7bfefc7baaad277b5114f146','c92933167469169bdae36a927d1355931c379545b60d38fca3dc59d3de038f2d'),
 (93,0x348250,8401,'4ac0ffd07305ce3dddc2557e0ca7aeba392b97373df000c6fb23c130ce88b2d1','751e5f4cc81e7f5c363a7e80383dcab0ca699a85350ca248bcf0de0f2780911c'),
)
def hook(argument):return f'\ttrial_ragdoll_recovery::Scope recovery(ctx,base,{argument});\n'
def clean(s):
    for text in (INCLUDE,hook(4),hook(20)):s=s.replace(text,'')
    return s
def body(s,a):return re.search(rf'X86_FUNC\(__imp__sub_{a:08X}\) \{{.*?^\}}',s,re.M|re.S).group()
def unprepare(area):
    for u in (82,87,94):
        p=Path(area)/f'generated/game/x86_recomp.{u}.cpp'
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
    sources={u:clean((area/f'generated/game/x86_recomp.{u}.cpp').read_text())for u in {p[0]for p in PINS}}
    originals={}
    for u,a,n,rh,sh in PINS:
        assert hashlib.sha256(raw(a,n)).hexdigest()==rh,hex(a)
        originals[a]=body(sources[u],a)
        assert hashlib.sha256(originals[a].encode()).hexdigest()==sh,hex(a)
    # Retail ragdoll body property 1=4; constraint instance exposes its two bodies.
    assert raw(0x2a64f9,10).hex()=='6a046a0155e81dccffff'
    assert raw(0x2fffa0,6).hex()=='8b70108b4014'
    parts=[]
    for u,a,_,_,_ in PINS:
        if a in HOOKS:
            s=sources[u];anchor=f'X86_FUNC(__imp__sub_{a:08X}) {{\n';assert s.count(anchor)==1
            s=s.replace(anchor,anchor+hook(HOOKS[a]),1)
            (area/f'generated/game/x86_recomp.{u}.cpp').write_text(INCLUDE+s)
            parts.extend((originals[a].replace(f'__imp__sub_{a:08X}',f'Original_{a:08X}',1),
                          body(s,a).replace(f'__imp__sub_{a:08X}',f'Fixed_{a:08X}',1)))
        else:parts.append(originals[a].replace('__imp__sub_','sub_',1))
    (area/'src/common/xbe_ragdoll_recovery.h').write_text((local/'xbe_ragdoll_recovery.h').read_text())
    declarations=sorted(set(re.findall(r'\bsub_[0-9A-F]{8}(?=\(ctx)','\n'.join(parts))))
    fixture=(local/'test_xbe_ragdoll_recovery.cpp').read_text()
    fixture=fixture.replace('// NATIVE_DECLARATIONS','\n'.join('X86_FUNC('+n+');'for n in declarations))
    fixture=fixture.replace('// NATIVE_BODIES','\n'.join(parts))
    (area/'src/compat/test_ragdoll_recovery.cpp').write_text(fixture)
    from migration_physics_joint_solver_checks.assets import emit
    out=area/'assets/ragdoll-recovery';out.mkdir(exist_ok=True)
    evidence=emit(local.resolve().parent,out,local/'ragdoll_asset_fixture.json')
    (out/'sources.json').write_text(json.dumps(evidence,indent=2)+'\n')

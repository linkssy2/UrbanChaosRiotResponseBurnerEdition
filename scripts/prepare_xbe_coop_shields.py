"""Separate the two authored training shield pickups only for local co-op."""
from pathlib import Path
import hashlib
import re
import struct

PINS=(
    (0x2189f8,16,'a8275b5a865e059cdccbbc51ef1a87f0f636aaf8adecc4c77803c4fdf146cef4'),
    (0x211450,443,'10cb93372ebc55ed35f3604abfdd3c3861c40deb4b54d0edd6b020f361314bb7'),
    (0x217bb0,1122,'250e23dfc6dcd30bda45783d8f69320dca9464fb55cac6022b68396af4bf2b25'),
    (0x2187a0,598,'bf841bb841e82e7a2c4ef27ca9c0fa02fee8671d1ecc1308f4e3adf6bc0434cf'),
    (0x2d3830,177,'f7087696b68bf23598dbb649ebf75bde130ed6b8fd8fc33903f2b5295b934adb'),
    (0xd0c30,488,'adb1022a9cd5d5eaf16a7528de5cc1a19332ecdedfb0a83a6e685cab92662a96'),
    (0x2084e0,10,'b1d7381cf9278dc64c2869ef57a6e9226d9142bfd7a282d0650c3cf953ec6cbe'),
    (0x210770,14,'73da5582476511b302e6a3fdc72096dd21a06e9c3f00a14a9110858609f2a0b4'),
    (0x210320,33,'d3f9b2ea973a499cc697b8256d5e90e0e30411675dd5c887e48a88b48277e1c3'),
    (0x2107c0,47,'03ec68a2d700219a9bc109f4b02947a80afc5a805ace0554e50c6d486b420198'),
    (0x46fc30,4,'48bf2e229c8d9bb7b1d6d758162e8cb4e97c0f6d1e880b5b1383978577b0f07d'),
)
HOOKS={
    21:[('\t// 000D0C74 test al, al','\tTrialCoopShieldPermission(ctx,base);\n')],
    53:[('X86_FUNC(__imp__sub_00211450) {\n','\tif(TrialCoopShieldOverlap(ctx,base))return;\n')],
    54:[('X86_FUNC(__imp__sub_00217BB0) {\n','\tif(TrialCoopShieldCommand(ctx,base))return;\n')],
}
BODIES=(
    (52,'002084E0','ac534c14e440f547fc4b3995c429dbe70fd8a65f4fe25d2ff22b01f3785ee350'),
    (53,'002107C0','68dd06077ca8eadb78393eda7b46c0ff226130fb2311d9f0c7bdfac954f86c04'),
    (53,'00210770','8084c2aa9896dd269b56d0ef48289aded47d7fbffcd0ffba73a1a4f6584014ea'),
    (53,'00210320','ac2e02783d00f2676007070d204ae64e9b523bef2d3f2725bfc31522e603cc41'),
    (53,'00211450','1bf0103415fa15e06d39a54414c6fa974e6a7013130e1154707a8593dd0fe46a'),
    (77,'002D3830','263100910a41c69fd455a0c1f1f67feb57b5ab7ead589b4e792370a7b6e70385'),
    (54,'002187A0','baa6b64bc65600b94812c89b2b53aee5e9fdd809c110b6b9c61f3b16b9566f07'),
)
REGIONS=(
    (21,'000D0C54','000D0C78','71fd48e4e2e9643c65e1adf9791ae0721abc6d49e310e9942c9f5a4530f9ea87','Permission'),
    (54,'00217C60','00217CA3','8139ea57cbfe1dbeadea1674540c5238242b3ab1d9a52365445dcd977a408b03','SetFlag'),
    (54,'00217CA3','00217CDD','06981dcf107dc073865e27d34b066d85a26eb086e4c0854d5ecb819b74ed4616','DisableTrigger'),
)
INCLUDE='#include "xbe_coop_shields.h"\n'

def clean(source,seams):
    source=source.replace(INCLUDE,'')
    for _,hook in seams:source=source.replace(hook,'')
    return source

def unprepare(area):
    for unit,seams in HOOKS.items():
        p=Path(area)/f'generated/game/x86_recomp.{unit}.cpp'
        if not p.exists():continue
        before=p.read_text();after=clean(before,seams)
        if after!=before:p.write_text(after)

def region(source,first,last):
    return source[source.index('\t// '+first+' '):source.index('\t// '+last+' ')]

def fixture(name,body):
    if name.endswith('Permission'):
        tail='\t++permissions;return;\nloc_000D0C99:\n\t++blocked;\n'
    else:tail='loc_00217FE8:\n\tassert(false);\n'
    return 'X86_FUNC('+name+') {\n\tX86_LOAD_FLAGS();\n\tuint32_t ca=0,cb=0;\n'+body+tail+'}\n'

def authored_scripts(area,local):
    # Verify the maintained literals against the original two level scripts.
    # This reads the local original disc asset, never packages it or rewrites it.
    import ast
    import sys
    sys.path.insert(0,str(local/'native_level_checks'))
    from inspect_level import load
    source=(local/'native_level_logic_checks/run.py').read_text()
    node=next(n for n in ast.parse(source).body if isinstance(n,ast.FunctionDef) and n.name=='reference')
    namespace={};exec(compile(ast.Module(body=[node],type_ignores=[]),'<level-tail-proof>','exec'),namespace)
    r=load(area/'assets/12-01.ent');summary=r.level();triggers,_=namespace['reference'](r,summary)
    production=(local/'xbe_coop_shields.cpp').read_text()
    literal=re.search(r'constexpr uint32_t scripts\[2\]\[31\]=\{(.*?)\n\};',production,re.S).group(1)
    arrays=re.findall(r'\{([^{}]+)\}',literal)
    for i,ordinal in enumerate((30,32)):
        t=triggers[ordinal];assert t['kind']==37 and t['shape']==3
        assert len(t['scripts'])==1 and t['scripts'][0][0]=='give shield'
        data=struct.pack('<'+'I'*len(arrays[i].split(',')),*[int(v.strip(),0)for v in arrays[i].split(',')])
        assert data==t['scripts'][0][1],ordinal
        prop=r.entities[(585,578)[i]]
        assert prop['kind']=='BasicProp' and prop['asset'].lower().endswith('eqp_shield\\eqp_shield.amf')

def prepare(area):
    area=Path(area);local=Path(__file__).parent
    xbe=(area/'assets/Default.xbe').read_bytes();base=struct.unpack_from('<I',xbe,0x104)[0]
    count,table=struct.unpack_from('<II',xbe,0x11c)
    sections=[struct.unpack_from('<6I',xbe,table-base+i*56)for i in range(count)]
    for va,size,digest in PINS:
        s=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4]);at=s[3]+va-s[1]
        assert hashlib.sha256(xbe[at:at+size]).hexdigest()==digest,hex(va)
    authored_scripts(area,local)
    sources={unit:clean((area/f'generated/game/x86_recomp.{unit}.cpp').read_text(),HOOKS.get(unit,[]))
             for unit in {u for u,_,_ in BODIES}|set(HOOKS)}
    test=(local/'test_xbe_coop_shields.cpp').read_text()
    for unit,address,digest in BODIES:
        body=re.search(r'X86_FUNC\(__imp__sub_'+address+r'\) \{.*?^\}',sources[unit],re.M|re.S).group()
        assert hashlib.sha256(body.encode()).hexdigest()==digest,address
        marker='// ORIGINAL_'+address;assert test.count(marker)==1
        test=test.replace(marker,body)
    extracted=''
    for unit,first,last,digest,name in REGIONS:
        body=region(sources[unit],first,last);assert hashlib.sha256(body.encode()).hexdigest()==digest,first
        extracted+=fixture('Original'+name,body)
        if name=='Permission':
            body=body.replace(HOOKS[21][0][0],HOOKS[21][0][1]+HOOKS[21][0][0])
            extracted+=fixture('Coop'+name,body)
    assert test.count('// ORIGINAL_SHIELD_REGIONS')==1
    test=test.replace('// ORIGINAL_SHIELD_REGIONS',extracted)
    for unit,seams in HOOKS.items():
        source=sources[unit]
        for anchor,hook in seams:
            assert source.count(anchor)==1,anchor
            source=source.replace(anchor,anchor+hook if anchor.startswith('X86_FUNC') else hook+anchor,1)
            assert source.count(hook)==1
        source=source.replace('#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+INCLUDE,1)
        (area/f'generated/game/x86_recomp.{unit}.cpp').write_text(source)
    for name in ('xbe_coop_shields.h','xbe_coop_shields.cpp'):
        (area/'src/common'/name).write_text((local/name).read_text())
    (area/'src/compat/test_coop_shields.cpp').write_text(test)

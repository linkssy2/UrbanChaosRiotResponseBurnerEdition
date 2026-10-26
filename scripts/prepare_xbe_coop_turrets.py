"""Give both local actors the helicopter's original mount/weapon events."""
from pathlib import Path
import hashlib
import re
import struct

PINS=(
    (0x199900,168,'069e197d274b9edce69d2a6e553d24013dc4dbe5d2bfeb0e425b5fb6616bdb61'),
    (0x199a10,832,'3a41151d5917e7f0da0afcbba74d2fa547b8ec1ba223162904ce400ff1b3e316'),
    (0x199d50,2302,'afd5ad06bee84ca5f6963901cfea20b00ab46310139a750145dba29660026417'),
    (0x1998b0,47,'caf4986ff6cafca5fe0324e9352650e6baa4c4b1aa9a1838ba6d182a1494fa20'),
    (0x13f1e0,47,'91441885621e51e304850a4c3b7c2d479ce21f441ab22600b158d2ceb1362292'),
    (0x1f90a0,56,'f7aa31f3736f27a11b0e94c7672dbb0a20a26401e7758b3140eb82cddbbef866'),
    (0xbd450,243,'0cafbb9208f385bc695dd3e5d01542ce88dd7245830284c6b5c3268bfe6d6efe'),
    (0xcc190,163,'a7419c24db603c6a7955758188d8787e1c211b8ceb107f2d76774d8fcf04e144'),
    (0xc8550,186,'f6061af5119826209bb2c24f7819e71eecb2d9975e8f2311b5d75ab5c287ff65'),
    (0xc8a8b,64,'1da7a2cdd69011cd7c59971939d766dd1b1f8bbca51ba3540f5ae4a20c4ee24c'),
    (0xc3f19,31,'51d02f13eaf5bd8340ef79053b8e2a4ef0203d9f21e4aaac0cca4e9c8788664f'),
    (0xd4cc0,168,'23f3f24c8300312e1023f26a7a9695e96f412811b5cd85f8690c7d5fc7ac3efc'),
    (0x3a370a,2,'52ee7bc49cb2b8661e54f8103f068838ea4bf5089db7626cad08a85a8a6dce66'),
    (0x3a7013,10,'33abef2690bec07cf1c1d23c39d2be2ea785b56125c81f6a224523edc0829172'),
    (0x3a723a,6,'8df471ed38424bb4041ca7c8bb78e0ed7769b738c4582339833c4db16eb2ea32'),
    (0x4753a0,208,'bb005a3b99080ba959a3dcc922dcf991a7276021c1c3bb8e5399cfb0eb8dc18a'),
    (0x1999b0,96,'c3a8423024ac61e0757ceea544c03b537a7224073123310794b3df3d62185999'),
    (0x475440,16,'213490d4cf57b2ab42948f1f064ff635f96410da333521d2dcf5b2ee3af92f30'),
    (0xc25f0,68,'3a6d01588c3c99e6fd3b85b6005ff888a5ee5414ca1286dc5485db0888d94fca'),
    (0xc20e0,101,'b03c3fe5fbd8bfb88c360ca6d86996124490341e3cd6274526be9483cf717275'),
    (0x19c40,90,'d84f1c67e68ea1c5fe4d7631dcc9e928c1c1852f2058d03dfcb30013de5c1419'),
    (0x4acd0,9,'3d88229ba8f81117aa293b673da37176d12ed5fdadb9eeb56bfda6da97e01c38'),
)
SOURCE_PINS={
    (41,0x199900):'9098cf585aa43edc99c3249190779be02547772f7a99bcf3874e5d03b42c4b24',
    (41,0x199a10):'bedf94a18e93702fe115911647ea73d5c7e876559e2f7f83fa08b428b9bcd3d5',
    (41,0x199d50):'724243b9aac86d296e66cb048f130fcbd9606ed9e9f6b0c6ba12f0bdba06b6eb',
    (41,0x1998b0):'96fe71ad9e81a4c52ea55810c699cefec260d03ab7de5d23d7e34212b4ef3f73',
    (32,0x13f1e0):'6c9dbdd00c799bed2cc4693dc27b39120ac6b830c3ada70ff051347a6d1d2421',
    (50,0x1f90a0):'16b6dedd634cd44a65105e309e0e85afd9bba2e4169f9ca187a617c257ca998e',
    (19,0xbd450):'e0ea9c87e85fe7737d5e6ce4e794170ab625c484517cced4cd8d3b5a415ac52b',
    (20,0xcc190):'d3028b728382689578a73d3bbfc221f3956be10691a30332ee169b81fc84319d',
    (20,0xc8550):'e07ce84ad8e97beb9b8fd9583cf0a573f8111e44d05c085c329e309c6b78f9a5',
    (19,0xc25f0):'82d19b8877a60a39a56087658997ba8fbb59ab7f2220609314726ad335f1ff4d',
    (19,0xc20e0):'9024b18b14591eee74302ddcb4f16a9c323f249f176aafe3c7476c12435d823c',
    (1,0x19c40):'5f3c68713da087e805a01e65d4ff6d785e7734740433091630019da0d6c6426d',
}
INCLUDE='#include "xbe_coop_turrets.h"\n'
RECIPIENT='\tTrialCoopTurretRecipient(ctx,base);\n'
EVENT='\tTrialCoopTurretEvent(ctx,base);\n'
RESUME='\tTrialCoopTurretResume(ctx,base);\n'
AIM='\tTrialCoopTurretAim(ctx,base);\n'
CAMERA='\tTrialCoopTurretCameraSlot(ctx,base);\n'
SCOPES={0x199900:'Exit',0x199a10:'Update',0x199d50:'State'}
def scope(phase):return '\tTrialCoopTurretScope trial_coop_turret(ctx,base,TrialCoopTurretScope::Phase::'+phase+');\n'
HOOKS=(INCLUDE,RECIPIENT,EVENT,RESUME,AIM,CAMERA,*(scope(v) for v in SCOPES.values()))
CALL_PREFIX='\tif (TrialCoopTurretEvent(ctx,base)) { '
CALL_SUFFIX=' } else { ctx.eax=0; ctx.esp+=20; }\n'
DOWNED_ORIGINAL='\tX86_PUSH32(0xC262Eu); sub_000BFE00(ctx, base);\n'
DOWNED='\tif (TrialCoopTurretKeepDowned(ctx,base)) { ctx.esp+=8; } else { X86_PUSH32(0xC262Eu); sub_000BFE00(ctx, base); }\n'

def strip(source):
    source=source.replace(DOWNED,DOWNED_ORIGINAL)
    source=re.sub(re.escape(CALL_PREFIX)+r'(X86_PUSH32\([^\n]+?sub_001F90A0\(ctx, base\);)'+re.escape(CALL_SUFFIX),r'\t\1\n',source)
    for hook in HOOKS:source=source.replace(hook,'')
    return source

def unprepare(area):
    for unit in (19,20,32,41):
        path=Path(area)/f'generated/game/x86_recomp.{unit}.cpp'
        if path.exists():
            old=path.read_text();new=strip(old)
            if new!=old:path.write_text(new)

def function(source,va):
    return re.search(r'X86_FUNC\(__imp__sub_'+f'{va:08X}'+r'\) \{.*?^\}',source,re.S|re.M)[0]+'\n'

def prepare(area):
    from prepare_xbe_coop_ko_rewards import clean as clean_ko
    area=Path(area);local=Path(__file__).parent
    names=('xbe_coop_turrets.h','xbe_coop_turrets.cpp','xbe_coop_turret_mount.h')
    maintained={name:(local/name).read_text() for name in names}
    test_marker='// ORIGINAL_COOP_TURRET_WITNESSES'
    test=(local/'test_xbe_coop_turrets.cpp').read_text();assert test.count(test_marker)==1
    xbe=(area/'assets/Default.xbe').read_bytes()
    base=struct.unpack_from('<I',xbe,0x104)[0];count,table=struct.unpack_from('<II',xbe,0x11c)
    sections=[struct.unpack_from('<6I',xbe,table-base+i*56) for i in range(count)]
    for va,size,digest in PINS:
        section=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4])
        offset=section[3]+va-section[1]
        assert hashlib.sha256(xbe[offset:offset+size]).hexdigest()==digest,hex(va)
    sources={unit:strip((area/f'generated/game/x86_recomp.{unit}.cpp').read_text()) for unit in (1,6,19,20,32,41,50)}
    for (unit,va),digest in SOURCE_PINS.items():
        assert hashlib.sha256(function(clean_ko(sources[unit],unit),va).encode()).hexdigest()==digest,hex(va)
    changed={unit:sources[unit] for unit in (19,20,32,41)}
    assert changed[19].count(DOWNED_ORIGINAL)==1
    changed[19]=changed[19].replace(DOWNED_ORIGINAL,DOWNED,1)
    for va,phase in SCOPES.items():
        marker=f'X86_FUNC(__imp__sub_{va:08X}) {{\n'
        assert changed[41].count(marker)==1
        changed[41]=changed[41].replace(marker,marker+scope(phase),1)
    for va in (0x199933,0x199a7d,0x199c4f,0x199cc3,0x199d16,0x19a447,0x19a476):
        marker=f'\t// {va:08X} '
        assert changed[41].count(marker)==1,hex(va)
        changed[41]=changed[41].replace(marker,RECIPIENT+marker,1)
    for unit,va in ((32,0x13f206),(41,0x1998d6),(41,0x199951),(41,0x199c6f),(41,0x199ce1),(41,0x199d34)):
        marker=f'\t// {va:08X} call 0x001F90A0\n'
        assert changed[unit].count(marker)==1,hex(va)
        start=changed[unit].index(marker)+len(marker);end=changed[unit].index('\n',start)
        call=changed[unit][start:end]
        assert re.fullmatch(r'\tX86_PUSH32\(0x[0-9A-F]+u\); sub_001F90A0\(ctx, base\);',call)
        changed[unit]=changed[unit][:start]+CALL_PREFIX+call.lstrip()+CALL_SUFFIX+changed[unit][end+1:]
    for unit,va,hook in ((41,0x199c74,AIM),(20,0xc8a92,CAMERA)):
        marker=f'\t// {va:08X} ';assert changed[unit].count(marker)==1
        changed[unit]=changed[unit].replace(marker,hook+marker,1)
    changed[41]=changed[41].replace(AIM,RESUME+AIM,1)
    assert changed[41].count(RECIPIENT)==7 and changed[41].count(CALL_PREFIX)==5
    assert changed[32].count(CALL_PREFIX)==1 and changed[20].count(CAMERA)==1
    # The separately emitted 19A065 tail has no mount, update or exit event.
    assert 'TrialCoopTurret' not in function(changed[41],0x19a065)
    # Tests execute the original event builders and actual player callbacks.
    functions=((6,0x4a840),(6,0x4acd0),(32,0x13f1e0),(41,0x1998b0),(50,0x1f90a0),(1,0x19c40),
               (19,0xbd450),(20,0xcc190),(19,0xc25f0),(20,0xc8550),(19,0xc20e0))
    witness=[f'X86_FUNC(sub_{va:08X});' for _,va in functions]
    for unit,va in functions:
        # Only fixture extraction omits the separate knockout ownership scope.
        part=function(clean_ko(sources[unit],unit),va).replace(f'__imp__sub_{va:08X}',f'sub_{va:08X}',1)
        if unit in changed and va in (0x13f1e0,0x1998b0,0xc25f0):
            part=function(clean_ko(changed[unit],unit),va).replace(f'__imp__sub_{va:08X}',f'sub_{va:08X}',1)
        witness.append(part)
    for name,unit,start,end in (('Mount',41,0x19a423,0x19a47e),('Update',41,0x199c39,0x199d39),
                                ('Exit',41,0x19991d,0x199956),('Camera',20,0xc8a8b,0xc8acb)):
        for fixed in (False,True):
            source=changed[unit] if fixed else sources[unit]
            a=source.index(f'\t// {start:08X} ');z=source.index(f'\t// {end:08X} ',a)
            # Include a hook immediately before the final boundary only when it
            # belongs to the selected body; the following instruction is omitted.
            part=source[a:z]
            witness.append(f'X86_FUNC({"Coop" if fixed else "Original"}{name}) {{\n\tX86_LOAD_FLAGS();\n\tuint32_t ca=0,cb=0;\n'+part+';\n}\n')
    # All pins, inputs and extraction boundaries must pass before mutation.
    for unit,source in changed.items():
        source=source.replace('#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+INCLUDE,1)
        (area/f'generated/game/x86_recomp.{unit}.cpp').write_text(source)
    for name,source in maintained.items():(area/'src/common'/name).write_text(source)
    (area/'src/compat/test_coop_turrets.cpp').write_text(test.replace(test_marker,'\n'.join(witness)))

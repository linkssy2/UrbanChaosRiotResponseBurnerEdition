"""Qualify the retail hostage AI's player-only target readers for local co-op."""
from pathlib import Path
import hashlib
import re
import struct

INCLUDE='#include "xbe_coop_hostage.h"\n'
HOOKS=(
    (0x11d1e1,'\tTrialCoopHostageTarget(ctx,base);\n'),
    (0x11b85d,'\tTrialCoopHostageFacing(ctx,base);\n'),
    (0x1189df,'\tTrialCoopHostageTarget(ctx,base,true);\n'),
    (0x118030,'\tTrialCoopHostageIdleFacing(ctx,base);\n'),
)
PINS=(
    (0x11d1d0,51,'9aa5907697b22614a24abfc9977e9c342379f42c3042a44108c08a33ab8e085e'),
    (0x11b840,46,'f2d715cc92ec4dbd9aa6d338e3c99a92caa49e14631ca3cc039d1ee75b8c306a'),
    (0x1189cf,190,'d00d2b62ec9c303d2ea1efc92a27c5c37c16c315abe32f51f5fdd7d2c859eb05'),
    (0x11bb30,655,'94858998e88794f765dcfc033c27b667e2e2c38888f44b14f8f3679e23dceb0e'),
    (0x11add0,373,'04c3a186518a57f1d5df36588e71f06a14780f08618a0255b89471d7a2aa5724'),
    (0x466f9c,4,'2be8e483bfd5b0890bd4fa87d4e178b8425b69b0dd396c3b56135fdf650ef9fd'),
    (0x4663fc,4,'edeaaae82f5cef744aaaf04c1e3c33d8edf9256977589ac12759781dc17e0eee'),
    (0x4acd0,9,'3d88229ba8f81117aa293b673da37176d12ed5fdadb9eeb56bfda6da97e01c38'),
    (0x4a840,6,'e51598aae247076e5665f4a60dfcf3e4ed9a470afbcf020165528094ba6f6c58'),
    (0x118018,69,'ab019dc6bc9fdda0988f1d1388b4fd881669bed10475f4a762f2bfffa8a3262d'),
    (0x4662dc,4,'443417493abaf32241145fcdfeb8e1db958bdfa35f2e5b8d2dfa2b3b3e65228a'),
)
BODIES=(
    (6,0x4a840,'77e02a84134be3bac2bfcea70562957f6be493f348198ff2ef8a30e72d891f4b'),
    (6,0x4acd0,'5e658c29e77f331c045498dba19928e7629042aec91a0cc55482940bbc4e1a05'),
    (28,0x11d1d0,'dc25e1982a95ee19407b3711ed4f4f957cfa3fd525e6696236eb5cdea5a651de'),
    (28,0x11bb30,'bf9ec38393f903522f8591634c1984f3b4d01b1f06cd67e5612b80bd4b65bd97'),
    (28,0x11add0,'7762fcd0ea284b12cb2afc91ff8e0089503dfd105f3ce9d2ca2bb62ee2fbf929'),
)
SLICES=(
    ('Facing',0x11b84d,0x11b86e,'780aca32a5ad142bd811988bc2401c9e9cdaed588df0a35b3010d598a99e4bc9'),
    ('Dragging',0x1189cf,0x118a8d,'04b0a6ec12b03fd2cc5901a2b2dbedfbfdf11333030a61615941ed970248e609'),
    ('IdleFacing',0x118018,0x11805d,'5744071b99ec197110543c46c4a9fb7dec6cfaf41bd5ca550861b24365f9ffe5'),
)

def clean(source):
    source=source.replace(INCLUDE,'')
    for _,hook in HOOKS:source=source.replace(hook,'')
    return source

def unprepare(area):
    path=Path(area)/'generated/game/x86_recomp.28.cpp'
    if path.exists():
        before=path.read_text();after=clean(before)
        if before!=after:path.write_text(after)

def prepare(area):
    area=Path(area);local=Path(__file__).parent
    image=(area/'assets/Default.xbe').read_bytes();base=struct.unpack_from('<I',image,0x104)[0]
    count,table=struct.unpack_from('<II',image,0x11c)
    sections=[struct.unpack_from('<6I',image,table-base+i*56) for i in range(count)]
    def raw(va,size):
        section=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4]);offset=section[3]+va-section[1]
        return image[offset:offset+size]
    for va,size,digest in PINS:assert hashlib.sha256(raw(va,size)).hexdigest()==digest,hex(va)
    sources={unit:clean((area/f'generated/game/x86_recomp.{unit}.cpp').read_text()) for unit in (6,28)}
    # These older scoped getter hooks remain in production. The fixture needs
    # the pinned original getter, then exercises our caller-local overrides.
    sources[6]=sources[6].replace('\tTrialCoopHazardPlayer(ctx,base);\n','').replace('\tTrialHealthPackMedicPlayer(ctx,base,false);\n','')
    fixtures=[]
    for unit,va,digest in BODIES:
        source=sources[unit];first=source.index(f'X86_FUNC(__imp__sub_{va:08X})');last=source.index('\nX86_WEAK_FUNC',first)
        body=source[first:last];assert hashlib.sha256(body.encode()).hexdigest()==digest,hex(va)
        if va==0x11d1d0:
            for fixed in (False,True):
                result=body.replace('__imp__sub_0011D1D0','CoopUpdate' if fixed else 'OriginalUpdate',1)
                if fixed:result=result.replace('\t// 0011D1E1 ',HOOKS[0][1]+'\t// 0011D1E1 ',1)
                fixtures.append(result)
        else:fixtures.append(body.replace(f'__imp__sub_{va:08X}',f'sub_{va:08X}',1))
    for name,first,last,digest in SLICES:
        source=sources[28];start=source.index(f'\t// {first:08X} ');stop=source.index(f'\t// {last:08X} ',start)
        body=source[start:stop];assert hashlib.sha256(body.encode()).hexdigest()==digest,hex(first)
        for fixed in (False,True):
            result=body
            if fixed:
                for va,hook in HOOKS:
                    if first<=va<last:result=result.replace(f'\t// {va:08X} ',hook+f'\t// {va:08X} ',1)
            external=set(re.findall(r'goto (loc_[0-9A-F]+)',result))-set(re.findall(r'^(loc_[0-9A-F]+):',result,re.M))
            result+='return;\n'+''.join(f'{label}:\n' for label in sorted(external))+'return;\n'
            fixtures.append(f'X86_FUNC({"Coop" if fixed else "Original"}{name}) {{\nX86_LOAD_FLAGS();uint32_t ca=0,cb=0;\n'+result+'}\n')
    test=(local/'test_xbe_coop_hostage.cpp').read_text()
    test=test.replace('// ORIGINAL_HOSTAGE_BODIES','\n'.join(fixtures))
    declarations=sorted(set(re.findall(r'\bsub_[0-9A-F]{8}(?=\(ctx)','\n'.join(fixtures))))
    test=test.replace('// ORIGINAL_HOSTAGE_DECLARATIONS','\n'.join('X86_FUNC('+name+');' for name in declarations))
    constants=(0x44c174,0x44bf94,0x466730,0x44c530,0x44e548,0x457328,0x44bf80)
    test=test.replace('// ORIGINAL_HOSTAGE_CONSTANTS','\n'.join(f'Put(0x{va:x},0x{struct.unpack("<I",raw(va,4))[0]:x});' for va in constants))
    changed=sources[28]
    for va,hook in HOOKS:
        marker=f'\t// {va:08X} ';assert changed.count(marker)==1,hex(va)
        changed=changed.replace(marker,hook+marker,1)
    assert clean(changed)==sources[28]
    changed=changed.replace('#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+INCLUDE,1)
    (area/'generated/game/x86_recomp.28.cpp').write_text(changed)
    for name in ('xbe_coop_hostage.h','xbe_coop_hostage.cpp'):(area/'src/common'/name).write_text((local/name).read_text())
    (area/'src/compat/test_coop_hostage.cpp').write_text(test)

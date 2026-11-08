"""Prepare the qualified current-ABI bounds leaf and exact reference fixture.

The CLI stages a private candidate without modifying the inspected source.
The build driver may call integrate(area) after restoring its generated source;
that operation is also safe to repeat against this exact prepared form.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import shutil
import struct

ROOT=Path(__file__).resolve().parents[1]
PINS={
    0xb2a40:(340,'0518efbfec621fae8f680e15772cc8f023b3de72ea9aa1752c2e7aeb7d580f0c','3e406fe3bb3e81258ff72e71944d0db96dc051775977480c05a79913766b99c6'),
    0xb2ba0:(24,'0823f83ec900256a78d7b360ea26ddf9f21afd5bbaeaadc44f728d2b9748c569','ec403b0b94d080cee3a0a0c64ea24bc8af9108040a807cfc7cf3db1912a19b45'),
}
INCLUDE='#include "xbe_lighting_bounds.h"\n'
ENTRY='\tif(trial_lighting_bounds::Try(ctx,base))return;\n'
def body(source,address):
    found=re.search(rf'X86_FUNC\(__imp__sub_{address:08X}\) \{{.*?^\}}',source,re.M|re.S)
    if not found:raise ValueError(f'Missing original bounds function {address:08X}')
    return found[0]
def qualify(source):
    if INCLUDE in source or ENTRY in source:raise ValueError('Bounds candidate already installed')
    originals={a:body(source,a)for a in PINS}
    for a,text in originals.items():
        if hashlib.sha256(text.encode()).hexdigest()!=PINS[a][2]:raise ValueError(f'Bounds source drift {a:08X}')
    if 'X86_SAVE_FLAGS' in originals[0xb2a40] or 'X86_SAVE_FLAGS' in originals[0xb2ba0]:raise ValueError('Reaudit observable flags')
    return originals
def transform(source):
    originals=qualify(source)
    old=originals[0xb2a40]
    return INCLUDE+source.replace(old,old.replace('{\n','{\n'+ENTRY,1),1)
def original(source):
    if INCLUDE in source or ENTRY in source:
        if not source.startswith(INCLUDE) or source.count(INCLUDE)!=1 or source.count(ENTRY)!=1:
            raise ValueError('Unexpected bounds patch layout')
        source=source[len(INCLUDE):].replace(ENTRY,'',1)
    qualify(source)
    return source
def retail(area):
    xbe_path=area/'assets/Default.xbe';xbe=xbe_path.read_bytes()
    image_base=struct.unpack_from('<I',xbe,0x104)[0];count,table=struct.unpack_from('<II',xbe,0x11c)
    sections=[struct.unpack_from('<6I',xbe,table-image_base+i*56)for i in range(count)]
    for address,(size,expected,_)in PINS.items():
        section=next(s for s in sections if s[1]<=address and address+size<=s[1]+s[4])
        start=section[3]+address-section[1]
        if hashlib.sha256(xbe[start:start+size]).hexdigest()!=expected:raise ValueError(f'Retail bounds drift {address:08X}')
def fixture(source,patched):
    originals=qualify(source)
    fixture=[]
    for a,suffix in ((0xb2a40,'Leaf'),(0xb2ba0,'Wrapper')):
        for prefix,text in (('Original',originals[a]),('Fixed',body(patched,a))):
            text=text.replace(f'__imp__sub_{a:08X}',prefix+suffix,1)
            if a==0xb2ba0:text=text.replace('sub_000B2A40(ctx, base)',prefix+'Leaf(ctx, base)')
            fixture.append(text)
    return (ROOT/'scripts/test_xbe_lighting_bounds.cpp').read_text().replace('// NATIVE_BODIES','\n'.join(fixture))
def integrate(area):
    area=Path(area).resolve();path=area/'generated/game/x86_recomp.18.cpp'
    source=original(path.read_text());retail(area);patched=transform(source)
    common=area/'src/common';common.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(ROOT/'scripts/xbe_lighting_bounds.h',common/'xbe_lighting_bounds.h')
    (common/'test_lighting_bounds.cpp').write_text(fixture(source,patched))
    path.write_text(patched)
def prepare(area,output):
    area,output=Path(area).resolve(),Path(output).resolve()
    if output==area or output.is_relative_to(area):raise ValueError('Candidate output must be outside the active build area')
    source_path=area/'generated/game/x86_recomp.18.cpp'
    source=original(source_path.read_text());retail(area);patched=transform(source)
    # Strict transform rejects double application; integration accepts only
    # its exact known form and requalifies the recovered original function.
    assert original(patched)==source
    for invalid in (patched,source.replace('// 000B2A43','// altered 000B2A43',1)):
        try:transform(invalid)
        except ValueError:pass
        else:raise AssertionError('Missing transform rejection')
    output.mkdir(parents=True,exist_ok=True)
    (output/'x86_recomp.18.candidate.cpp').write_text(patched)
    shutil.copyfile(ROOT/'scripts/xbe_lighting_bounds.h',output/'xbe_lighting_bounds.h')
    (output/'test_lighting_bounds.cpp').write_text(fixture(source,patched))
    report={'integrated':False,'compiled':False,'source':str(source_path),'source_sha256':hashlib.sha256(source_path.read_bytes()).hexdigest(),
            'function_pins':{f'{a:08X}':p for a,p in PINS.items()},
            'files':{name:hashlib.sha256((output/name).read_bytes()).hexdigest()for name in ('xbe_lighting_bounds.h','test_lighting_bounds.cpp','x86_recomp.18.candidate.cpp')},
            'limitations':['No fixture execution, microbenchmark or live acceptance yet.','Unsupported inputs execute the unchanged current generated leaf.','No B9970 changes or old-runtime ABI/assembly integration.']}
    (output/'prepared.json').write_text(json.dumps(report,indent=2)+'\n')
    return report
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--area',type=Path,default=ROOT/'build/xbe-full-integration-20261014')
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();print(json.dumps(prepare(args.area,args.output),indent=2))

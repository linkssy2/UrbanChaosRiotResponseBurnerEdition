"""Use independently saved P2 controls at qualified retail gameplay readers."""
from pathlib import Path
import hashlib
import re
import struct

INCLUDE='#include "xbe_coop_controls.h"\n'
# instruction VA, setting, original load, replacement load
READS=(
 (19,0xbc4b8,'X86_MEM8(ea)','TrialCoopControlSlotValue(base,ctx.esi,4,X86_MEM8(ea))'),
 (19,0xbe7ea,'X86_MEM8(ea)','TrialCoopControlValue(base,ctx.esi,3,X86_MEM8(ea))'),
 (19,0xbef98,'X86_MEM8(ea)','TrialCoopControlValue(base,ctx.edi,4,X86_MEM8(ea))'),
 (19,0xbeff1,'X86_MEM8(ea)','TrialCoopControlValue(base,ctx.edi,1,X86_MEM8(ea))'),
 (19,0xbf9a1,'X86_MEM8(ea)','TrialCoopControlValue(base,ctx.edi,2,X86_MEM8(ea))'),
 (19,0xbfa45,'X86_MEM32(ea)','TrialCoopControlValue(base,ctx.edi,5,X86_MEM32(ea))'),
 (19,0xbfa71,'X86_MEM32(ea)','TrialCoopControlValue(base,ctx.edi,6,X86_MEM32(ea))'),
 (9,0x68307,'X86_MEM8(ea)','TrialCoopVibrationValue(base,ctx.ebp,ctx.ecx,X86_MEM8(ea))'),
)
PINS=(
 (9,0x63b70,124,"755f8a9a5043e96b3888cb6f796a862978cd6a33ef9c3a4630389b621f648428","6b347b27aba0c2a2c0517d489edd26a662992c0f19cb8314fde270d925412079"),
 (9,0x682f0,492,"a60c61182b3dabd6dfc8d40e2b1710137092bc5acf7bc129f76f7bd82397694c","c39e3800b64c227706cdc95191d7d90af54644572e2ea76dc5e70d9536677713"),
 (19,0xbc4b0,165,"1c3247fbdd7b939b55a7922bfa3d73f4aad5070a92a3e3463e32a1afbe1b1c2f","f5503c9c916852a178ce5e4d0425694c0598ecd00ebd0333ee78e450ddbbb11a"),
 (19,0xbe7d0,158,"93b94c6c09f5727eb5c6c4a1c31ed1dd020399d8aeeb512108b4b215941719e2","9434b299047ccd107b172acd7ac47b4bd03c7dc9341436dfd6aafdd042861ec2"),
 (19,0xbef80,3695,"f0d83761e926db7a90d629244d6b952e7480bcddd7617538f72b8c6e2f5e59e0","2170d51d38ec9d1b9fd3d1bcaf59333a8d61206f9602bd091701405978a6e745"),
)

def body(source,address):
    return re.search(rf'X86_FUNC\(__imp__sub_{address:08X}\) \{{.*?^\}}',source,re.M|re.S).group()

def clean(source):
    source=source.replace(INCLUDE,'')
    for _,_,original,fixed in READS:source=source.replace(fixed,original)
    return source

def unprepare(area):
    for unit in (9,19):
        path=Path(area)/f'generated/game/x86_recomp.{unit}.cpp'
        if path.exists():
            before=path.read_text();after=clean(before)
            if after!=before:path.write_text(after)

def prepare(area):
    area=Path(area);local=Path(__file__).parent
    image=(area/'assets/Default.xbe').read_bytes();base=struct.unpack_from('<I',image,0x104)[0]
    count,table=struct.unpack_from('<II',image,0x11c)
    sections=[struct.unpack_from('<6I',image,table-base+i*56)for i in range(count)]
    def raw(va,size):
        s=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4]);off=s[3]+va-s[1]
        return image[off:off+size]
    assert struct.unpack('<I',raw(0x452640+0x2c,4))[0]==0x682f0
    sources={unit:clean((area/f'generated/game/x86_recomp.{unit}.cpp').read_text())for unit in(9,19)}
    originals={}
    for unit,address,size,raw_hash,lifted_hash in PINS:
        assert hashlib.sha256(raw(address,size)).hexdigest()==raw_hash,hex(address)
        text=body(sources[unit],address)
        assert hashlib.sha256(text.encode()).hexdigest()==lifted_hash,hex(address)
        originals[address]=text
    for unit,address,original,fixed in READS:
        source=sources[unit]
        pattern=rf'(\t// {address:08X} [^\n]+\n)([^\n]+)'
        match=re.search(pattern,source);assert match and source.count(f'\t// {address:08X} ')==1,hex(address)
        instruction=match[2];assert instruction.count(original)==1,hex(address)
        source=source[:match.start(2)]+instruction.replace(original,fixed)+source[match.end(2):]
        sources[unit]=source
    for unit,source in sources.items():
        path=area/f'generated/game/x86_recomp.{unit}.cpp'
        path.write_text(source.replace('#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+INCLUDE,1))
    fixtures=[]
    for unit,address,*_ in PINS:
        if address==0xbef80:continue
        for name,source in [('Original',originals[address]),('Coop',body(sources[unit],address))]:
            fixtures.append(source.replace(f'__imp__sub_{address:08X}',f'{name}_{address:08X}'))
    # Native look excerpts retain instruction arithmetic and branches; surrounding
    # aim acceleration and animation are deliberately outside this bounded test.
    for label,start,end in [('Layout',0xbef98,0xbefbd),('Invert',0xbeff1,0xbf00f),
                            ('AutoCentre',0xbf9a1,0xbf9ac),('Horizontal',0xbfa45,0xbfa68),
                            ('Vertical',0xbfa71,0xbfaa8)]:
        for name,source in [('Original',originals[0xbef80]),('Coop',body(sources[19],0xbef80))]:
            part=source[source.index(f'\t// {start:08X} '):source.index(f'\t// {end:08X} ')]
            # External targets are labeled at this excerpt's end to expose the
            # original branch outcome without simulating the omitted camera code.
            external=set(re.findall(r'goto (loc_[0-9A-F]+)',part))-set(re.findall(r'^(loc_[0-9A-F]+):',part,re.M))
            if label=='Invert':
                assert part.rstrip().endswith('loc_000BF00F:')
                part+=';\n'
            elif external:part+='ctx.eax=1;return;\n'+''.join(f'{target}:\n'for target in sorted(external))+'ctx.eax=0;\n'
            fixtures.append('X86_FUNC('+name+'_'+label+') {\nX86_LOAD_FLAGS();uint32_t ca=0,cb=0;\n'+part+'}\n')
    fixture=(local/'test_xbe_coop_controls.cpp').read_text()
    declarations=sorted(set(re.findall(r'\bsub_[0-9A-F]{8}(?=\(ctx)','\n'.join(fixtures))))
    fixture=fixture.replace('// ORIGINAL_DECLARATIONS','\n'.join('X86_FUNC('+n+');'for n in declarations))
    fixture=fixture.replace('// ORIGINAL_BODIES','\n'.join(fixtures))
    constants=(0x44bf94,0x44bf80,0x44c174,0x44c538,0x44c5fc,0x44c47c)
    fixture=fixture.replace('// ORIGINAL_CONSTANTS','\n'.join(f'Put(0x{a:x},0x{struct.unpack("<I",raw(a,4))[0]:x});'for a in constants))
    (area/'src/compat/test_coop_controls.cpp').write_text(fixture)

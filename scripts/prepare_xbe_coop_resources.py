"""Load original character art before campaign loading; select arms at creation."""
from pathlib import Path
import hashlib
import re
import struct
from coop_assets import WEAPON_ROOTS

PINS={
    0x54fe0:(7,1579,'175eab758296750d132f539a54479bc60de104f002d79e4594fd39e0e07ea7af','cdc6b231c84464c0f29cf80c442d0c48586cc1a47dc167f9a7a1106b627ceffe'),
    0x54b70:(7,122,'346e51ec44380ac147c78e6c0cc10e9fb9788a9c749c29bd6ef77f84bcfd02f6','a03df7c385386256dd1c6972c6daa8cb71815496e170682c46501aee9fd94f43'),
    0x8dac0:(14,99,'ce3336cdd4c0cafcd40f47f70eec9def02a8cdb79933fae56ab0539f4bfe1f28','5560658165d2e334f31fa6bd28fa0b57fc9bb9d618b29f0e11eb3cc295b72a64'),
    0x2a7b50:(71,18,'0d4641e22a94bf3e3d952540cba846c6865d42df80cd139af0ea42c5333f9379','2f829fe5180452b288ab17b3e0b32471bc0c0713deb74286bc6fb5eff90c02b3'),
    0x172370:(37,15,'162fa1b76a6d2bda38869d7fa1f64555ed9f54582455cde0f8439c6038285dd8','3e6ffbe7ee85fd1f484b69d6e382df554e83112670f3c59fce60ec62a34c9807'),
    0x23da10:(59,78,'575be1c8cfdaa417f647040c57db05f353e82db117a948e674cb193f5f68fb70','fe164baee13cd7d10fd7f188bc88e9a524340ee76c5dc80faae56d346b5dd3db'),
    0x23de20:(59,91,'eb7ec35a882b86c645bf776ca65c8254470dd8f1cef32ba1ab00f0609994187a','f3e67ad09c7f8fced2ea92f6983b38ae35890904a4d4ac879ebc75a93bf1a071'),
}
HOOKS={7:('\t// 0005505B call 0x00047B40','\tTrialCoopPreload(ctx,base);\n'),
       14:('\t// 0008DB16 dec ebx','\tTrialCoopResourceLoaded(ctx,base);\n'),
       37:('X86_FUNC(__imp__sub_00172370) {\n','\tTrialCoopArmsScope trial_coop_arms(ctx,base);\n')}
INCLUDE='#include "xbe_coop_resources.h"\n'

def runtime_call_guest():
    source=(Path(__file__).resolve().parents[1]/'build/upstream-review-xboxxbe-20261014/runtime/cpu/dispatch.cpp').read_text()
    start=source.index('uint32_t CallGuest(')
    body=source[start:source.index('\n}\n',start)+2]
    assert hashlib.sha256(body.encode()).hexdigest()=='1e3a800968e6deb99ddc92052680c56a77c359aba6d7e0471b343fb0b4e0f16b'
    return body

def prepare(area):
    area=Path(area);local=Path(__file__).parent
    image=(area/'assets/Default.xbe').read_bytes()
    base=struct.unpack_from('<I',image,0x104)[0]
    count,table=struct.unpack_from('<II',image,0x11c)
    sections=[struct.unpack_from('<6I',image,table-base+i*56) for i in range(count)]
    def raw(va,size):
        section=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4])
        offset=section[3]+va-section[1]
        return image[offset:offset+size]
    weapon_table=raw(0x4fa090,0x400)
    assert hashlib.sha256(weapon_table).hexdigest()=='4b3c7ab6626f324b3e76c045fbc6a3e5b40678bddbb0735185f59ff93b033dda'
    runtime=(local/'xbe_coop_resources.cpp').read_text()
    for index,name in enumerate(WEAPON_ROOTS):
        primary,secondary=struct.unpack_from('<II',weapon_table,index*64)
        assert raw(primary,len(name)+1).lower()==name.encode()+b'\0',name
        assert secondary in (0,primary),name
        assert '"'+name.replace('\\','\\\\')+'"' in runtime,name
    paths={unit:area/f'generated/game/x86_recomp.{unit}.cpp' for unit,_,_,_ in PINS.values()}
    sources={unit:path.read_text().replace(INCLUDE,'') for unit,path in paths.items()}
    for unit,(_,hook) in HOOKS.items():sources[unit]=sources[unit].replace(hook,'')
    bodies={}
    for va,(unit,size,digest,body_digest) in PINS.items():
        assert hashlib.sha256(raw(va,size)).hexdigest()==digest,hex(va)
        body=re.search(rf'X86_FUNC\(__imp__sub_{va:08X}\) \{{.*?^\}}',sources[unit],re.S|re.M)[0]
        assert hashlib.sha256(body.encode()).hexdigest()==body_digest,hex(va)
        bodies[va]=body
    assert hashlib.sha256(raw(0x45bf50,55)).hexdigest()=='f5653a04bd24e32d05a1e0b6d880dbe09f6e587f826d2692c1451d2ce0dd37a9'
    for unit,(anchor,hook) in HOOKS.items():
        assert sources[unit].count(anchor)==1,anchor
        sources[unit]=sources[unit].replace(anchor,anchor+hook if anchor.startswith('X86_FUNC') else hook+anchor,1)
        sources[unit]=sources[unit].replace('#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+INCLUDE,1)
        paths[unit].write_text(sources[unit])
    for name in ('xbe_coop_resources.h','xbe_coop_resources.cpp'):
        (area/'src/common'/name).write_text((local/name).read_text())
    fixture=(local/'test_xbe_coop_resources.cpp').read_text()
    code=[]
    for va in (0x2a7b50,0x8dac0,0x172370,0x23da10,0x23de20):
        body=bodies[va]
        unit=PINS[va][0]
        if unit in HOOKS:
            anchor,hook=HOOKS[unit]
            body=body.replace(anchor,anchor+hook if anchor.startswith('X86_FUNC') else hook+anchor,1)
        code.append(body.replace('__imp__sub_','sub_'))
    assert fixture.count('// RETAIL_RESOURCE_FUNCTIONS')==1
    assert fixture.count('// PRODUCTION_CALL_GUEST')==1
    fixture=fixture.replace('// PRODUCTION_CALL_GUEST',runtime_call_guest())
    (area/'src/compat/test_coop_resources.cpp').write_text(fixture.replace('// RETAIL_RESOURCE_FUNCTIONS','\n'.join(code)))

if __name__=='__main__':prepare(Path(__file__).resolve().parents[1]/'build/xbe-full-integration-20261014')

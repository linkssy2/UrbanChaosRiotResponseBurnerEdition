"""Retain the local player's scene viewport across original heat-effect passes."""
from pathlib import Path
import hashlib
import struct

PINS=(
    (0x99dd0,1820,'1fda59bcde4c563ead5bac7d477cdaddc7c60d62e9f3cc6e57bce46afc331986'),
    (0x9a4f0,837,'cc649e8e6d6c00c61f8b8d74b330426e914358d94e7cca2398da2a1af8ede4e4'),
    (0x9a8a0,1166,'4e27b7ad7fa3d25a10ec3fe050d22e5a67ee53c4e4054cca12e71e99edb32d1d'),
    (0x9ad30,947,'03bdca5d4f6d61d7b7d2523031963564501ccea79850070a21874235472fa7e5'),
    (0x9b150,1497,'ee0806f5b0f63dda375718b55268a8ee79c2da97847f4d03c53b3c1868e39276'),
    (0x9f730,3190,'56cd4b81437f24d7fcedee0682aeb2209ed9a8c661eac33f2b567a8e23e1c31b'),
    (0x3e35d0,19,'5abcc03c6fddb5df6e28d028eb9b39f4ec1c2e8924c4d47f716b3473bb846f14'),
    (0x3e35f0,19,'fbae4d58f2656bd8b5d27bb1f45c35c6f2990c99da51394ce31c4da76b3ae45a'),
    (0x3e2b80,46,'9a0d51505b3c72da086baf506f77975747aa1c7b45dc856435d66f36dfe7e73b'),
)
FUNCTIONS={
    15:(('00099DD0','4fc7068c4684139a98487dd117322cf13381c1b8216524ef28883e8d301ac2b9'),
        ('0009A4F0','e95af35b778683ceb948de31f711ad89c48616e9115f0b89b9ae7fa047451a0c'),
        ('0009A8A0','e914e78df59f3bf179bfb4b865716935bd2d25d37806c6777137200042a19912'),
        ('0009AD30','9270eec791d93b2f57d2cc45a5c007d8359bf11e61ad0de88ced235de7bfafc3'),
        ('0009B150','2966bcb4a8976c4a2f0d8fb948eae1ea9e295e4b6ec6f77b4452d25d798124f6')),
    16:(('0009F730','a7382d9379150beb87e24968e4388deca5dd6addcceb0a0bbd3688f03569e764'),),
}

def prepare(area):
    area=Path(area);local=Path(__file__).parent
    xbe=(area/'assets/Default.xbe').read_bytes()
    base=struct.unpack_from('<I',xbe,0x104)[0]
    count,table=struct.unpack_from('<II',xbe,0x11c)
    sections=[struct.unpack_from('<6I',xbe,table-base+i*56) for i in range(count)]
    for va,size,digest in PINS:
        section=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4])
        offset=section[3]+va-section[1]
        assert hashlib.sha256(xbe[offset:offset+size]).hexdigest()==digest,hex(va)
    sources={}
    include='#include "xbe_coop_effects.h"\n'
    hook='\tTrialCoopEffectsViewportScope trial_coop_effects_viewport(ctx,base);\n'
    for unit,functions in FUNCTIONS.items():
        path=area/f'generated/game/x86_recomp.{unit}.cpp'
        source=path.read_text().replace(include,'').replace(hook,'')
        sources[unit]=source
        for name,digest in functions:
            anchor=f'X86_FUNC(__imp__sub_{name}) {{\n'
            first=source.index(anchor);last=source.index('\nX86_WEAK_FUNC(sub_'+name,first)
            assert hashlib.sha256(source[first:last].encode()).hexdigest()==digest,name
            assert source.count(anchor)==1,name
            source=source.replace(anchor,anchor+hook,1)
        assert source.count(hook)==len(functions)
        source=source.replace('#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+include,1)
        path.write_text(source)
    for name in ('xbe_coop_effects.h','xbe_coop_effects.cpp'):
        (area/'src/common'/name).write_text((local/name).read_text())
    transitions=[]
    for kind,ranges in [
        ('Enter',((0x9a10c,0x9a154),(0x9a5c2,0x9a606),(0x9aa86,0x9aaca),
                  (0x9ae02,0x9ae46),(0x9b3af,0x9b3f3))),
        ('Restore',((0x9a49b,0x9a4d9),(0x9a7e4,0x9a826),(0x9acbc,0x9ad06),
                    (0x9b08a,0x9b0d0),(0x9b6d4,0x9b71a),(0x9fddd,0x9fdf6))),
    ]:
        for index,(first,last) in enumerate(ranges):
            source=sources[16 if first>=0x9f000 else 15]
            start=source.index(f'\t// {first:08X} ')
            stop=source.index(f'\t// {last:08X} ',start)
            transitions.append(f'X86_FUNC(Particle{kind}{index}) {{\n\tX86_LOAD_FLAGS();\n\tuint32_t ca=0,cb=0;\n'+source[start:stop]+';\n}\n')
    test=(local/'test_xbe_coop_effects.cpp').read_text()
    marker='// ORIGINAL_COOP_EFFECTS_TRANSITIONS'
    assert test.count(marker)==1
    test=test.replace(marker,'\n'.join(transitions))
    (area/'src/compat/test_coop_effects.cpp').write_text(test)

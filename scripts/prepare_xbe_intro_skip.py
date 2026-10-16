"""One physical press per qualified startup movie; retain the retail state machine."""
from pathlib import Path
import hashlib
import struct


def between(text, begin, end):
    start=text.index(begin)
    return text[start:text.index(end,start)]


def prepare(area):
    """After prepare_xbe_input; stage the helper and original-backed CPU fixture."""
    area=Path(area);local=Path(__file__).parent
    xbe=(area/'assets/Default.xbe').read_bytes()
    base=struct.unpack_from('<I',xbe,0x104)[0]
    count,table=struct.unpack_from('<II',xbe,0x11c)
    sections=[struct.unpack_from('<6I',xbe,table-base+i*56) for i in range(count)]
    def image(va,size):
        s=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4])
        at=s[3]+va-s[1];return xbe[at:at+size]
    # Whole movie constructor/update, held-action query, exact startup chain,
    # vtable and its literal names. No gameplay cinematic owner is replaced.
    for va,size,digest in [
        (0x272bb0,1027,'6f33cf9392fdf78e692cafcaa47169817c1d5fcec37b20fa22eb2fc5d738021c'),
        (0x274740,45,'9458b26dc8bfe5d1a47c24c6c97d7fe570dd195718e6ffd09f7e6e819fdded97'),
        (0x272ad0,221,'26a6d7cc8d8ad24f7def6f0a4e5b95cf1f1e2d9bde33cb157c115fb46f73ee2e'),
        (0x272780,43,'ae537be36023348c3aa5b2ff0ab14816174f54e4fe6a6f358a0a2b6e4973787f'),
        (0x2723b0,43,'ca1a8b01ba20631368329bdc758787f730bfdab9c5b9b4899a63072c284eaa15'),
        (0x272380,43,'8017cd77c14e61e9844829a0ad99e98653641bb74fc2aa626662136489a2f3f4'),
        (0x2722f0,135,'bb5291e46b71ad803b4def5712bde1428449a38c5e417e07e3f4f7473c590a0e'),
        (0x489020,12,'e3f6f806a077569b0817d52502e58e1f99ccb2c9de943b76de01cd05c6ac8d6c'),
        (0x488e28,48,'ece5f2da030062fbe9d3b97ad6b042907e0558d406b577ee57236ebe9d216720'),
        (0x45289c,6,'126a3de351335585db57a815ccdb6fa9c37900d7f03d7af3f25a7af4479ec4d3'),
    ]:
        assert hashlib.sha256(image(va,size)).hexdigest()==digest,hex(va)
    path=area/'generated/game/x86_recomp.65.cpp'
    source=path.read_text()
    declarations='''extern void TrialIntroUpdate(X86Context&,uint8_t*);
extern bool TrialIntroQuery(X86Context&,uint8_t*);
extern void TrialIntroEnd(X86Context&,uint8_t*);'''
    if declarations not in source:
        source=source.replace('#include "x86_recomp_shared.h"',
                              '#include "x86_recomp_shared.h"\n'+declarations,1)
    for address,hook in [('00272BD6','TrialIntroUpdate(ctx,base);'),
                         ('00274740','if(TrialIntroQuery(ctx,base))return;'),
                         ('00272F87','TrialIntroEnd(ctx,base);')]:
        marker='\t// '+address+' '
        assert source.count(marker)==1
        insertion='\t'+hook+'\n'
        if insertion not in source:source=source.replace(marker,insertion+marker,1)
        assert source.count(insertion)==1 and insertion+marker in source
    path.write_text(source)
    for name in ('xbe_intro_skip.h','xbe_intro_skip.cpp'):
        (area/'src/common'/name).write_text((local/name).read_text())
    movie=between(source,'X86_FUNC(__imp__sub_00272BB0) {','\nX86_WEAK_FUNC(sub_00272BB0')
    query=between(source,'X86_FUNC(__imp__sub_00274740) {','\nX86_WEAK_FUNC(sub_00274740')
    query=query.replace('__imp__sub_00274740','sub_00274740',1)
    host=(local/'xbe_trial_menu_input.cpp').read_text()
    bodies=[]
    for name,typ in [('TrialIntroMovie','void'),('TrialIntroTake','bool'),('TrialIntroClose','void')]:
        body=between(host,typ+' '+name+'(','\n}')+'\n}'
        bodies.append(body)
    fixture=(local/'test_xbe_intro_skip.cpp').read_text()
    constants='\n'.join(f'W32(0x{a:x},0x{struct.unpack("<I",image(a,4))[0]:x});'
                        for a in [0x44c174,0x44ff80,0x4523d4,0x4523d0])
    for marker,body in [('// GENERATED_HOST','\n'.join(bodies)),('// GENERATED_QUERY',query),
                        ('// GENERATED_MOVIE',movie),('// GENERATED_CONSTANTS',constants)]:
        assert fixture.count(marker)==1,marker
        fixture=fixture.replace(marker,body)
    (area/'src/compat/test_intro_skip.cpp').write_text(fixture)

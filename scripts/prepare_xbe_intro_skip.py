"""Qualified movie skips with PC hold confirmation; retain the retail state machine."""
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
    # vtable and its literal names. Original movie playback and successors remain.
    for va,size,digest in [
        (0x272bb0,1027,'6f33cf9392fdf78e692cafcaa47169817c1d5fcec37b20fa22eb2fc5d738021c'),
        (0x274740,45,'9458b26dc8bfe5d1a47c24c6c97d7fe570dd195718e6ffd09f7e6e819fdded97'),
        (0x272ad0,221,'26a6d7cc8d8ad24f7def6f0a4e5b95cf1f1e2d9bde33cb157c115fb46f73ee2e'),
        (0x272fe0,70,'f856bb550543543abc50207cd5d315969da4f78635ea4edd659f9aaa46173964'),
        (0x272780,43,'ae537be36023348c3aa5b2ff0ab14816174f54e4fe6a6f358a0a2b6e4973787f'),
        (0x2723b0,43,'ca1a8b01ba20631368329bdc758787f730bfdab9c5b9b4899a63072c284eaa15'),
        (0x272380,43,'8017cd77c14e61e9844829a0ad99e98653641bb74fc2aa626662136489a2f3f4'),
        (0x2722f0,135,'bb5291e46b71ad803b4def5712bde1428449a38c5e417e07e3f4f7473c590a0e'),
        (0x489020,12,'e3f6f806a077569b0817d52502e58e1f99ccb2c9de943b76de01cd05c6ac8d6c'),
        (0x488e28,48,'ece5f2da030062fbe9d3b97ad6b042907e0558d406b577ee57236ebe9d216720'),
        (0x45289c,6,'126a3de351335585db57a815ccdb6fa9c37900d7f03d7af3f25a7af4479ec4d3'),
        (0x276690,0xfa,'64d83be46aee5a8a6af349e264161854c27a3e0898614f7ac885c8fe3d278c2b'),
        (0x271c00,0x279,'4b64b8368755663cc751126236e0b0c33186743cb8f665048344b79d71979223'),
        (0x271e80,0x19,'6677c8d9fdd69224c99f0184e2ecc5dfa2af047f19b78ba2f7d9e3db12f332fa'),
        (0x2717b0,0x56,'82c33570acae488b6338b639b4a8e9ad61036e3f7ae9844cce591e7b6773068b'),
        (0x271810,0x25,'4fce902715104ec14de187d23413a14614598ab4b70e24cabd52cb12a3ee2d38'),
        (0x64e30,4,'916ae10aa3fd76a4d5b25b2fb66e9df0fd04957c7ab10930268d1ed7808be689'),
        (0x5a920,0x86,'aa4947acd46eff54ae044de950fa4bd88e6383c4b176ae42bf6615b34f0bf4e5'),
        (0x5a9f0,10,'3daa0840c7b5b319ec7845454f8b40fb063f534abec39f8eacd0a0f9b1b95d4f'),
        (0x5aab0,6,'82a6f08404d9c41eb2d1437c1fe102f4c25209e68bea2833795ddaf6641ca0c7'),
        (0x2727e0,0x82,'2867e6afab786303e4bd5034887a0542eda79022e374842f4bf92d5ddb8de4ba'),
        (0x272870,0x13d,'d4e626b5839ae231a22f9c129becf403586dbeae36ae06f910a5a5d18088acc6'),
        (0x2767a0,0x69,'e53c728372ab7844f1257b6799c15e4be75e857fef75fa6f49d8bb1811bd6d73'),
        (0x27ca20,0x172,'dc95dd2dc86938403d283e239d2254d5b0ae643685ef333db98af48680641dd9'),
        (0x2845b0,0x23,'1af4a0582f060fea3d54462876695bf9ddefabe81de5e3cb962d72030865ffa0'),
    ]:
        assert hashlib.sha256(image(va,size)).hexdigest()==digest,hex(va)
    path=area/'generated/game/x86_recomp.65.cpp'
    source=path.read_text()
    declarations='''extern void TrialIntroUpdate(X86Context&,uint8_t*);
extern bool TrialIntroQuery(X86Context&,uint8_t*);
extern void TrialIntroEnd(X86Context&,uint8_t*);'''
    declarations+='\nextern void TrialTitleUpdate(X86Context&,uint8_t*);\nextern void TrialTitleAccept(X86Context&,uint8_t*);'
    if declarations not in source:
        source=source.replace('#include "x86_recomp_shared.h"',
                              '#include "x86_recomp_shared.h"\n'+declarations,1)
    startup_declarations='extern bool TrialStartupMovie(X86Context&,uint8_t*);\nextern bool TrialStartupMovieClose(const X86Context&,uint8_t*);'
    if startup_declarations not in source:source=source.replace(declarations,declarations+'\n'+startup_declarations,1)
    profile_declaration='extern bool TrialStartupProfile(X86Context&,uint8_t*);'
    if profile_declaration not in source:source=source.replace(startup_declarations,startup_declarations+'\n'+profile_declaration,1)
    for address,hook in [('00272BB0','if(TrialStartupMovie(ctx,base))return;'),
                         ('00272BD6','TrialIntroUpdate(ctx,base);'),
                         ('00274740','if(TrialIntroQuery(ctx,base))return;'),
                         ('00272F87','TrialIntroEnd(ctx,base);'),
                         ('00271CCD','if(TrialStartupProfile(ctx,base))return;'),
                         ('00276690','TrialTitleUpdate(ctx,base);'),
                         ('00276758','TrialTitleAccept(ctx,base);')]:
        marker='\t// '+address+' '
        assert source.count(marker)==1
        insertion='\t'+hook+'\n'
        if insertion not in source:source=source.replace(marker,insertion+marker,1)
        assert source.count(insertion)==1 and insertion+marker in source
    close='\tX86_PUSH32(0x273010u); sub_000877B0(ctx, base);'
    skipped='\tif(!TrialStartupMovieClose(ctx,base)){X86_PUSH32(0x273010u); sub_000877B0(ctx, base);}'
    source=source.replace(skipped,close);assert source.count(close)==1
    source=source.replace(close,skipped,1)
    path.write_text(source)
    for name in ('xbe_intro_skip.h','xbe_intro_skip.cpp'):
        (area/'src/common'/name).write_text((local/name).read_text())
    movie=between(source,'X86_FUNC(__imp__sub_00272BB0) {','\nX86_WEAK_FUNC(sub_00272BB0')
    # A maintained tree may already include the separately tested drawing scope.
    movie=movie.replace('\tTrialMenuLayoutScope trial_movie_overlay(4.f/3.f);\n','')
    query=between(source,'X86_FUNC(__imp__sub_00274740) {','\nX86_WEAK_FUNC(sub_00274740')
    query=query.replace('__imp__sub_00274740','sub_00274740',1)
    title=between(source,'X86_FUNC(__imp__sub_00276690) {','\nX86_WEAK_FUNC(sub_00276690')
    destroy=between(source,'X86_FUNC(__imp__sub_00272FE0) {','\nX86_WEAK_FUNC(sub_00272FE0')
    factories=[]
    for unit,address in [(8,'0005AAB0'),(8,'0005A9F0'),(8,'0005A920'),(9,'00064E30'),
                         (65,'002767A0'),(67,'002845B0'),(66,'0027CA20'),
                         (65,'002717B0'),(65,'00271C00'),(65,'00271E80'),
                         (65,'00271810'),(65,'002727E0'),(65,'00272870')]:
        text=source if unit==65 else (area/f'generated/game/x86_recomp.{unit}.cpp').read_text()
        body=between(text,'X86_FUNC(__imp__sub_'+address+') {','\nX86_WEAK_FUNC(sub_'+address)
        body=body.replace('__imp__sub_'+address,'sub_'+address,1)
        # Presentation adaptation is separate from the original controller ABI.
        body=body.replace('\tTrialMenuLayoutScope trial_menu_layout;\n','')
        body=body.replace('\tTrialGuestHook(ctx, base, 0);\n','')
        body=body.replace('\tTrialGuestHook(ctx, base, 1);\n','')
        if address=='0005A9F0':body=body.replace('{\n','{\n\t++controller_resets;FactoryEvent(2);\n',1)
        if address=='002717B0':body=body.replace('{\n','{\n\t++successors;FactoryEvent(3);\n',1)
        factories.append(body)
    host=(local/'xbe_trial_menu_input.cpp').read_text()
    bodies=[]
    for name,typ in [('TrialIntroMovie','void'),('TrialIntroTake','bool'),('TrialIntroClose','void'),('TrialIntroAdvance','void'),('TrialIntroProgress','float'),('TrialTitleFinish','void')]:
        body=between(host,typ+' '+name+'(','\n}')+'\n}'
        bodies.append(body)
    fixture=(local/'test_xbe_intro_skip.cpp').read_text()
    constants='\n'.join(f'W32(0x{a:x},0x{struct.unpack("<I",image(a,4))[0]:x});'
                        for a in [0x44c174,0x44ff80,0x4523d4,0x4523d0])
    factory_data='\n'.join('{const uint8_t data[]={'+','.join(str(b) for b in image(a,n))+
                           '};std::memcpy(g_base+0x'+format(a,'x')+',data,sizeof(data));}'
                           for a,n in [(0x450090,4),(0x485f00,32),(0x488d3c,0x3b),
                                       (0x27cb84,16)])
    for marker,body in [('// GENERATED_HOST','\n'.join(bodies)),('// GENERATED_QUERY',query),
                        ('// GENERATED_MOVIE',movie),('// GENERATED_TITLE',title),('// GENERATED_DESTROY',destroy),
                        ('// GENERATED_FACTORIES','\n'.join(factories)),('// GENERATED_FACTORY_DATA',factory_data),
                        ('// GENERATED_CONSTANTS',constants)]:
        assert fixture.count(marker)==1,marker
        fixture=fixture.replace(marker,body)
    (area/'src/compat/test_intro_skip.cpp').write_text(fixture)

"""Move only the retail Character Interaction skip query/prompt to keyboard F."""
from pathlib import Path
import hashlib
import struct


def between(text,begin,end):
    start=text.index(begin)
    return text[start:text.index(end,start)]


def prepare(area):
    """After input/PC prompt preparation; preserve their generated hooks."""
    area=Path(area);local=Path(__file__).parent
    image=(area/'assets/Default.xbe').read_bytes()
    base=struct.unpack_from('<I',image,0x104)[0]
    count,table=struct.unpack_from('<II',image,0x11c)
    sections=[struct.unpack_from('<6I',image,table-base+i*56) for i in range(count)]
    def raw(va,size):
        section=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4])
        at=section[3]+va-section[1];return image[at:at+size]
    # Complete speech skip/update and Fades setter, original edge-query arm,
    # close/destroy paths and exact owner identities. This excludes SimpleCutscene,
    # friendly-death cinematics, startup movie skipping and ordinary A/reload.
    pins=[
        (0xcf7e9,160,'7d5543121d9e24fede796a6736eef3a2dc9a63bd52aa311e0b34f157390234ba'),
        (0x17bb40,73,'b3e16da586c02f4caac367e3a088eca32574a84c08e58f2e9d0e14b13772148c'),
        (0x17bd80,1681,'665016a86df82f93cd599f56be769fd501d0546244f32ae88218821885af5a22'),
        (0x223090,224,'b5029b7f14b74d2ac5825ea8190d40ba8ed474da6208da621c4e433e96f15209'),
        (0x568b2,59,'8194b0eedf54d3de00512a32546a1ad60e6978b6f39cb81cb69565a70c191eb5'),
        (0x17bac0,38,'636ac9a96537d9f2ede88be579771fe3ef9eaf3f4aaab6df5d166207b9c16e93'),
        (0x17bc10,89,'fa3f5001428b38c76c619fcb4cc239be50ba355d128f544885a1291d552f0352'),
        (0x2232f0,248,'8b4f52cb5f8f0d325d1c55c5e657f9f2a4ae30c553991bbc13098b1ea2093ec3'),
        (0x471060,64,'af8c4b55d0d546d5f986f38afb722e2d71f98f02f319e6eea07d9d1b3cf13ec3'),
        (0x48154c,28,'c816fd095504235b85ea05323c42123d9626a7b60fa16711d0f05124bbaba58a'),
        (0x470ee4,13,'c37aa9edb3d7bdd0eca143e4947795924af37c642f5384e611e89f33b4fb978d'),
    ]
    for va,size,digest in pins:
        assert hashlib.sha256(raw(va,size)).hexdigest()==digest,hex(va)
    declarations='''extern bool TrialConversationQuery(X86Context&,uint8_t*);
extern void TrialConversationPrompt(X86Context&,uint8_t*);
extern void TrialConversationClose(uint32_t);
extern void TrialReloadQuery(X86Context&,uint8_t*);'''
    sources={}
    for chunk in (21,38,55):
        path=area/f'generated/game/x86_recomp.{chunk}.cpp'
        source=path.read_text()
        # Older prepared files have only the three conversation declarations.
        owned=set(declarations.splitlines())
        source=''.join(line for line in source.splitlines(keepends=True)
                       if line.rstrip('\r\n') not in owned)
        shared='#include "x86_recomp_shared.h"\n'
        assert source.count(shared)==1
        source=source.replace(shared,shared+declarations+'\n',1)
        assert all(source.splitlines().count(line)==1 for line in owned)
        if chunk==21:
            hooks=[('000CF7FF','TrialReloadQuery(ctx,base);')]
        elif chunk==38:
            hooks=[('0017BB73','TrialConversationQuery(ctx,base);'),
                   ('0017BAC0','TrialConversationClose(ctx.ecx);'),
                   ('0017BC10','TrialConversationClose(ctx.ecx);'),
                   ('0017BDA4','if(!ctx.eax)TrialConversationClose(ctx.esi);')]
        else:
            hooks=[('00223090','TrialConversationPrompt(ctx,base);'),
                   ('002232F0','TrialConversationClose(ctx.ecx);')]
        for address,hook in hooks:
            marker='\t// '+address+' '
            assert source.count(marker)==1
            insertion='\t'+hook+'\n'
            if insertion+marker not in source:source=source.replace(marker,insertion+marker,1)
            assert source.count(insertion+marker)==1
        path.write_text(source);sources[chunk]=source
    for name in ('xbe_conversation_skip.h','xbe_conversation_skip.cpp'):
        (area/'src/common'/name).write_text((local/name).read_text())
    leaf=between(sources[38],'X86_FUNC(__imp__sub_0017BB40) {','\nX86_WEAK_FUNC(sub_0017BB40')
    original=leaf.replace('\tTrialConversationQuery(ctx,base);\n','').replace('__imp__sub_0017BB40','OriginalSpeechSkip')
    candidate=leaf.replace('__imp__sub_0017BB40','CandidateSpeechSkip')
    publication=between(sources[38],'\t// 0017C3E4 ','\t// 0017C3FF ')
    publication='X86_FUNC(PublishSpeechPrompt) {\nX86_LOAD_FLAGS();\n'+publication+'\n}'
    constants='\n'.join(f'W32(0x{a:x},0x{struct.unpack("<I",raw(a,4))[0]:x});' for a in (0x44c174,0x44c530))
    fixture=(local/'test_xbe_conversation_skip.cpp').read_text()
    for marker,body in [('// GENERATED_LEAF',original+'\n'+candidate),
                        ('// GENERATED_PUBLICATION',publication),('// GENERATED_CONSTANTS',constants)]:
        assert fixture.count(marker)==1;fixture=fixture.replace(marker,body)
    (area/'src/compat/test_conversation_skip.cpp').write_text(fixture)

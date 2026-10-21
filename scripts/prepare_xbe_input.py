"""Port the existing source-qualified PC input seams to the X86Context ABI."""
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[1]
AREA=ROOT/'build/xbe-full-integration-20261014'

def prepare():
    out=AREA/'src/common'
    from prepare_xbe_graphics_menu import prepare as prepare_graphics_menu
    prepare_graphics_menu(AREA)
    for name in ('menu_pointer_guest.h','menu_pointer_targets.h','menu_dialog_guest.inc','hybrid_frontend_guest.inc','hybrid_frontend_guest.h','pc_mouse_rotation.h'):
        source=(ROOT/'src'/name).read_text()
        source=source.replace('direction==2?0x20:0x21','direction==2?0x20u:0x21u')
        if name=='menu_pointer_guest.h':
            old='return address>=0x10000u&&(size_t)address<g_xbox_total_ram&&\n  (size_t)bytes<=g_xbox_total_ram-(size_t)address;'
            assert source.count(old)==1
            source=source.replace(old,'return TrialGuestReadable(address,bytes);')
        (out/name).write_text(source)
    (out/'xbe_trial_guest_input.cpp').write_text((ROOT/'scripts/xbe_trial_guest_input.cpp').read_text())
    (out/'xbe_input_memory.h').write_text((ROOT/'scripts/xbe_input_memory.h').read_text())
    backups=AREA/'generated-input-original';backups.mkdir(exist_ok=True)
    def call(n):return f'TrialGuestHook(ctx, base, {n})'
    wrappers={'00272870':(0,1),'002659C0':(2,3),'00263840':(4,5),'002649F0':(4,5),'00265250':(4,5),'00270320':(10,11),'0025A310':(16,19),'002840C0':(20,23)}
    seams={'00259860':(6,''),'00265C48':(7,''),'0027071F':(12,'002703C3'),'00270906':(13,''),'00272A00':(15,''),'0025A49D':(17,''),'0025A754':(18,''),'002840F4':(21,'0028415D'),'002843CE':(22,''),'00208880':(24,''),'000BF9A1':(25,'000BFA40'),'000D30C3':(26,'')}
    found=set()
    # Bracket the original device pass; background connection polls cannot consume taps.
    wrappers['00262A00']=(38,39)
    seams['0022E5E2']=(34,'')
    seams['0022E602']=(35,'')
    seams['000C6501']=(36,'')
    seams['000C6554']=(37,'000C6617')
    seams['0025C721']=(32,'')
    seams['0025C8A3']=(33,'')
    for p in (AREA/'generated/game').glob('x86_recomp.*.cpp'):
        text=p.read_text();backup=backups/p.name
        if 'TrialGuestHook' in text:
            assert backup.exists();text=backup.read_text()
        original=text
        for name,(entry,leave) in wrappers.items():
            pat=r'X86_FUNC\(__imp__sub_'+name+r'\) \{.*?^\}'
            m=re.search(pat,text,re.S|re.M)
            if not m:continue
            body=m[0];body=body.replace('{','{\n\t'+call(entry)+';',1)
            if name=='00262A00':
                marker='\t// 00263102 mov eax, [edi+0xAC]'
                assert body.count(marker)==1
                body=body.replace(marker,'\t'+call(40)+';\n'+marker,1)
            assert 'ctx.esp +=' in body,name
            body=re.sub(r'(\tctx\.esp \+= \d+; return;)',r'\t'+call(leave)+r';\n\1',body)
            text=text[:m.start()]+body+text[m.end():];found.add(name)
        for addr,(op,branch) in seams.items():
            marker='\t// '+addr+' '
            if marker not in text:continue
            assert text.count(marker)==1,addr
            insertion=(f'if ({call(op)}) goto loc_{branch};' if branch else call(op)+';')
            if branch and ('loc_'+branch+':') not in text:
                target='\t// '+branch+' '
                assert text.count(target)==1,branch
                text=text.replace(target,'loc_'+branch+':\n'+target,1)
            text=text.replace(marker,'\t'+insertion+'\n'+marker,1);found.add(addr)
        # These bounds are captured after the original stack writes, before the next instruction.
        for addr,op in [('00264422',8),('0026F44D',14)]:
            marker='\t// '+addr+' '
            if marker in text:
                assert text.count(marker)==1
                text=text.replace(marker,'\t'+call(op)+';\n'+marker,1);found.add(addr)
        marker='\t// 002745F0 '
        if marker in text:
            text=text.replace(marker,'\tif ('+call(9)+') { ctx.eax = (ctx.eax & ~255u) | 1; ctx.esp += 4; return; }\n'+marker,1);found.add('002745F0')
        if text!=original:
            if backup.exists():assert backup.read_text()==original,p
            else:backup.write_text(original)
            text=text.replace('#include "x86_recomp_shared.h"','#include "x86_recomp_shared.h"\nextern int TrialGuestHook(X86Context&, uint8_t*, unsigned);',1)
            if p.read_text()!=text:p.write_text(text)
    required=set(wrappers)|set(seams)|{'00264422','0026F44D','002745F0'}
    assert found==required,required-found
    import hashlib,struct
    image=(AREA/'assets/Default.xbe').read_bytes()
    base=struct.unpack_from('<I',image,0x104)[0];count,table=struct.unpack_from('<II',image,0x11c)
    sections=[struct.unpack_from('<6I',image,table-base+i*56) for i in range(count)]
    section=next(s for s in sections if s[1]<=0xc64d0< s[1]+s[4]);offset=section[3]+0xc64d0-section[1]
    assert hashlib.sha256(image[offset:offset+405]).hexdigest()=='a7acb040a35630faaa8e5d7e78fe09de97747ef21bbff4f8439fa498ad256093'
    for address,size,digest in [(0x643e0,6,'2f50be2683201136880faa73c01094369ebb3428808d2188c528381e7a624d6a'),
                                (0x276810,319,'96004b7edf645697396aff39a35a7a3e585a32e68108da5c704572bb031cb246')]:
        section=next(s for s in sections if s[1]<=address and address+size<=s[1]+s[4])
        offset=section[3]+address-section[1]
        assert hashlib.sha256(image[offset:offset+size]).hexdigest()==digest,hex(address)
    source=(AREA/'generated/game/x86_recomp.20.cpp').read_text()
    body=re.search(r'X86_FUNC\(__imp__sub_000C64D0\) \{.*?^\}',source,re.S|re.M)[0]
    host=(ROOT/'scripts/xbe_trial_menu_input.cpp').read_text();start=host.index('int TrialSpinSample(')
    host=host[start:host.index('\n}',start)+2]
    guest=(ROOT/'scripts/xbe_trial_guest_input.cpp').read_text();start=guest.index('    case 36:')
    guest=guest[start:guest.index('    case 0:',start)]
    test=(ROOT/'scripts/test_xbe_pistol_spin.cpp').read_text()
    for marker,value in [('// PRODUCTION_HOST_SAMPLE',host),('// PRODUCTION_GUEST_SAMPLE',guest),('// ORIGINAL_SPIN_DETECTOR',body)]:
        assert test.count(marker)==1;test=test.replace(marker,value)
    (AREA/'src/compat/test_pistol_spin.cpp').write_text(test)
if __name__=='__main__':prepare()

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
if __name__=='__main__':prepare()

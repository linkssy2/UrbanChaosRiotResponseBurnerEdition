#include <cstring>
#include "kernel/kernel.h"
#include "xbe_input_memory.h"
#include "xbe_frontend.h"
#include "xbe_level_details.h"

namespace {
uint32_t Word(const uint8_t* base,uint32_t p){uint32_t v;std::memcpy(&v,base+p,4);return v;}

// 270320 renders its detail list (+F4) through274320. The other list (+100)
// contains award animations; those retain their complete original behaviour.
bool LevelDetail(const uint8_t* base,uint32_t list,uint32_t node,uint32_t widget,uint32_t caller){
    if(caller!=0x2704df||list<0x100f4)return false;
    const uint32_t owner=list-0xf4;
    if(!TrialGuestReadable(owner,0x118)||Word(base,owner)!=0x4885fc||
       !Word(base,owner+0xec)||Word(base,owner+0xec)>17||
       Word(base,owner+0x1c)>=Word(base,owner+0xec)||
       !TrialFrontendPresents(owner,UC_HF_LEVELS)||!TrialGuestReadable(node,12)||
       !TrialGuestReadable(widget,0x50)||Word(base,node+8)!=widget)return false;
    const uint32_t head=Word(base,list+4),next=Word(base,node),previous=Word(base,node+4);
    return node!=head&&TrialGuestReadable(head,12)&&TrialGuestReadable(next,12)&&
        TrialGuestReadable(previous,12)&&Word(base,previous)==node&&Word(base,next+4)==node;
}
}

uint32_t TrialLevelDetailTarget(const X86Context& ctx,uint8_t* base,uint32_t target){
    // At274395 the original dt argument is already pushed. Both qualified
    // classes first execute273730 and then render; outgoing entries only need
    // that same update.274320 still owns expiration, destruction and unlinking.
    if(!TrialGuestReadable(ctx.esp,24)||
       !LevelDetail(base,ctx.edi,ctx.esi,ctx.ecx,Word(base,ctx.esp+20)))return target;
    const uint32_t type=Word(base,ctx.ecx);
    if(base[ctx.ecx+0x1c]&&
       ((type==0x48905c&&target==0x273860)||(type==0x452f1c&&target==0x2739a0)))return 0x273730;
    return target;
}

void TrialLevelDetailBlur(X86Context& ctx,uint8_t* base){
    // 273860 saves its caller's EDI(list)/ESI(node), then17730 receives the
    // computed blur. Change only that argument: original position, opacity,
    // clipping, text, progress/medals and the later font reset stay intact.
    if(!TrialGuestReadable(ctx.esp,56)||Word(base,ctx.esp)!=0x2738f7||
       Word(base,ctx.esp+28)!=0x274398||
       !LevelDetail(base,Word(base,ctx.esp+8),Word(base,ctx.esp+12),ctx.esi,Word(base,ctx.esp+52))||
       !TrialGuestReadable(ctx.esi,0xb4)||Word(base,ctx.esi)!=0x48905c||
       !TrialGuestReadable(0x509704,4)||Word(base,0x509704)!=ctx.ecx)return;
    const uint32_t zero=0;std::memcpy(base+ctx.esp+4,&zero,4);
}

#include <atomic>
#include <cstring>
#include "kernel/kernel.h"
#include "xbe_input_memory.h"
#include "xbe_conversation_skip.h"

extern int xbox_KeyboardMode();
namespace {
bool previous_f=false;
std::atomic<bool> pressed_f{false},prompt_active{false};
uint32_t speech_owner=0,fade_owner=0;
uint32_t ConversationWord(uint8_t* base,uint32_t address){
    uint32_t value;std::memcpy(&value,base+address,4);return value;
}
bool ConversationOwner(uint8_t* base,uint32_t owner){
    return TrialGuestReadable(owner,0x160)&&ConversationWord(base,owner)==0x471060;
}
}

void TrialConversationPublish(bool down,bool enabled){
    pressed_f.store(enabled&&down&&!previous_f,std::memory_order_relaxed);
    // Track the physical hold even while menus/focus block it. A held key cannot
    // become a new skip when focus returns or the original delay expires.
    previous_f=down;
}

bool TrialConversationQuery(X86Context& ctx,uint8_t* base){
    if(!xbox_KeyboardMode()||!ConversationOwner(base,ctx.esi)||
       !TrialGuestReadable(ctx.esp,4))return false;
    const uint32_t caller=ConversationWord(base,ctx.esp);
    if((caller!=0x17c0d4&&caller!=0x17c26e&&caller!=0x17c315)||
       !base[ctx.esi+0x119]||!base[ctx.esi+0x148])return false;
    // Exact return boundary17BB73. Keep the original query's polling, volatile
    // state and stdcall cleanup; replace only its mode1 press-edge float result.
    X87_ST(0)=pressed_f.load(std::memory_order_relaxed)?1.:0.;
    return true;
}

void TrialConversationPrompt(X86Context& ctx,uint8_t* base){
    if(!TrialGuestReadable(ctx.esp,8)||!TrialGuestReadable(ctx.ecx,0x1c)||
       ConversationWord(base,ctx.ecx)!=0x48154c)return;
    const bool active=ConversationWord(base,ctx.esp)==0x17c3ff&&
        (ConversationWord(base,ctx.esp+4)&255)&&ConversationOwner(base,ctx.esi)&&
        base[ctx.esi+0x119]&&base[ctx.esi+0x148];
    // Other cinematics use the same 'Skip' text. Their setter must restore the
    // usual A-button artwork instead of inheriting the conversation override.
    if(active){speech_owner=ctx.esi;fade_owner=ctx.ecx;}
    else if(fade_owner!=ctx.ecx)return;
    prompt_active.store(active,std::memory_order_relaxed);
    if(!active)speech_owner=fade_owner=0;
}

void TrialConversationClose(uint32_t owner){
    if(owner==speech_owner||owner==fade_owner){
        speech_owner=fade_owner=0;prompt_active.store(false,std::memory_order_relaxed);
    }
}
bool TrialConversationPromptActive(){return prompt_active.load(std::memory_order_relaxed);}

namespace {
bool previous_reload=false;
std::atomic<bool> pressed_reload{false};
}
void TrialReloadPublish(bool down,bool enabled){
    pressed_reload.store(enabled&&down&&!previous_reload,std::memory_order_relaxed);
    previous_reload=down;
}
void TrialReloadQuery(X86Context& ctx,uint8_t* base){
    // Only the original ManualReload query; automatic empty-magazine reload is unchanged.
    if(xbox_KeyboardMode()&&TrialGuestReadable(ctx.esi,0x36c)&&ConversationWord(base,ctx.esi+0x368)==0)
        X87_ST(0)=pressed_reload.load(std::memory_order_relaxed)?1.:0.;
}

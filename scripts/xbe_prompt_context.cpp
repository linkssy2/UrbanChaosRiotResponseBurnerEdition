#include <array>
#include <cstring>
#include "kernel/kernel.h"
#include "scripts/xbe_input_memory.h"
#include "xdk/d3d8.h"
#include "xbe_prompt_context.h"
#include "xbe_coop.h"
#include "xbe_coop_guest.h"

namespace {
thread_local unsigned prompt_text_variant=0;
thread_local unsigned prompt_player=0;
// The retail renderer owns one backwards-filled quad queue. No guest record or
// material is changed. The fixed sidecar has one byte per possible record.
std::mutex prompt_queue_lock;
std::array<uint8_t,65536> prompt_records{};
uint32_t prompt_owner=0,prompt_buffer=0,prompt_capacity=0;
uint32_t PromptWord(const uint8_t* base,uint32_t p){
    uint32_t value;std::memcpy(&value,base+p,4);return value;
}
unsigned PromptTextVariant(uint8_t* base,uint32_t text,uint32_t flags,uint32_t caller,uint32_t label){
    // The owned-screen footer draws its label and icon separately. Reward
    // 26DF19 publishes Continue in context slot1 (A). At this exact second draw
    // EDI still owns that key and ESI is the pinned, literal A glyph. Other
    // literal strings, menu labels and Reload retain their ordinary artwork.
    if(flags&0x80)return caller==0x254969&&flags==0x81&&text==0x4847e8&&label==0x485c58?1:0;
    char key[40]{};
    for(unsigned i=0;i<sizeof(key);++i){
        if(!TrialGuestReadable(text,1))return 0;
        unsigned c=base[text++];if(c>='A'&&c<='Z')c+='a'-'A';key[i]=char(c);
        if(!c){
            // This authored D-pad hint means Orders, not menu navigation.
            if(!std::strcmp(key,"hud-help-40"))return 2;
            // Exact authored interaction IDs: Open/Close Door, Use Ladder,
            // Operate, Rescue, Recapture, Open. Reload is HUD-Help-50.
            if(i==13&&!std::memcmp(key,"hud-operate-",12)&&
               std::strchr("1234568",key[12]))return 1;
            // The same authored Skip key is used by Character Interaction,
            // SimpleCutscene and the other cinematic controller. All query
            // action22/A, which accepts the configured interaction key. The
            // conversation-only publication flag excluded the latter two.
            return !std::strcmp(key,"hud-help-120")?1:0;
        }
    }
    return 0;
}
bool PromptQueue(uint8_t* base,uint32_t owner){
    if(!TrialGuestReadable(owner,0x40)||PromptWord(base,owner)!=0x456bb8)return false;
    const uint32_t buffer=PromptWord(base,owner+0x10),capacity=PromptWord(base,owner+0x18);
    if(!capacity||capacity>prompt_records.size()||buffer<0x10000||
       uint64_t(buffer)+uint64_t(capacity)*28>0x100000000ull)return false;
    if(owner!=prompt_owner||buffer!=prompt_buffer||capacity!=prompt_capacity){
        prompt_records.fill(0);prompt_owner=owner;prompt_buffer=buffer;prompt_capacity=capacity;
    }
    return true;
}
unsigned PromptRecordTag(uint8_t* base,uint32_t owner){
    std::lock_guard guard(prompt_queue_lock);
    auto invalid=[](){prompt_records.fill(0);prompt_owner=prompt_buffer=prompt_capacity=0;return 0u;};
    if(!PromptQueue(base,owner))return invalid();
    const uint32_t record=PromptWord(base,owner+0x14),count=PromptWord(base,owner+0x1c);
    if(count>prompt_capacity||record<prompt_buffer)return invalid();
    const uint32_t offset=record-prompt_buffer,index=offset/28;
    if(offset%28||index>=prompt_capacity||index<prompt_capacity-count||
       !TrialGuestReadable(record,28))return invalid();
    return prompt_records[index];
}
unsigned PromptRecordVariant(uint8_t* base,uint32_t owner){return PromptRecordTag(base,owner)&3u;}
}
TrialPromptPlayerScope::TrialPromptPlayerScope():previous(prompt_player){prompt_player=0;}
TrialPromptPlayerScope::~TrialPromptPlayerScope(){prompt_player=previous;}
void TrialPromptPlayerScope::Clear(){prompt_player=0;}
void TrialPromptPlayerScope::Select(const X86Context& c,const uint8_t* b){
    Clear();
    // 2153B3 gets EBP from the current original layer's owner. At 2153EB,
    // EDI is the world and stack+10 is the original HUD iterator's slot.
    if(!b||!trial_coop::Active()||!TrialGuestReadable(0x51c4dc,4)||
       !TrialGuestReadable(c.esp,0x14)||c.edi!=PromptWord(b,0x51c4dc)||
       !TrialGuestReadable(c.edi,8))return;
    const uint32_t slot=PromptWord(b,c.esp+0x10),state=PromptWord(b,c.edi+4);
    if(slot>1||!TrialGuestReadable(state,12))return;
    const uint32_t first=PromptWord(b,state+4);
    if(first>0xfffffff7u||PromptWord(b,state+8)!=first+8||!TrialGuestReadable(first,8))return;
    const uint32_t owner=TrialCoopPlayer(slot),layer=PromptWord(b,first+slot*4);
    if(c.ebp!=owner||!TrialGuestReadable(owner,0x36c)||PromptWord(b,owner)!=0x46fbe8||
       PromptWord(b,owner+0x54)==0xffffffffu||(PromptWord(b,owner+0x5c)&1)||
       PromptWord(b,owner+0x368)!=slot||!TrialGuestReadable(layer,0x28)||
       PromptWord(b,layer+4)!=owner||PromptWord(b,layer+0x24)!=slot)return;
    prompt_player=slot+1;
}
TrialPromptTextScope::TrialPromptTextScope(uint8_t* base,uint32_t text,uint32_t flags,uint32_t caller,uint32_t label):previous(prompt_text_variant){
    prompt_text_variant=PromptTextVariant(base,text,flags,caller,label);
}
TrialPromptTextScope::~TrialPromptTextScope(){prompt_text_variant=previous;}
void TrialPromptQueueRecord(uint8_t* base,uint32_t owner,uint32_t record){
    std::lock_guard guard(prompt_queue_lock);
    if(!PromptQueue(base,owner)){prompt_records.fill(0);prompt_owner=0;return;}
    const uint32_t count=PromptWord(base,owner+0x1c);
    if(count>=prompt_capacity||uint64_t(prompt_buffer)+uint64_t(prompt_capacity-count-1)*28!=record||
       !TrialGuestReadable(record,28))return;
    // Called only after the original bounds/capacity admission checks. An
    // ordinary glyph replacing this queue slot always clears its prior tag.
    prompt_records[prompt_capacity-count-1]=uint8_t(prompt_text_variant|(prompt_player<<2));
}
void TrialPromptQueueReset(uint32_t owner){
    std::lock_guard guard(prompt_queue_lock);
    if(owner==prompt_owner){prompt_records.fill(0);prompt_owner=prompt_buffer=prompt_capacity=0;}
}
TrialPromptDrawScope::TrialPromptDrawScope(uint8_t* base,uint32_t callback):lock(d3d::Lock()),previous(d3d::State().uc_prompt_variant),previous_player(d3d::State().uc_prompt_player){
    // 9E278 passes the secondary callback at primary owner+8. 9DB90 reads
    // callback+0C, i.e. primary+14, after the scheduler selects this record.
    const uint32_t owner=callback>=8&&TrialGuestReadable(callback,4)&&
        PromptWord(base,callback)==0x456b44?callback-8:0;
    const unsigned tag=PromptRecordTag(base,owner);
    d3d::State().uc_prompt_variant=tag&3u;
    d3d::State().uc_prompt_player=(tag>>2)&3u;
}
TrialPromptDrawScope::~TrialPromptDrawScope(){d3d::State().uc_prompt_variant=previous;d3d::State().uc_prompt_player=previous_player;}

bool TrialPromptOrdersKeyboard(const X86Context& c,const uint8_t* b){
    extern int xbox_KeyboardMode();
    if(!trial_coop::Active())return xbox_KeyboardMode()!=0;
    // 1592BD installs this behavior; 159357 stores its authored Player number.
    // 15895A reads its parent ID and 1589D3 uses the same number for input.
    if(!b||!TrialGuestReadable(c.esi,0xdc)||PromptWord(b,c.esi)!=0x46caa8||
       (PromptWord(b,c.esi+0x5c)&1)||PromptWord(b,c.esi+0x98)!=0)return false;
    const uint32_t owner=TrialCoopPlayer(0);
    if(!TrialGuestReadable(owner,0x36c)||PromptWord(b,owner)!=0x46fbe8||
       PromptWord(b,owner+0x54)==0xffffffffu||(PromptWord(b,owner+0x5c)&1)||
       PromptWord(b,owner+0x368)!=0)return false;
    return PromptWord(b,c.esi+0x58)==PromptWord(b,owner+0x54);
}

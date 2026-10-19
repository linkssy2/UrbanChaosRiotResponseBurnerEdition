#include <array>
#include <cstring>
#include "kernel/kernel.h"
#include "scripts/xbe_input_memory.h"
#include "xdk/d3d8.h"
#include "xbe_prompt_context.h"

namespace {
thread_local unsigned prompt_text_variant=0;
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
unsigned PromptRecordVariant(uint8_t* base,uint32_t owner){
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
    prompt_records[prompt_capacity-count-1]=uint8_t(prompt_text_variant);
}
void TrialPromptQueueReset(uint32_t owner){
    std::lock_guard guard(prompt_queue_lock);
    if(owner==prompt_owner){prompt_records.fill(0);prompt_owner=prompt_buffer=prompt_capacity=0;}
}
TrialPromptDrawScope::TrialPromptDrawScope(uint8_t* base,uint32_t callback):lock(d3d::Lock()),previous(d3d::State().uc_prompt_variant){
    // 9E278 passes the secondary callback at primary owner+8. 9DB90 reads
    // callback+0C, i.e. primary+14, after the scheduler selects this record.
    const uint32_t owner=callback>=8&&TrialGuestReadable(callback,4)&&
        PromptWord(base,callback)==0x456b44?callback-8:0;
    d3d::State().uc_prompt_variant=PromptRecordVariant(base,owner);
}
TrialPromptDrawScope::~TrialPromptDrawScope(){d3d::State().uc_prompt_variant=previous;}

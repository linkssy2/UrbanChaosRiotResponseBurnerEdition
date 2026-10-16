#include <atomic>
#include <exception>
#include "app/app.h"
#include "kernel/kernel.h"
#include "scripts/xbe_input_memory.h"
#include "xbe_language.h"

uint32_t TrialLanguage=0;
namespace trial_language {
static std::atomic<uint32_t> active{0};
uint32_t Active(){return active.load();}
uint32_t XboxLanguage(){
    // Original16C50 maps Xbox1/4/6/3 to title0/1/2/3 respectively.
    static constexpr uint32_t ids[Count]={1,4,6,3};
    return ids[Active()];
}
uint32_t Snapshot(){
    std::lock_guard lock(app::UiLock());
    return TrialLanguage<Count?TrialLanguage:0;
}
void Initialize(){
    std::lock_guard lock(app::UiLock());
    if(TrialLanguage>=Count)TrialLanguage=0;
    active=TrialLanguage;
}
bool PendingRestart(){return Snapshot()!=Active();}
bool Apply(uint32_t language,std::string& message){
    std::lock_guard lock(app::UiLock());
    if(language>=Count){message="That language is not available on this game disc.";return false;}
    const uint32_t previous=TrialLanguage;
    if(language!=previous){
        TrialLanguage=language;
        try {app::Save();} // existing private atomic replace; no live bank reload
        catch(const std::exception& e){
            TrialLanguage=previous;
            message=std::string("Could not save the language: ")+e.what();return false;
        }
        catch(...){TrialLanguage=previous;message="Could not save the language.";return false;}
    }
    message=language==Active()?"Language saved.":"Language saved. Restart the game to use it.";
    return true;
}
}

void TrialLanguageStartup(const X86Context& ctx,uint8_t* base){
    if(!base||!TrialGuestReadable(ctx.esp,4)||!TrialGuestReadable(0x5096e0,4)||
       !TrialGuestReadable(ctx.ecx,0x2c))return;
    auto read=[&](uint32_t at){uint32_t v;std::memcpy(&v,base+at,4);return v;};
    if(read(ctx.esp)!=0x16eef||read(0x5096e0)!=ctx.ecx||read(ctx.ecx+0x28)!=0xffffffff)return;
    // Retail16C00 admits non-English only on PAL. This PC setting supplies the
    // same cached title index at its one constructor boundary, leaving video
    // standard, authored bank/voice selection and the original cached path intact.
    const uint32_t value=trial_language::Active();
    std::memcpy(base+ctx.ecx+0x28,&value,4);
}

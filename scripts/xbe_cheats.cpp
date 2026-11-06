#include "xbe_cheats.h"
#include "app/app.h"
#include "cpu/x86_ops.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <array>
#include <atomic>
#include <cstring>
#include <exception>
#include <mutex>
#include <xmmintrin.h>

// Generated from the pinned retail activation/toggle instructions. It preserves
// native sound refresh and never replaces weapon, damage or progression logic.
void TrialCheatsNativeToggle(X86Context&,uint8_t*,unsigned,bool discover);
bool TrialCorpseShield=false;

namespace cheats {
constexpr uint32_t profile=0x5152b0,table=0x501728;
constexpr unsigned offsets[trial_cheats::RetailCount]={3,2,4,6,7,11,10,12,13,14};
constexpr uint32_t flags[trial_cheats::RetailCount]={0,0x101,0,1,0x100,0x100,0x100,0x100,0x100,0x100};
std::atomic<bool> corpse_shield{false};
constexpr uint32_t corpse_bit=1u<<trial_cheats::CorpseShield;
uint32_t WithCustom(uint32_t enabled){return (enabled&~corpse_bit)|(corpse_shield.load()?corpse_bit:0);}
struct Key {
    uint64_t pause=0;uint32_t world=0,level=0;
    std::array<uint8_t,24> profile_identity{};
    bool operator==(const Key& k)const{
        return pause==k.pause&&world==k.world&&level==k.level&&profile_identity==k.profile_identity;
    }
};
struct Native {Key key{};uint32_t enabled=0,known=0,allowed=0;};
struct Request {unsigned index=trial_cheats::Count;trial_cheats::State expected{};};
std::mutex mutex;
trial_cheats::State published;
Request pending;
Key key;
uint64_t revision=0;
uint32_t Read(const uint8_t* b,uint32_t at){uint32_t v;std::memcpy(&v,b+at,4);return v;}
bool ReadNative(const uint8_t* b,uint64_t pause,Native& result){
    if(!pause||!b||!TrialGuestReadable(profile,0x19c)||
       !TrialGuestReadable(table,trial_cheats::RetailCount*20)||
       !TrialGuestReadable(0x51c4dc,4)||!TrialGuestReadable(0x5096e0,4))return false;
    const uint32_t world=Read(b,0x51c4dc),settings=Read(b,0x5096e0);
    if(!TrialGuestReadable(world,0x70)||!TrialGuestReadable(settings,8))return false;
    const uint32_t level=Read(b,world+4),audio=Read(b,world+0x6c);
    // Main Menu has no campaign level; a nonnull level still needs a valid identity.
    if((level&&!TrialGuestReadable(level,8))||!TrialGuestReadable(audio,4))return false;
    const uint32_t vtable=Read(b,audio);
    if(!TrialGuestReadable(vtable,0x6c)||!TrialGuestReadable(Read(b,vtable+0x50),1)||
       !TrialGuestReadable(Read(b,vtable+0x68),1))return false;
    result={};result.key={pause,world,level,{}};
    std::memcpy(result.key.profile_identity.data(),b+profile,24);
    for(unsigned i=0;i<trial_cheats::RetailCount;++i){
        // +16 is the original menu's mutable visible-row number. Only verify
        // the immutable index/permission fields used by the native actions.
        if(Read(b,table+i*20+8)!=offsets[i]||Read(b,table+i*20+12)!=flags[i])return false;
        const uint32_t bit=1u<<i;
        if(b[profile+0x179+offsets[i]])result.enabled|=bit;
        if(b[profile+0x188+offsets[i]])result.known|=bit;
        // Original code entry restricts some first-time unlocks in its special
        // session mode; the original already-unlocked toggle has no such gate.
        if((flags[i]&0x100)||!(b[settings+4]&0x10)||(result.known&bit)||(result.enabled&bit))result.allowed|=bit;
    }
    return true;
}
void Revoke(){key={};pending={};published={};}
struct HostRestore {unsigned csr=_mm_getcsr();~HostRestore(){_mm_setcsr(csr);}};
struct StackRestore {
    uint8_t* b;uint32_t address;std::array<uint8_t,1024> saved;
    StackRestore(uint8_t* base,uint32_t top):b(base),address(top-uint32_t(sizeof saved)){
        std::memcpy(saved.data(),b+address,saved.size());
    }
    ~StackRestore(){std::memcpy(b+address,saved.data(),saved.size());}
};
}

trial_cheats::State trial_cheats::Snapshot(){std::lock_guard<std::mutex> lock(cheats::mutex);return cheats::published;}
void trial_cheats::Initialize(){cheats::corpse_shield=TrialCorpseShield;}
bool trial_cheats::CorpseShieldEnabled(){return cheats::corpse_shield.load();}
bool trial_cheats::Toggle(unsigned index,const State& expected){
    if(index>=Count||!expected.valid||expected.pending||!expected.identity||!expected.pause_epoch)return false;
    // ImGui already holds UiLock; preserve that lock order before the guest queue lock.
    std::unique_lock<std::recursive_mutex> ui;
    if(index==CorpseShield)ui=std::unique_lock<std::recursive_mutex>(app::UiLock());
    std::lock_guard<std::mutex> lock(cheats::mutex);auto& state=cheats::published;
    if(!state.valid||state.pending||state.identity!=expected.identity||state.pause_epoch!=expected.pause_epoch||
       ((state.enabled^expected.enabled)&(1u<<index)))return false;
    if(index==CorpseShield){
        const bool before=TrialCorpseShield;TrialCorpseShield=!before;
        try{app::Save();}
        catch(const std::exception&){TrialCorpseShield=before;return false;}
        cheats::corpse_shield=TrialCorpseShield;
        state.enabled=cheats::WithCustom(state.enabled);return true;
    }
    cheats::pending={index,expected};state.pending=true;return true;
}
void TrialCheatsInvalidate(){std::lock_guard<std::mutex> lock(cheats::mutex);cheats::Revoke();}
void TrialCheatsUpdate(X86Context& c,uint8_t* b,uint64_t pause_identity){
    using namespace cheats;HostRestore host;Native native;
    if(!ReadNative(b,pause_identity,native)){TrialCheatsInvalidate();return;}
    Request action;
    {
        std::lock_guard<std::mutex> lock(mutex);
        native.enabled=WithCustom(native.enabled);
        if(!published.valid||!(key==native.key)){
            if(!++revision)++revision;
            key=native.key;pending={};published={revision,native.enabled,true,false,pause_identity};
        }else published.enabled=native.enabled;
        action=pending;pending={};
        if(action.index>=trial_cheats::RetailCount)return;
        const uint32_t bit=1u<<action.index;
        if(action.expected.identity!=published.identity||action.expected.pause_epoch!=pause_identity||
           ((action.expected.enabled^native.enabled)&bit)||!(native.allowed&bit)||
           c.esp<0x10400||!TrialGuestReadable(c.esp-1024,1024)){
            published.pending=false;return;
        }
    }
    // Borrow unused guest-stack space, keeping the caller's complete CPU state
    // and host SSE environment intact. The native activation owns a tiny result
    // object above its temporary stack; it does not replace the pause screen.
    {
        StackRestore stack(b,c.esp);X86Context temp=c;temp.esp=c.esp-64;
        const bool discover=!(native.enabled&(1u<<action.index))&&!(native.known&(1u<<action.index));
        TrialCheatsNativeToggle(temp,b,action.index,discover);
    }
    Native after;
    const bool valid=ReadNative(b,pause_identity,after)&&after.key==native.key;
    std::lock_guard<std::mutex> lock(mutex);
    if(published.identity!=action.expected.identity||published.pause_epoch!=pause_identity)return;
    if(!valid){Revoke();return;}
    published.enabled=WithCustom(after.enabled);published.pending=false;
}

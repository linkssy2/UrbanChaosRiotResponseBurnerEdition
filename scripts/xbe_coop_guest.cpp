#include "xbe_coop_guest.h"
#include "xbe_coop.h"
#include "xbe_coop_pause.h"
#include "xbe_coop_body.h"
#include "xbe_coop_death.h"
#include "xbe_coop_downed_camera.h"
#include "xbe_coop_shields.h"
#include "xbe_coop_progression.h"
#include "xbe_coop_interactions.h"
#include "xbe_coop_rewards.h"
#include "xbe_coop_spin.h"
#include "xbe_coop_ko_rewards.h"
#include "xbe_coop_turrets.h"
#include "cpu/x86_ops.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <array>
#include <atomic>
#include <cmath>
#ifndef UC_BURNER_RELEASE
#include <cstdio>
#endif
#include <cstring>
#include <xmmintrin.h>

namespace {
std::atomic<uint32_t> players[2]{};
uint32_t world=0,primary_id=0xffffffffu;
uint32_t death_sequence=0,death_sequence_id=0xffffffffu;
bool attempted=false,spawning=false;
#ifndef UC_BURNER_RELEASE
std::array<unsigned,4> startup_trace_counts{};
constexpr std::array<unsigned,4> startup_trace_limits{4,2,2,1};
#endif
uint32_t Read(const uint8_t* b,uint32_t a){uint32_t v;std::memcpy(&v,b+a,4);return v;}
float Real(const uint8_t* b,uint32_t a){float v;std::memcpy(&v,b+a,4);return v;}
void Write(uint8_t* b,uint32_t a,uint32_t v){std::memcpy(b+a,&v,4);}
bool Actor(const uint8_t* b,uint32_t p){return b&&TrialGuestReadable(p,0x890)&&Read(b,p)==0x46fbe8u;}
void Forget(){players[0]=0;players[1]=0;world=0;primary_id=0xffffffffu;death_sequence=0;death_sequence_id=0xffffffffu;attempted=false;spawning=false;trial_coop::SetActive(false);trial_coop::ResetPauseOwner();}
bool SecondConstruction(const uint8_t* b,uint32_t p){
    return spawning&&b&&TrialGuestReadable(p,0x890)&&p!=players[0].load();
}
bool CameraReady(const uint8_t* b,uint32_t p){
    return Actor(b,p)&&Read(b,p+0x308)!=0xffffffffu;
}
#ifndef UC_BURNER_RELEASE
void TraceAdmission(const uint8_t* b,const char* stage){
    // Bounded development evidence; the release binary excludes this function.
    static unsigned records=0;
    if(!trial_coop::Available()||!trial_coop::Requested()||records>=8)return;
    ++records;
    std::fprintf(stderr,"[coop-admission] %s active=%u",stage,unsigned(trial_coop::Active()));
    for(unsigned slot=0;slot<2;++slot){
        const uint32_t p=players[slot];
        if(Actor(b,p))std::fprintf(stderr," p%u=%08X id=%u slot=%u health=%.6g max=%.6g camera=%08X",
            slot+1,p,Read(b,p+0x54),Read(b,p+0x368),double(Real(b,p+0x9c)),
            double(Real(b,p+0xa0)),Read(b,p+0x308));
        else std::fprintf(stderr," p%u=unavailable",slot+1);
    }
    std::fputc('\n',stderr);std::fflush(stderr);
}
#endif
bool DeathSequence(const uint8_t* b,uint32_t sequence){
    return trial_coop::Active()&&b&&Read(b,0x51c4dc)==world&&
        TrialGuestReadable(sequence,0x140)&&Read(b,sequence)==0x46b928u&&
        Read(b,sequence+0x54)!=0xffffffffu;
}
uint32_t LivePlayer(const uint8_t* b,uint32_t slot){
    if(slot>1)return 0;
    const uint32_t actor=players[slot];
    return Actor(b,actor)&&Read(b,actor+0x368)==slot&&!(Read(b,actor+0x5c)&1)&&
        Read(b,actor+0x54)!=0xffffffffu?actor:0;
}
struct SharedConstructionState {
    uint32_t world=0,preset=0,counters[2]{};
    uint8_t latch=0,flags[3]{};
    uint32_t flag_addresses[3]{};
    bool Capture(const uint8_t* b,uint32_t owner,uint32_t state){
        if(!TrialGuestReadable(owner,0xf7c)||!TrialGuestReadable(state,0x2058))return false;
        const uint32_t provider=Read(b,state+0x2054);
        if(!TrialGuestReadable(provider,0x3c))return false;
        const uint32_t count=Read(b,provider+0x38),map=Read(b,provider+0x34),data=Read(b,provider+0x2c);
        if(count>4096||(count&&!TrialGuestReadable(map,count*8)))return false;
        constexpr uint32_t ids[]={7,0x10,0x11};
        for(unsigned n=0;n<3;++n)for(uint32_t i=0;i<count&&ids[n]<=count;++i){
            if(Read(b,map+i*8)!=ids[n])continue;
            const int32_t offset=static_cast<int32_t>(Read(b,map+i*8+4));
            if(offset>=0){
                const uint64_t address=uint64_t(data)+uint32_t(offset);
                if(address>UINT32_MAX||!TrialGuestReadable(uint32_t(address),1))return false;
                flag_addresses[n]=uint32_t(address);flags[n]=b[address];
            }
            break;
        }
        world=owner;preset=Read(b,owner+0x948);latch=b[0x51bdf4];
        counters[0]=Read(b,owner+0xf74);counters[1]=Read(b,owner+0xf78);return true;
    }
    void Restore(uint8_t* b)const{
        // CC992/55BE0 resets P1's preset; CC404 clears the shared spin latch.
        Write(b,world+0x948,preset);b[0x51bdf4]=latch;
        // CCBA5/CCBB7 restore shared counters/flags from P1's checkpoint.
        Write(b,world+0xf74,counters[0]);Write(b,world+0xf78,counters[1]);
        // Exact flag bytes follow 2107F0's map; CC3DB also touches flag7.
        for(unsigned i=0;i<3;++i)if(flag_addresses[i])b[flag_addresses[i]]=flags[i];
    }
};
struct ShieldGate {
    uint32_t flags=0,count=0,animation=0;
    uint8_t locked=0;
    bool Capture(const uint8_t* b,uint32_t actor,uint32_t state){
        if(!Actor(b,actor)||(Read(b,actor+0x5c)&1))return false;
        flags=Read(b,actor+0x84c);count=Read(b,actor+0x850);
        animation=Read(b,actor+0x858);locked=b[actor+0x854];
        if(count>4096||(count&&!TrialGuestReadable(flags,count*4)))return false;
        if(count){
            const uint32_t provider=Read(b,state+0x2054);
            if(!TrialGuestReadable(provider,0x30))return false;
            const uint32_t data=Read(b,provider+0x2c);
            for(uint32_t i=0;i<count;++i){
                const uint64_t address=uint64_t(data)+Read(b,flags+i*4);
                if(address>UINT32_MAX||!TrialGuestReadable(uint32_t(address),1))return false;
            }
        }
        // This is a borrowed level-resource string, as in D49BC, not an owned
        // allocation. Only the flag list is freed by the actor at C28CA.
        if(animation)for(uint32_t i=0;;++i){
            if(i==1024||uint64_t(animation)+i>UINT32_MAX||!TrialGuestReadable(animation+i,1))return false;
            if(!b[animation+i])break;
        }
        return true;
    }
};
}

uint32_t TrialCoopPlayer(unsigned slot){return slot<2?players[slot].load():0;}

void TrialCoopRestoreViewViewport(X86Context& c,uint8_t* b){
    // 9E761 follows the original distortion-target clear (9F590). Its two
    // SetRenderTarget calls legitimately reset the SDK viewport, but the caller
    // has already selected this player's rectangle at 9E738. Restore that
    // rectangle before scene drawing, without changing any SDK target behavior.
    if(!b||!trial_coop::Requested()||!trial_coop::Active()||
       !TrialGuestReadable(0x51c4dc,4)||Read(b,0x51c4dc)!=world||
       !LivePlayer(b,0)||!LivePlayer(b,1)||!TrialGuestReadable(world,8)||
       !TrialGuestReadable(c.esi,0x12c)||Read(b,c.esi)!=0x480a6cu||
       Read(b,c.esi+0x128)!=0xffffffefu||!c.edi||Read(b,c.esi+4)!=c.edi||
       c.esp<0x10000||c.esp>0xfffffe00u||!TrialGuestReadable(c.esp-0x100,0x1ac)||
       Read(b,c.esp+0x70)!=c.edi)return;
    const uint32_t state=Read(b,world+4);
    if(!TrialGuestReadable(state,12))return;
    const uint32_t first=Read(b,state+4);
    if(!first||first>0xfffffff7u||Read(b,state+8)!=first+8||!TrialGuestReadable(first,8))return;
    unsigned slot=2;
    for(unsigned i=0;i<2;++i){
        const uint32_t layer=Read(b,first+i*4);
        if(!TrialGuestReadable(layer,0x28)||Read(b,layer+4)!=LivePlayer(b,i)||Read(b,layer+0x24)!=i)return;
        if(Read(b,layer+0x20)==c.edi){if(slot!=2)return;slot=i;}
    }
    if(slot==2||!TrialGuestReadable(0x3f4238,4))return;
    const uint32_t device=Read(b,0x3f4238),wanted=c.esp+0x94;
    if(!device||!TrialGuestReadable(device,0xef8))return;
    const uint32_t current=device+0xee0;
    std::array<uint32_t,6> before{},after{};
    std::memcpy(before.data(),b+current,24);std::memcpy(after.data(),b+wanted,24);
    // Only the fullscreen reset is repaired. Foreign/subviewport mismatches,
    // malformed bounds and depth overrides retain their original behavior.
    if(before==after||before[0]||before[1]||!after[2]||!after[3]||
       uint64_t(after[0])+after[2]>before[2]||uint64_t(after[1])+after[3]>before[3]||
       (after[0]==0&&after[1]==0&&after[2]==before[2]&&after[3]==before[3])||
       before[4]||before[5]!=0x3f800000u||after[4]||after[5]!=0x3f800000u)return;
    struct Restore {
        X86Context& context;X86Context saved;unsigned csr;
        ~Restore(){context=saved;_mm_setcsr(csr);}
    } restore{c,c,_mm_getcsr()};
    kernel::CallGuest(c,0x3e3610,{wanted});
#ifndef UC_BURNER_RELEASE
    static unsigned records=0;
    if(trial_coop::Available()&&trial_coop::Requested()&&records<8){
        ++records;
        std::fprintf(stderr,"[coop-viewport] restored slot=%u viewport=%08X before=%u,%u,%u,%u requested=%u,%u,%u,%u actual=%u,%u,%u,%u\n",
            slot,restore.saved.edi,before[0],before[1],before[2],before[3],after[0],after[1],after[2],after[3],
            Read(b,current),Read(b,current+4),Read(b,current+8),Read(b,current+12));
        std::fflush(stderr);
    }
#endif
}

#ifndef UC_BURNER_RELEASE
void TrialCoopTraceStartup(const X86Context& c,const uint8_t* b,uint32_t owner,TrialCoopTraceStage stage){
    // Private evidence at the original callback boundary. Budgets are independent
    // of admission logging and survive mission resets; this never advances startup.
    const unsigned index=unsigned(stage);
    if(!trial_coop::Available()||!trial_coop::Requested()||index>=startup_trace_counts.size()||
       startup_trace_counts[index]>=startup_trace_limits[index]||owner!=players[1]||!Actor(b,owner)||
       !TrialGuestReadable(0x51c4dc,4)||Read(b,0x51c4dc)!=world||!TrialGuestReadable(world,0x1c))return;
    ++startup_trace_counts[index];
    const unsigned csr=_mm_getcsr();
    constexpr const char* names[]={"update","camera-enter","camera-exit","unready-view"};
    const uint32_t coroutine=Read(b,owner+0x888),state=Read(b,world+4);
    std::fprintf(stderr,"[coop-startup] %s owner=%08X id=%u slot=%u flags5c=%08X flags60=%08X health=%.6g hud=%08X camera=%08X coroutine=%08X",
        names[index],owner,Read(b,owner+0x54),Read(b,owner+0x368),Read(b,owner+0x5c),Read(b,owner+0x60),
        double(Real(b,owner+0x9c)),Read(b,owner+0x300),Read(b,owner+0x308),coroutine);
    if(coroutine&&TrialGuestReadable(coroutine,0x30))
        std::fprintf(stderr," coro-owner=%08X selector=%u running=%u completed=%u",Read(b,coroutine+0x28),
            unsigned(b[coroutine+0x2c]),unsigned(b[coroutine+0x2d]),unsigned(b[coroutine+0x2e]));
    else std::fprintf(stderr," coro-invalid");
    std::fprintf(stderr," world=%08X flags10=%08X dt=%.6g state=%08X",world,Read(b,world+0x10),double(Real(b,world+0x18)),state);
    if(state&&TrialGuestReadable(state,0x34)){
        const uint32_t first=Read(b,state+4),end=Read(b,state+8);
        std::fprintf(stderr," update-head=%08X views=%08X..%08X",Read(b,state+0x30),first,end);
        // Retail view records contain actor at +4 and controller tag at +24.
        // Inspect only this actor's validated layer, never follow its camera.
        if(end>=first&&(end-first)%4==0&&end-first<=4*4&&first&&TrialGuestReadable(first,end-first)){
            for(uint32_t entry=first;entry<end;entry+=4){
                const uint32_t layer=Read(b,entry);
                if(layer&&TrialGuestReadable(layer,0x28)&&Read(b,layer+4)==owner)
                    std::fprintf(stderr," layer=%08X tag=%u",layer,Read(b,layer+0x24));
            }
        }else std::fprintf(stderr," views-invalid");
    }else std::fprintf(stderr," state-invalid");
    std::fprintf(stderr," ecx=%08X esi=%08X edi=%08X\n",c.ecx,c.esi,c.edi);std::fflush(stderr);
    _mm_setcsr(csr);
}
#endif

void TrialCoopMainplayerCreated(const X86Context& c,uint8_t* b){
    if(!trial_coop::Requested()||!Actor(b,c.eax))return;
    if(spawning){players[1]=c.eax;return;}
    // One authored campaign player owns scripts/save data. A new factory result
    // marks a new mission/checkpoint lifecycle, including address reuse.
    Forget();TrialCoopDeathReset();TrialCoopDownedCameraReset();TrialCoopBodiesReset();TrialCoopShieldReset();TrialCoopProgressionReset();TrialCoopInteractionReset();TrialCoopRewardsReset();TrialCoopSpinReset();TrialCoopKORewardsReset();TrialCoopTurretsReset();
    players[0]=c.eax;primary_id=Read(b,c.eax+0x54);world=Read(b,0x51c4dc);
}
void TrialCoopPlayerSlot(const X86Context& c,uint8_t* b){
    // D4804's original zero is followed by input-slot and OrderManager setup.
    if(SecondConstruction(b,c.esi))Write(b,c.esi+0x368,1);
}
void TrialCoopPlayerLayer(const X86Context& c,uint8_t* b){
    // 172DEC initializes the original new layer's controller tag to zero.
    if(SecondConstruction(b,c.esi)&&TrialGuestReadable(c.eax,0x2c))Write(b,c.eax+0x24,1);
}
void TrialCoopPlayerAlias(const X86Context& c,uint8_t* b){
    // Named groups retain unique IDs. Re-adding P1 leaves PLAYER unchanged;
    // P2 remains a normal entity and can still be an AI attention target.
    if(SecondConstruction(b,c.esi)&&Actor(b,players[0])&&TrialGuestReadable(c.esp,8)&&
       Read(b,c.esp+4)==0x44b828u)Write(b,c.esp,primary_id);
}
void TrialCoopPlayerDestroyed(const X86Context& c,uint8_t*){
    if(c.ecx==players[0]||c.ecx==players[1])Forget();
}

void TrialCoopAudioListenerCamera(X86Context& c,const uint8_t* b,uint32_t view_count){
    // 20E019-20E026 discards the supplied camera with multiple views. The null
    // listener sits at (-10000000,-10000000,-10000000), silencing nearby sounds.
    // 208930 supplies slot zero's camera. Keep that first-person listener,
    // including P2 startup;
    // all original spatialization, source updates and listener publication stay.
    if(c.ecx||view_count!=2||!b||!trial_coop::Requested()||Read(b,0x51c4dc)!=world||
       !LivePlayer(b,0)||!LivePlayer(b,1)||!TrialGuestReadable(c.eax,0x104)||Read(b,c.eax+0x5c)&1)return;
    const uint32_t actor=LivePlayer(b,0),id=Read(b,actor+0x54);
    if(TrialGuestReadable(c.eax,0x230)&&CameraReady(b,actor)&&Read(b,c.eax+0x54)==Read(b,actor+0x308)&&
       Read(b,c.eax+0x58)==id&&Read(b,c.eax+0x22c)==id&&Read(b,c.eax+0x210)==2){c.ecx=c.eax;return;}
    // Shared campaign cinematics replace the active camera in P1's original
    // stack. Qualify that exact camera, not any object of a similar class.
    const uint32_t type=Read(b,c.eax);
    if(!trial_coop::Available()||!trial_coop::Active()||Read(b,c.eax+0x54)==0xffffffffu||
       Read(b,c.eax+0xd0)!=0x478d20u||
       (type!=0x46fd70u&&type!=0x477da8u&&type!=0x477c58u)||!TrialGuestReadable(world,8))return;
    const uint32_t state=Read(b,world+4);
    if(!TrialGuestReadable(state,12))return;
    const uint32_t first=Read(b,state+4);
    if(!first||first>0xfffffff7u||Read(b,state+8)!=first+8||!TrialGuestReadable(first,8))return;
    const uint32_t layer=Read(b,first);
    if(!TrialGuestReadable(layer,0x28)||Read(b,layer+4)!=actor||Read(b,layer+0x24))return;
    const uint32_t stack=Read(b,layer+8);
    if(!TrialGuestReadable(stack,12)||!Read(b,stack+8))return;
    const uint32_t head=Read(b,stack+4);
    if(!TrialGuestReadable(head,8))return;
    const uint32_t node=Read(b,head+4);
    if(TrialGuestReadable(node,12)&&Read(b,node+8)==c.eax)c.ecx=c.eax;
}

void TrialCoopDeathPlayer(X86Context& c,const uint8_t* b,uint32_t sequence){
    // Only PlayerDeathSequence's five actor lookups use this hook. Its B8
    // listener stores the dying actor's controller tag at +D4 (141A20).
    // Original 4ACD0 ignores that tag and always returns the campaign's P1.
    if(!DeathSequence(b,sequence)||
       Read(b,sequence+0xd4)!=1||!Read(b,sequence+0x114)||Read(b,sequence+0x114)>10)return;
    const uint32_t actor=LivePlayer(b,1);if(!actor)return;
    c.eax=Read(b,actor+0x54);
}
bool TrialCoopDeathSequenceAllowed(const X86Context& c,const uint8_t* b){
    // Called only on 141A20's original idle branch, before its state changes.
    if(!DeathSequence(b,c.ecx)||Read(b,c.ecx+0x114)||!TrialGuestReadable(c.esp+4,4))return true;
    const uint32_t data=Read(b,c.esp+4);
    if(!TrialGuestReadable(data,4)||!LivePlayer(b,Read(b,data)))return true;
    return !death_sequence||(death_sequence==c.ecx&&death_sequence_id==Read(b,c.ecx+0x54));
}
void TrialCoopDeathSequenceAdmitted(const X86Context& c,const uint8_t* b){
    // The generated caller invokes this only after the retail state=1 and
    // controller-tag writes. Repeated callbacks on running sequences don't latch.
    if(death_sequence||!DeathSequence(b,c.ecx)||Read(b,c.ecx+0x114)!=1||
       !LivePlayer(b,Read(b,c.ecx+0xd4)))return;
    death_sequence=c.ecx;death_sequence_id=Read(b,c.ecx+0x54);
}
void TrialCoopDeathRumble(const X86Context& c,uint8_t* b,uint32_t sequence){
    // After the original push 0 at 142859/142895, replace only the port argument.
    if(DeathSequence(b,sequence)&&Read(b,sequence+0xd4)==1&&
       Read(b,sequence+0x114)>0&&Read(b,sequence+0x114)<=10&&LivePlayer(b,1)&&
       TrialGuestReadable(c.esp,4)&&Read(b,c.esp)==0)Write(b,c.esp,1);
}

void TrialCoopPlayerStarted(X86Context& ctx,uint8_t* b,uint32_t owner){
    if(!trial_coop::Requested()||!b||Read(b,0x51c4dc)!=world)return;
    if(owner==players[1]&&CameraReady(b,owner)&&CameraReady(b,players[0])){
        trial_coop::SetActive(true);
#ifndef UC_BURNER_RELEASE
        TraceAdmission(b,"cameras-ready");
#endif
        TrialCoopBodiesEnsure(ctx,b);return;
    }
    if(owner!=players[0]||attempted||spawning||!CameraReady(b,owner))return;
    trial_coop::SetResourceFailure(!trial_coop::ResourcesReady());
    if(trial_coop::ResourceFailure()||ctx.esp<0x18000)return;
    if(!TrialGuestReadable(world,8))return;
    const uint32_t state=Read(b,world+4);
    const uint32_t services=Read(b,0x51c398);
    if(!TrialGuestReadable(state,0x2050)||
       !TrialGuestReadable(services,16)||!TrialGuestReadable(Read(b,services+4),8))return;
    const uint32_t first=Read(b,state+4),end=Read(b,state+8);
    // Do not duplicate an already split session or a camera-only frontend world.
    if(!TrialGuestReadable(first,4)||end!=first+4)return;
    const uint32_t layer=Read(b,first);
    if(!TrialGuestReadable(layer,0x2c)||Read(b,layer+4)!=owner)return;
    std::array<float,16> transform;
    std::memcpy(transform.data(),b+owner+0x10,64);
    for(float f:transform)if(!std::isfinite(f))return;
    const uint32_t scratch=(ctx.esp-0x400)&~15u;
    if(!TrialGuestReadable(scratch-0x6000,0x6400))return;
    SharedConstructionState shared;
    if(!shared.Capture(b,world,state))return;
    ShieldGate shield;
    if(!shield.Capture(b,owner,state))return;
    struct Restore {
        X86Context& ctx;X86Context saved;uint8_t* base;uint32_t state,source;
        std::array<uint8_t,12> creation;unsigned csr;SharedConstructionState shared;
        ~Restore(){shared.Restore(base);std::memcpy(base+state+0x2044,creation.data(),12);Write(base,0x51c3e0,source);ctx=saved;_mm_setcsr(csr);spawning=false;}
    } restore{ctx,ctx,b,state,Read(b,0x51c3e0),{},_mm_getcsr(),shared};
    std::memcpy(restore.creation.data(),b+state+0x2044,12);
    struct OwnedFlags {
        X86Context& c;uint32_t stack,pointer=0;
        ~OwnedFlags(){if(pointer){c.esp=stack;c.ecx=0;kernel::CallGuest(c,0x2d631d,{pointer});}}
    } owned{ctx,scratch};
    if(shield.count){
        // BBC20 allocates the authored list with this allocator; C28CA releases
        // it with 2D631D. Never share P1's owned pointer with the second actor.
        ctx.esp=scratch;ctx.ecx=0;
        owned.pointer=kernel::CallGuest(ctx,0x2d589f,{shield.count*4});
        if(!owned.pointer||!TrialGuestReadable(owned.pointer,shield.count*4))return;
        std::memcpy(b+owned.pointer,b+shield.flags,shield.count*4);
    }
    attempted=true;spawning=true;_mm_setcsr(0x1f80);
    std::memset(b+scratch,0,0x2b4);
    ctx.esp=scratch;ctx.ecx=scratch;
    kernel::CallGuest(ctx,0x159c0,{0,0xffffffffu});
    // Independent world transform, not a child that follows P1. The original
    // factory allocates a fresh ID, actor, children, collision owner and view.
    Write(b,scratch+0x2a4,5);
    std::memcpy(b+scratch+0x260,transform.data(),64);
    Write(b,scratch+0x2b0,0x45a87cu); // Original immutable "Mainplayer" string.
    ctx.esp=scratch;ctx.ecx=world;
    // 207D20 inserts after an existing parent; FFFFFFFF has no predecessor and
    // silently leaves the new actor outside the world update list. 207CD0 uses
    // original priority insertion for an independent actor with no parent.
    const uint32_t created_id=kernel::CallGuest(ctx,0x207cd0,{scratch+0x2b0,0xffffffffu,scratch});
    const uint32_t secondary=LivePlayer(b,1);
    if(secondary&&LivePlayer(b,0)==owner&&Read(b,secondary+0x54)==created_id){
        // The generic factory has no authored shield-enable properties. Restore
        // P1's current campaign gate before the first P2 update; retail D0C30
        // retains ownership of the flag transition and activation animation.
        // These fields are empty on a newly constructed generic Mainplayer.
        if(!Read(b,secondary+0x84c)&&!Read(b,secondary+0x850)){
            Write(b,secondary+0x84c,owned.pointer);Write(b,secondary+0x850,shield.count);
            b[secondary+0x854]=shield.locked;Write(b,secondary+0x858,shield.animation);
            owned.pointer=0;
        }
        const float health=Real(b,secondary+0x9c),primary_health=Real(b,owner+0x9c);
        if(std::isfinite(health)&&health<=0&&std::isfinite(primary_health)&&primary_health>.001f){
            // 218550 starts at zero; the authored campaign initializes only P1.
            // 172380 skips camera startup at zero health. Keep checkpoint-restored
            // health, otherwise use 2180A0 to initialize health AND its +A0 maximum.
            const float maximum=Real(b,owner+0xa0);
            if(std::isfinite(maximum)&&maximum>primary_health){
                ctx.esp=scratch;ctx.ecx=secondary;
                kernel::CallGuest(ctx,0x2180a0,{Read(b,owner+0xa0)});
            }
            ctx.esp=scratch;ctx.ecx=secondary;
            kernel::CallGuest(ctx,0x2180a0,{Read(b,owner+0x9c)});
        }
    }
#ifndef UC_BURNER_RELEASE
    TraceAdmission(b,"secondary-created");
#endif
    ctx.esp=scratch;ctx.ecx=scratch;
    kernel::CallGuest(ctx,0x1f8ba0,{});
    // The normal world update advances this actor's own startup coroutine.
    // Active is published only after its 1726A0 creates/activates the camera.
    // shortcut: initial placement coincides with P1; safe separated spawn and
    // checkpoint/respawn policy need live validation before public co-op use.
}

// Execute retail 211610 enter/exit state commits and actual PLAYER/AABB query.
// Only group scheduling/name storage are fixture boundaries; script bytes and
// volumes come from the current original campaign archives during preparation.
#undef NDEBUG
#include <array>
#include <algorithm>
#include <cassert>
#include <cstdio>
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x1000000> memory{};}
namespace mem {bool IsCommitted(uint32_t p){return p<kernel::memory.size();}}
#include "../common/xbe_coop_progression.cpp"
// LEGACY_CENSUS
// Execute the same predicate against its former generated census. This is a
// fixture-only copy; production hooks and state have no legacy-policy switch.
namespace legacy {
void TrialCoopProgressionReset();
void TrialCoopProgressionEnter(X86Context&,uint8_t*);
namespace {
using legacy_progression_assets::kGates;
using legacy_progression_assets::kProofs;
}
#define coop_progression_assets legacy_progression_assets
#include "../common/xbe_coop_progression.cpp"
#undef coop_progression_assets
}
constexpr uint32_t P[2]={0x610000,0x612000},W=0x620000,S=0x624000,Q=0x650000;
constexpr uint32_t V=0x700000,L=0x690000,E=0x681000,D=0x682000,A=0x683000;
constexpr uint32_t C=0x800000,BX=0x685000,T=0x686000,R=0x687000,N=0x692000,SP=0xfe0000;
constexpr uint32_t SUBJECTS=0x6a0000,FLAGMAP=0x6a1000,FLAGS=0x6a2000,ENTRIES=0x6a3000,BARRIER_DESC=0x6a4000;
constexpr uint32_t NPC[2]={0x614000,0x616000};
static unsigned started=0,scheduled=0,overlaps=0,last_group=0,entered=0;
static uint32_t enter_group=0;
enum class Control {None,GaragePopulation,ShieldPickup,HostageTripwire,HostageBarrier};
struct Sample {
    uint32_t count,source,ordinal,group,script_count;
    const uint8_t*const* codes;const uint32_t* sizes;
    const uint8_t* rows;const uint8_t* boxes;uint32_t box_count;bool selected;Control control;
    uint32_t kind,subject_count;const uint8_t* subjects;const uint8_t* barrier_matrix;
};
// AUTHORED_SAMPLES
uint8_t* B(){return kernel::memory.data();}
void Put(uint32_t p,uint32_t v){std::memcpy(B()+p,&v,4);}
void Real(uint32_t p,float v){std::memcpy(B()+p,&v,4);}
float Value(uint32_t p){float v;std::memcpy(&v,B()+p,4);return v;}
uint32_t TrialCoopPlayer(unsigned i){return i<2?P[i]:0;}
// ORIGINAL_002084E0
X86_FUNC(sub_002084E0){__imp__sub_002084E0(ctx,base);}
// ORIGINAL_002107C0
X86_FUNC(sub_002107C0){__imp__sub_002107C0(ctx,base);}
// ORIGINAL_00210770
X86_FUNC(sub_00210770){__imp__sub_00210770(ctx,base);}
// ORIGINAL_00210320
X86_FUNC(sub_00210320){__imp__sub_00210320(ctx,base);}
X86_FUNC(sub_00210780){assert(false);}
X86_FUNC(sub_002102F0){assert(false);}
X86_FUNC(sub_001F7EA0){assert(Read(base,ctx.esp+4)==0x44b828);ctx.eax=1;ctx.esp+=8;}
X86_FUNC(sub_001F7DA0){ctx.eax=1;ctx.esp+=4;}
X86_FUNC(sub_001F7E00){assert(Read(base,ctx.esp+4)==0);ctx.eax=N;ctx.esp+=8;}
X86_FUNC(sub_001F8FC0){assert(Read(base,ctx.esp+4)==100);ctx.eax=P[0];ctx.esp+=4;}
// ORIGINAL_00207D70
X86_FUNC(sub_00207D70){__imp__sub_00207D70(ctx,base);}
// ORIGINAL_002D3830
X86_FUNC(sub_002D3830){__imp__sub_002D3830(ctx,base);}
X86_FUNC(sub_00214490){assert(false);}
// ORIGINAL_002187A0
// ORIGINAL_00211450
X86_FUNC(sub_00211450){__imp__sub_00211450(ctx,base);}
X86_FUNC(sub_002175A0){
    assert(ctx.ecx==T&&Read(base,ctx.esp+8)==1);
    ++started;last_group=Read(base,ctx.esp+4);
    // Count this sample's enter event independently of authored startup/exit
    // groups. These are the original initial-entry and re-entry return sites.
    const uint32_t caller=Read(base,ctx.esp);
    if(last_group==enter_group&&(caller==0x2117d4||caller==0x2116b8))++entered;
    ctx.eax=1;ctx.esp+=12;
}
X86_FUNC(sub_00218C40){assert(ctx.ecx==T);++scheduled;ctx.esp+=4;}
// ORIGINAL_00211610
// PATCHED_00211610
// LEGACY_00211610
void x86_icall(X86Context& c,uint8_t* b,uint32_t fn){
    assert(fn==0x2187a0);++overlaps;__imp__sub_002187A0(c,b);
}
namespace kernel {
uint32_t CallGuest(X86Context& c,uint32_t fn,const uint32_t* args,size_t count){
    assert(fn==0x2187a0&&count==1);const auto sp=c.esp;
    c.esp-=4;Put(c.esp,args[0]);c.esp-=4;Put(c.esp,0);
    ++overlaps;__imp__sub_002187A0(c,B());assert(c.esp==sp);const auto result=c.eax;
    // Additional volatile state makes preservation observable, rather than
    // relying on this particular leaf query to happen not to disturb it.
    c.eax=c.ebx=c.ecx=c.edx=c.esi=c.edi=c.ebp=0xdeadbeef;
    c.cf=c.zf=c.sf=c.of=c.pf=c.df=1;c.xflags^=0xff;c.top=6;c.st[2]=19.5;
    c.mm[3]=0xabcdef;c.xmm[5].u[2]=0xabcdef;c.mxcsr=0x5f80;_mm_setcsr(0x5f80);
    return result;
}
}
X86Context Context(){
    X86Context c{};c.esp=SP;c.eax=0xabc12301;c.ebx=21;c.ecx=T;c.edx=22;c.edi=23;c.esi=24;c.ebp=25;
    c.cf=1;c.zf=0;c.sf=1;c.of=1;c.pf=0;c.xflags=0x202;c.st[0]=1.25;c.top=3;
    c.mxcsr=0x3f80;c.xmm[5].u[2]=91;return c;
}
void Position(unsigned slot,bool inside){
    for(unsigned axis=0;axis<3;++axis)
        Real(P[slot]+0x40+axis*4,(Value(BX+axis*4)+Value(BX+12+axis*4))*0.5f-(inside?0.f:1000000.f));
}
void Setup(const Sample& sample){
    kernel::memory.fill(0);trial_coop::Reset();TrialCoopProgressionReset();legacy::TrialCoopProgressionReset();
    started=scheduled=overlaps=entered=0;enter_group=sample.group;
    trial_coop::Request();trial_coop::SetActive(true);trial_coop::ResourcesReady(true);
    Put(0x51c4dc,W);Put(W+4,S);Put(S+0x2054,Q);Put(Q+4,L);Put(Q+0x10,sample.count);Put(Q+0x1c,V);
    Put(0x46fbe8+0x48,0x2187a0);Put(N,100);
    Put(0x2189f8,0x2187c3);Put(0x2189fc,0x218814);Put(0x218a00,0x218868);Put(0x218a04,0x218960);
    const uint32_t vol=V+sample.ordinal*0xc0;
    Put(T,0x480888);Put(T+0x50,sample.source);Put(T+0x54,200);
    Put(L+sample.source*4,E);Put(E+4,200);Put(E+8,8);Put(E+0xc,D);Put(E+0x10,vol);
    Put(D+0x64,A);Put(A,sample.script_count);Put(A+4,A+0x20);
    uint32_t at=C;
    for(unsigned i=0;i<sample.script_count;++i){
        Put(A+0x20+i*4,at);std::memcpy(B()+at,sample.codes[i],sample.sizes[i]);at+=sample.sizes[i]+16;
    }
    assert(at<0x900000);
    Put(vol+0x50,sample.kind);Put(vol+0x54,3);B()[vol+0x6c]=sample.kind==17?57:16;Put(vol+0x74,BX);Put(vol+0x78,BX+sample.box_count*48);
    for(unsigned i=0;i<sample.box_count;++i)std::memcpy(B()+BX+i*48,sample.boxes+i*24,24);
    uint32_t rows;std::memcpy(&rows,sample.rows,4);assert(rows<=64);
    Put(T+0xd8,rows);Put(T+0xdc,R);std::memcpy(B()+R,sample.rows+4,rows*8);
    for(unsigned i=0;i<2;++i){
        Put(P[i],0x46fbe8);Put(P[i]+0x54,100+i);Put(P[i]+0x368,i);Real(P[i]+0x9c,100);Real(P[i]+0x88,100);
        Position(i,true);
    }
    if(sample.kind==17){
        assert(sample.subject_count==3);Put(vol+0x94,SUBJECTS);Put(vol+0x98,SUBJECTS+12);
        std::memcpy(B()+SUBJECTS,sample.subjects,12);
        for(unsigned i=0;i<3;++i){
            const auto actor=i?NPC[i-1]:P[0],id=i?299+i:100,entry=ENTRIES+i*0x20;
            Put(L+Read(B(),SUBJECTS+i*4)*4,entry);Put(entry+4,id);Put(entry+8,8);
            Put(S+0x3c+(id&1023)*8,id);Put(S+0x40+(id&1023)*8,actor);
            if(i){Put(actor,0x46fbe8);Put(actor+0x54,id);Real(actor+0x88,100);std::memcpy(B()+actor+0x40,B()+P[0]+0x40,12);}
        }
        // Authored script177 is local, separate from global18 ->local171.
        Put(Q+0x30,179);Put(Q+0x38,23);Put(Q+0x34,FLAGMAP);Put(Q+0x2c,FLAGS);
        Put(FLAGMAP,18);Put(FLAGMAP+4,171);B()[FLAGS+18]=B()[FLAGS+171]=1;
        Put(L+1296*4,ENTRIES+0x80);Put(ENTRIES+0x8c,BARRIER_DESC);
        std::memcpy(B()+BARRIER_DESC,sample.barrier_matrix,64);
    }
}
void Update(bool patched,bool old_census=false){
    auto c=Context();const auto initial=c;const auto before=scheduled;
    if(old_census)LegacyCoopEnterUpdate(c,B());
    else if(patched)CoopEnterUpdate(c,B());else __imp__sub_00211610(c,B());
    assert(c.esp==SP+4&&c.ebx==initial.ebx&&c.ebp==initial.ebp&&c.esi==initial.esi&&c.edi==initial.edi);
    assert(scheduled==before+1);
}
enum class Replay {Retail,Current,Legacy};
void SequentialPassage(const Sample& sample,Replay replay,bool p2_first=false,bool coop_active=true){
    Setup(sample);trial_coop::SetActive(coop_active);
    const bool old_census=replay==Replay::Legacy;
    const auto update=[&]{Update(replay!=Replay::Retail,old_census);};
    Position(0,false);Position(1,false);update();assert(!entered);
    unsigned expected=0;
    for(unsigned step=0;step<2;++step){
        const unsigned slot=p2_first?1-step:step;
        Position(slot,true);update();
        if(slot==0&&(!old_census||!coop_active))expected=1;
        assert(entered==expected); // P2 alone must never become native PLAYER.
        Position(slot,false);update();assert(entered==expected);
    }
    // With both actors already beyond the volume, the old population event
    // remains unfired. The corrected/original single-player path stays at one.
    for(unsigned tick=0;tick<3;++tick){update();assert(entered==expected);}
    assert(entered==unsigned(!old_census||!coop_active));
    // This tests scheduling of the original script, not a replacement command
    // interpreter or a new P2 PLAYER alias. No frame placed both actors inside.
}
X86Context HookContext(const Sample& sample){
    auto c=Context();c.esi=T;c.ebp=V+sample.ordinal*0xc0;c.df=1;
    for(c.edi=0;c.edi<Read(B(),T+0xd8);++c.edi)
        if(Read(B(),R+c.edi*8)==1&&Read(B(),R+c.edi*8+4)==sample.group)return c;
    assert(false);return c;
}
void HostageRendezvous(const Sample& sample){
    assert(sample.count==1710&&sample.source==1699&&sample.ordinal==67&&sample.group==1);
    // Native PLAYER enters alone after its partner has crossed the rear door.
    // Replay both walking orders; the thin tripwire never contains both actors.
    for(bool old_policy:{true,false})for(bool p2_first:{false,true}){
        Setup(sample);
        const auto clear=[&](unsigned slot,bool ahead){
            Position(slot,true);Real(P[slot]+0x48,Value(BX+(ahead?20:8))+(ahead?200.f:-200.f));
        };
        clear(0,false);clear(1,false);Update(true,old_policy);assert(!entered);
        for(unsigned step=0;step<2;++step){
            const unsigned slot=p2_first?1-step:step;
            Position(slot,true);Update(true,old_policy);
            clear(slot,true);Update(true,old_policy);
        }
        assert(entered==unsigned(!old_policy));
    }
    Setup(sample);Position(1,false);Real(P[1]+0x40,1500);Real(P[1]+0x44,-400);Real(P[1]+0x48,-2800);
    Update(true);assert(entered==1); // safe room, outside cinematic tripwire
    // Earlier checkpoint832 is on the wrong floor/side despite x>1060.
    Setup(sample);Real(P[1]+0x40,2400);Real(P[1]+0x44,2);Real(P[1]+0x48,-200);Update(true);assert(!entered);
    Real(P[1]+0x44,-400);Update(true);assert(!entered); // near-side corridor guard
    Real(P[1]+0x48,-2900);Real(P[1]+0x44,2);Update(true);assert(!entered); // floor guard
    Real(P[1]+0x44,-393.954f);Real(P[1]+0x48,-3200);Update(true);assert(entered==1); // P2 crossed first
    for(float x:{622.224f,1000.f,1060.f,1070.f}){
        Setup(sample);Real(P[1]+0x40,x);Update(true);assert(!entered);
    }
    Setup(sample);Real(P[1]+0x40,1070.01f);Update(true);assert(entered==1);
    Setup(sample);Real(P[1]+0x40,1200);Real(P[1]+0x88,200.f*200.f);Update(true);assert(!entered);
    Real(P[1]+0x40,1260);Update(true);assert(!entered); // sphere tangent to safe plane
    Real(P[1]+0x40,1261);Update(true);assert(entered==1);
    for(unsigned offset:{0x40u,0x44u,0x48u,0x88u})for(uint32_t bad:{0x7f800000u,0xff800000u,0x7fc00000u}){
        Setup(sample);Put(P[1]+offset,bad);Update(true);assert(!entered);
    }
    Setup(sample);Real(P[1]+0x88,-1.f);Update(true);assert(!entered);
    Setup(sample);Real(P[1]+0x40,1500);Real(P[1]+0x9c,0);Update(true);assert(!entered);
    Real(P[1]+0x9c,100);Update(true);assert(entered==1);
    Setup(sample);Real(P[1]+0x40,1500);Put(P[1]+0x170,12);Update(true);assert(!entered);
    // Live volume drift disables only the wider partner predicate, retaining
    // both actors' native overlap. Single-player remains the original routine.
    Setup(sample);Real(P[1]+0x40,1500);Real(BX,Value(BX)+1);Update(true);assert(!entered);
    Position(1,true);Update(true);assert(entered==1);
    Setup(sample);Real(P[1]+0x40,500);trial_coop::SetActive(false);Update(true);assert(entered==1);
    Setup(sample);Real(P[1]+0x40,1500);auto context=HookContext(sample);auto expected=context;
    const auto csr=_mm_getcsr();_mm_setcsr(0x3f80);TrialCoopProgressionEnter(context,B());
    assert(!std::memcmp(&context,&expected,sizeof context)&&_mm_getcsr()==0x3f80);
    Real(P[1]+0x40,500);TrialCoopProgressionEnter(context,B());expected.eax&=0xffffff00;
    assert(!std::memcmp(&context,&expected,sizeof context)&&_mm_getcsr()==0x3f80);_mm_setcsr(csr);
    // Preserve original exit group2 (KillCop script0); camera owns P1 containment.
    Setup(sample);Update(true);assert(entered==1);
    Position(0,false);Update(true);assert(entered==1&&last_group==2&&Read(B(),T+0xd4)==2);
    puts("Hope Street hostage tripwire: old strict overlap 0/2 sequential entries; safe-side rendezvous 2/2; rear boundary, sphere, down/revive, finite, volume drift and native exit controls passed");
}
void HostageBarrier(const Sample& sample){
    assert(sample.count==1710&&sample.kind==17&&sample.subject_count==3);
    const auto gate=std::find_if(kGates.begin(),kGates.end(),[&](const auto& g){return g.source_index==sample.source&&g.provider_slot_count==sample.count;});
    assert(gate!=kGates.end()&&gate->completion_flag==177&&gate->barrier_source==1296);
    // Exact original ALL-subject predicate includes P1 and both authored NPCs.
    // P2 is not a native subject: retail creates the barrier while it is behind.
    for(bool fixed:{false,true}){
        Setup(sample);Real(P[1]+0x48,-2437);Update(fixed);assert(entered==unsigned(!fixed));
        if(fixed){
            Position(1,true);Update(true);assert(entered==1);Update(true);assert(entered==1);
            Position(0,false);Update(true);assert(Read(B(),T+0xd4)==2&&(B()[V+sample.ordinal*0xc0+0x6c]&2));
            Position(0,true);Update(true);assert(entered==1); // native once-on-exit rule
        }
    }
    for(bool p2_first:{false,true}){
        Setup(sample);Real(P[0]+0x48,-2437);Real(P[1]+0x48,-2437);Update(true);assert(!entered);
        Position(p2_first?1:0,true);Update(true);assert(!entered);
        Position(p2_first?0:1,true);Update(true);assert(entered==1);
    }
    for(unsigned npc=0;npc<2;++npc){
        Setup(sample);Real(NPC[npc]+0x48,-4000);Update(true);assert(!entered);
        std::memcpy(B()+NPC[npc]+0x40,B()+P[0]+0x40,12);Update(true);assert(entered==1);
        Setup(sample);Put(ENTRIES+(npc+1)*0x20+8,0);Update(true);assert(!entered);
    }
    for(unsigned slot=0;slot<2;++slot){
        Setup(sample);Real(P[slot]+0x88,6400);Real(P[slot]+0x48,gate->barrier_front_z+60);Update(true);assert(!entered);
        Real(P[slot]+0x48,gate->barrier_front_z+80);Update(true);assert(!entered); // tangent still overlaps
        Real(P[slot]+0x48,gate->barrier_front_z+81);Update(true);assert(entered==1);
        Setup(sample);Real(P[slot]+0x9c,0);Update(true);assert(!entered);Real(P[slot]+0x9c,100);Update(true);assert(entered==1);
        Setup(sample);Put(P[slot]+0x170,12);Update(true);assert(!entered);
        for(unsigned offset:{0x40u,0x44u,0x48u,0x88u})for(uint32_t bad:{0x7fc00000u,0x7f800000u,0xff800000u}){
            Setup(sample);Put(P[slot]+offset,bad);auto c=HookContext(sample);TrialCoopProgressionEnter(c,B());assert(!(c.eax&255));
        }
    }
    // No late wall after the original Wolf script has retired it/set local177,
    // including completion while P2 is still outside the pending entry.
    for(bool pending:{false,true}){
        Setup(sample);if(pending){Real(P[1]+0x48,-2437);Update(true);assert(!entered);}
        auto flag_call=Context();flag_call.ecx=Q;Put(SP+4,177);__imp__sub_00210770(flag_call,B());assert(B()[FLAGS+177]==1);Position(1,true);Update(true);Update(true);assert(!entered);
        Position(0,false);Update(true);Position(0,true);Update(true);assert(!entered);
    }
    // The original script's local177 differs from its global flag map. Mapping
    // count23 must not reject179-byte storage; an equal local bound must reject.
    Setup(sample);assert(Read(B(),Q+0x30)==179&&Read(B(),Q+0x38)==23);Update(true);assert(entered==1);
    for(uint32_t size:{0u,177u,65537u}){Setup(sample);Put(Q+0x30,size);Update(true);assert(!entered);}
    Setup(sample);Put(Q+0x2c,0xfffffff0u);Update(true);assert(!entered);
    for(unsigned drift=0;drift<6;++drift){
        Setup(sample);
        if(drift==0)B()[BARRIER_DESC]^=1;
        if(drift==1)Real(BX,Value(BX)+1);
        if(drift==2)Put(V+sample.ordinal*0xc0+0x98,SUBJECTS+8);
        if(drift==3)Put(V+sample.ordinal*0xc0+0x54,0);
        if(drift==4)Put(ENTRIES+0x8c,0);
        if(drift==5)Put(SUBJECTS+8,999);
        auto c=HookContext(sample);TrialCoopProgressionEnter(c,B());assert(!(c.eax&255));
    }
    // Unknown roots/scripts/kinds remain retail; recognized blocking roots with
    // changed geometry withhold only their sole create command.
    for(unsigned drift=0;drift<3;++drift){
        Setup(sample);Real(P[1]+0x48,-2437);
        if(drift==0)Put(T+0x50,sample.source+1);
        if(drift==1)B()[C]^=1;
        if(drift==2)Put(V+sample.ordinal*0xc0+0x50,37);
        auto c=HookContext(sample);const auto expected=c;TrialCoopProgressionEnter(c,B());assert(!std::memcmp(&c,&expected,sizeof c));
    }
    Setup(sample);Real(P[1]+0x48,-2437);trial_coop::SetActive(false);Update(true);assert(entered==1);
    Setup(sample);auto c=HookContext(sample),expected=c;const auto csr=_mm_getcsr();
    std::array<uint8_t,0x890> snapshots[2];for(unsigned i=0;i<2;++i)std::memcpy(snapshots[i].data(),B()+P[i],snapshots[i].size());
    TrialCoopProgressionEnter(c,B());assert(!std::memcmp(&c,&expected,sizeof c)&&_mm_getcsr()==csr);
    for(unsigned i=0;i<2;++i)assert(!std::memcmp(snapshots[i].data(),B()+P[i],snapshots[i].size()));
    Real(P[1]+0x48,-2437);TrialCoopProgressionEnter(c,B());expected.eax&=0xffffff00;
    assert(!std::memcmp(&c,&expected,sizeof c)&&_mm_getcsr()==csr);
    std::printf("Hope Street barrier%u: native kind17 separation witness, both crossing orders, NPC requirements, full-sphere clearance, local177 completion, provenance and single-player controls passed\n",sample.ordinal);
}
int main(){
    unsigned selected=0,ordinary=0,removed=0,garage=0,pickup=0;
    for(const auto& sample:samples){
        if(sample.control==Control::HostageBarrier){HostageBarrier(sample);++selected;continue;}
        Setup(sample);Position(1,false);Update(false);
        assert(started==1&&Read(B(),T+0xd4)==1); // original one-player lockout witness
        Setup(sample);Position(1,false);Update(true);
        if(!sample.selected){
            assert(started==1&&Read(B(),T+0xd4)==1);++ordinary;
            if(sample.control!=Control::ShieldPickup){
                ++removed;
                if(sample.control==Control::GaragePopulation)++garage;
                for(bool p2_first:{false,true}){
                    SequentialPassage(sample,Replay::Legacy,p2_first); // old census suppresses population
                    SequentialPassage(sample,Replay::Current,p2_first); // current enter schedules it once
                    SequentialPassage(sample,Replay::Retail,p2_first,false); // unpatched native control
                    SequentialPassage(sample,Replay::Current,p2_first,false); // single-player passthrough
                }
            }
            if(sample.control==Control::ShieldPickup){
                ++pickup;
                // Both policies leave ordinary pickup admission unchanged.
                Setup(sample);Position(1,false);Update(true,true);assert(started==1);
                SequentialPassage(sample,Replay::Current);
            }
            continue;
        }
        ++selected;assert(!started&&Read(B(),T+0xd4)==0&&!Read(B(),T+0xc0)&&!Read(B(),T+0xd0));
        Position(1,true);Update(true);assert(started==1&&Read(B(),T+0xd4)==1&&last_group==sample.group);
        Setup(sample);Put(T+0xd4,2);Position(1,false);Update(true);assert(!started&&Read(B(),T+0xd4)==2);
        Position(1,true);Update(true);assert(started==1&&Read(B(),T+0xd4)==1);
        Setup(sample);Position(0,false);Update(true);assert(!started); // P2 does not rewrite global PLAYER alias
        Setup(sample);Position(1,false);trial_coop::SetActive(false);Update(true);assert(started==1);
        Setup(sample);Real(P[1]+0x9c,0);Update(true);assert(!started);
        Real(P[1]+0x9c,100);Update(true);assert(started==1); // revived partner permits the untouched event
        Setup(sample);Real(P[0]+0x9c,0);Real(P[1]+0x9c,0);Update(true);assert(!started);
        // Thin door volumes must admit two separate bounds, not just two actors
        // placed at the same point. Keep the original sphere/box overlap rule.
        Setup(sample);
        const unsigned axis=Value(BX+12)-Value(BX)>Value(BX+20)-Value(BX+8)?0:2;
        // 2D38CA compares this field directly with squared distance. These
        // 100-unit fixture spheres test admission, not live model clearance.
        const float center=Value(P[0]+0x40+axis*4),radius=100.f;
        Real(P[0]+0x88,radius*radius);Real(P[1]+0x88,radius*radius);
        Real(P[0]+0x40+axis*4,center-radius);Real(P[1]+0x40+axis*4,center+radius);
        Update(true);assert(started==1);
        if(sample.control==Control::HostageTripwire)HostageRendezvous(sample);
    }
    assert(selected==std::size(kGates)&&selected+ordinary==std::size(samples)&&garage==1&&pickup==1);
    assert(selected+removed==std::size(legacy_progression_assets::kGates)&&ordinary==removed+pickup);
    const auto& sample=samples[0];
    Setup(sample);auto c=HookContext(sample);auto before=c;const auto csr=_mm_getcsr();_mm_setcsr(0x3f80);
    TrialCoopProgressionEnter(c,B());assert(!std::memcmp(&c,&before,sizeof c)&&_mm_getcsr()==0x3f80);
    Position(1,false);_mm_setcsr(0x3f80);TrialCoopProgressionEnter(c,B());before.eax&=0xffffff00;
    assert(!std::memcmp(&c,&before,sizeof c)&&_mm_getcsr()==0x3f80);_mm_setcsr(csr);
    // Exit and one-time startup rows retain the original transition contract.
    Setup(sample);Put(R,2);Put(T+0xd8,1);Put(T+0xd4,1);Position(0,false);Position(1,false);Update(true);
    assert(started==1&&Read(B(),T+0xd4)==2&&(B()[V+sample.ordinal*0xc0+0x6c]&2));
    Setup(sample);Put(R,0);Put(T+0xd8,1);Position(1,false);Update(true);assert(started==1&&B()[T+0xd0]);
    for(unsigned invalid=0;invalid<8;++invalid){
        Setup(sample);Position(1,false);auto q=HookContext(sample);const auto wanted=q;
        if(invalid==0)Put(T,0x480889);
        if(invalid==1)Put(E+4,201);
        if(invalid==2)B()[C]^=1;
        if(invalid==3)Put(P[1]+0x5c,1);
        if(invalid==4)Put(T+0xd4,1);
        if(invalid==5)Put(V+sample.ordinal*0xc0+0x78,BX+24);
        if(invalid==6)Put(A,65);
        if(invalid==7)Put(R+q.edi*8+4,0xffffffff);
        TrialCoopProgressionEnter(q,B());assert(!std::memcmp(&q,&wanted,sizeof q));
    }
    // Cache lifetime resets even when every guest address/identity is reused.
    Setup(sample);Position(1,false);c=HookContext(sample);TrialCoopProgressionEnter(c,B());assert(!(c.eax&255));
    TrialCoopProgressionReset();B()[C]^=1;c=HookContext(sample);before=c;TrialCoopProgressionEnter(c,B());
    assert(!std::memcmp(&c,&before,sizeof c));
    puts("co-op progression: original enter/re-enter/exit state machine, campaign script/volume inputs, old-census population suppression, both sequential crossing orders across every removed root, single-player passthrough, untouched shield pickup, both-player overlap, down/revive, lifecycle and context guards passed (offline; live campaign events pending)");
}

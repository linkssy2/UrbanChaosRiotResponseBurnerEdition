#undef NDEBUG
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <vector>
#include "xbe_ragdoll_drag.cpp"

namespace drag_test {
alignas(16) std::array<uint8_t,0x800000> memory{};
constexpr uint32_t P=0x600000,Q=0x602000,W=0x610000,L=0x620000,A=0x630000,Z=0x632000,
    R=0x640000,T=0x642000,LIST=0x650000,BODY=0x660000,MOTION=0x670000,ISLAND=0x680000,
    RESULT=0x690000,MAP=0x6c0000,PADS=0x6d0000,SP=0x780000;
bool downed=false,committed=true,wall=false,miss=false,reenter_cancel=false,origin_hit=false,clobber_result=false;
unsigned selected[2]{},hit_limb=10,presentation=0,queries[2]{},rays=0,wakes=0,forces=0,activations=0;
uint32_t handles[2]{1,2};
std::vector<uint32_t> transcript;
uint8_t* B(){return memory.data();}
uint32_t U(uint32_t a){return ragdoll_drag::Word(B(),a);}
void U(uint32_t a,uint32_t v){ragdoll_drag::Put(B(),a,v);}
float F(uint32_t a){return ragdoll_drag::Real(B(),a);}
void F(uint32_t a,float v){std::memcpy(B()+a,&v,4);}
uint32_t Owner(unsigned slot){return slot?Q:P;}
uint32_t Actor(unsigned which){return which?Z:A;}
uint32_t Ragdoll(unsigned which){return which?T:R;}
uint32_t Limb(unsigned which,unsigned limb=10){return BODY+which*0x2000+limb*0x100;}
uint32_t Motion(unsigned which,unsigned limb=10){return MOTION+which*0x2000+limb*0x100;}
}
namespace mem {bool IsCommitted(uint32_t page){return drag_test::committed&&page<drag_test::memory.size();}}
uint32_t TrialCoopPlayer(unsigned slot){return slot<2?drag_test::Owner(slot):0;}
unsigned TrialCoopPresentation(){return drag_test::presentation;}
bool TrialCoopKeepDownedBody(const uint8_t*,uint32_t actor){return drag_test::downed&&actor==drag_test::A;}
bool TrialRagdollFindLimb(const uint8_t* b,uint32_t body,uint32_t& handle,uint32_t& ragdoll,unsigned& limb){
    using namespace drag_test;
    for(unsigned which=0;which<2;++which)for(unsigned i=0;i<12;++i){
        const uint32_t r=Ragdoll(which);
        if(body==Limb(which,i)&&handles[which]!=UINT32_MAX&&ragdoll_drag::Word(b,r+0x1c)&&ragdoll_drag::Word(b,r+0x20)){
            if(clobber_result)std::memset(memory.data()+RESULT,0,0x30);
            handle=handles[which];ragdoll=r;limb=i;return true;
        }
    }
    return false;
}
bool TrialRagdollMatchesLimb(const uint8_t* b,uint32_t handle,uint32_t ragdoll,unsigned limb,uint32_t body){
    uint32_t current,owner;unsigned index;
    return TrialRagdollFindLimb(b,body,current,owner,index)&&current==handle&&owner==ragdoll&&index==limb;
}
X86_FUNC(sub_002F4FF0);X86_FUNC(sub_002ECAA0);X86_FUNC(sub_000567F0);
X86_FUNC(sub_002F6CC0){
    using namespace drag_test;assert(U(ctx.esp+4)==W&&U(ctx.esp+8)==ISLAND);++activations;
    B()[ISLAND+0x28]=1;ctx.esp+=4;
}
void x86_icall(X86Context& c,uint8_t* b,uint32_t target){
    using namespace drag_test;transcript.push_back(target);
    if(target==0x567f0){
        const auto slot=U(c.esp+4);assert(c.ecx==W&&slot<2&&U(c.esp+12)==0);
        ++queries[slot];sub_000567F0(c,b);
        if(reenter_cancel){TrialRagdollDragInput(slot,false);TrialRagdollDragInput(slot,true);reenter_cancel=false;}
        return;
    }
    if(target==0x214230){
        ++rays;assert(U(c.esp+12)==9&&U(c.esp+20)==0x4000&&U(c.esp+24)==0);
        const unsigned slot=U(c.esp+16)-17;assert(slot<2);
        const uint32_t from=U(c.esp+4),to=U(c.esp+8);assert(F(from)==F(Owner(slot)+0x6e0));
        if(miss){c.eax=0;c.esp+=4;return;}
        const unsigned which=selected[slot];
        std::memcpy(b+RESULT,b+to,12);F(RESULT+0x18,origin_hit?0.f:100.f);
        U(RESULT+0x24,origin_hit?0:wall?BODY+0x8000:Limb(which,hit_limb));U(RESULT+0x2c,origin_hit||wall?0:Actor(which));
        c.eax=RESULT;c.esp+=4;return;
    }
    if(target==0x2ecaa0){++wakes;assert(c.ecx==Limb(0,hit_limb)||c.ecx==Limb(1,hit_limb));sub_002ECAA0(c,b);return;}
    if(target==0x2f4ff0){++forces;assert(c.ecx==Motion(0,hit_limb)||c.ecx==Motion(1,hit_limb));sub_002F4FF0(c,b);return;}
    std::abort();
}
[[noreturn]]void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
void x86_resume(X86Context&,uint8_t*,const X86Resume&){std::abort();}
// Behavior allocation and event storage are boundaries; retail DF380 sets the knockout state.
X86_FUNC(sub_000159C0){ctx.esp+=12;}
X86_FUNC(sub_001F8EA0){ctx.esp+=8;}
X86_FUNC(sub_001F8DE0){ctx.esp+=16;}
X86_FUNC(sub_001F8BA0){ctx.esp+=4;}
X86_FUNC(sub_001F92B0){ctx.eax=0;ctx.esp+=4;}
X86_FUNC(sub_0020A260){ctx.eax=0;ctx.esp+=16;}
X86_FUNC(sub_00032CB0){ctx.esp+=8;}
// NATIVE_DRAG_CONTRACTS

namespace drag_test {
void Register(uint32_t object,uint32_t id){U(object+0x54,id);U(L+0x3c+(id&1023)*8,id);U(L+0x40+(id&1023)*8,object);}
void Seed(bool coop=true){
    memory.fill(0);ragdoll_drag::players[0]={};ragdoll_drag::players[1]={};
    downed=wall=miss=reenter_cancel=origin_hit=clobber_result=false;
    committed=true;presentation=rays=wakes=forces=activations=0;
    selected[0]=0;selected[1]=1;hit_limb=10;handles[0]=1;handles[1]=2;transcript.clear();trial_coop::SetActive(coop);
    U(0x51c4dc,W);U(W+4,L);F(W+8,1);F(W+0x14,1.f/60.f);U(W+0x10,0x80);
    // Native held query reads the same A bytes populated by the host Interact adapter.
    U(0x5096e0,MAP);U(W+0x20,PADS);
    // NATIVE_INPUT_CONSTANTS
    for(unsigned slot=0;slot<2;++slot){
        B()[MAP+0xc+slot]=uint8_t(slot);B()[MAP+0x10+slot]=uint8_t(slot);B()[MAP+0x14+slot]=1;
        ragdoll_drag::input[slot]=0;queries[slot]=0;TrialRagdollDragInput(slot,true);
        const auto p=Owner(slot),r=Ragdoll(slot),a=Actor(slot);
        U(p,0x46fbe8);Register(p,100+slot);U(p+0x368,slot);F(p+0x9c,100);U(p+0x94,17+slot);
        // Game Y is vertical; this camera looks along game +X.
        F(p+0x6b8,1);F(p+0x6c4,1);F(p+0x6d0,1);F(p+0x6e4,100);
        U(a,0x45c770);Register(a,200+slot);F(a+0x9c,0);
        U(r,0x48dd60);U(r+0x1c,0x6a0000+slot*0x100);U(r+0x20,0x6b0000+slot*0x100);
        U(r+0x70,LIST+slot*0x100);U(r+0x74,12);
        for(unsigned i=0;i<12;++i){
            const auto body=Limb(slot,i),m=Motion(slot,i);U(LIST+slot*0x100+i*4,body);
            U(body+0x50,m);U(body+0x54,ISLAND);U(body+8,W);U(m,0x49a8b8);
            F(m+0xc0,.1f);F(m+0x60,1);F(m+0x64,0);F(m+0x68,.2f);
        }
    }
    B()[ISLAND+0x28]=1;
    std::fill(memory.begin()+SP-4096,memory.begin()+SP,uint8_t(0x6a));
}
X86Context Context(unsigned slot=0){
    X86Context c{};c.ecx=Owner(slot);c.esp=SP;c.eax=71;c.ebx=83;c.ebp=97;c.esi=101;c.edi=107;
    c.top=3;c.st[3]=17;return c;
}
void CheckPreserved(const X86Context& before,const X86Context& after,const std::array<uint8_t,4096>& stack,unsigned csr){
    assert(!std::memcmp(&before,&after,sizeof before));assert(_mm_getcsr()==csr);
    assert(!std::memcmp(stack.data(),B()+SP-4096,stack.size()));
}
void Update(unsigned slot,bool button,float dt=1.f/60.f){
    B()[PADS+slot*0xd0+0x10]=button?3:0;F(W+0x14,dt);F(W+8,F(W+8)+std::max(0.f,dt));
    auto c=Context(slot);const auto before=c;std::array<uint8_t,4096> stack;std::memcpy(stack.data(),B()+SP-4096,stack.size());
    const auto csr=_mm_getcsr();TrialRagdollDragUpdate(c,B());CheckPreserved(before,c,stack,csr);
}
void Step(unsigned which=0,float dt=1.f/60.f){
    auto c=Context();c.esi=Ragdoll(which);std::memcpy(&c.ebx,&dt,4);
    const auto before=c;std::array<uint8_t,4096> stack;std::memcpy(stack.data(),B()+SP-4096,stack.size());
    const auto csr=_mm_getcsr();TrialRagdollDragStep(c,B());CheckPreserved(before,c,stack,csr);
}
bool Held(unsigned slot=0){return ragdoll_drag::players[slot].grab.body!=0;}
void Hold(unsigned slot,float dt=1.f/60.f){for(unsigned n=0;n<unsigned(std::ceil(.5f/dt));++n)Update(slot,true,dt);}
void Acquire(unsigned slot=0){Update(slot,false);Hold(slot);assert(Held(slot));}
void InteractMapping(){
    for(bool coop:{false,true})for(unsigned slot=0;slot<(coop?2u:1u);++slot){
        Seed(coop);Update(slot,false);
        // Cycle Firearm/X must not start a drag, even while held past the threshold.
        B()[PADS+slot*0xd0+0x28]=3;
        for(unsigned frame=0;frame<40;++frame)Update(slot,false);
        assert(!Held(slot)&&rays==0);
        B()[PADS+slot*0xd0+0x28]=0;Hold(slot);assert(Held(slot));
        assert(B()[PADS+slot*0xd0+0x10]==3); // Read-only query preserves the Interact press edge.
        assert(!Held(slot^1));
    }
}
void Latch(){
    for(bool coop:{false,true})for(float fps:{30.f,60.f,120.f,240.f,360.f}){
        Seed(coop);const float dt=1.f/fps;Hold(0,dt);assert(!Held()); // Fresh neutral after startup.
        Update(0,false,dt);for(unsigned i=0;i<unsigned(fps*.3f);++i)Update(0,true,dt);assert(!Held());
        Update(0,false,dt);Hold(0,dt);assert(Held());const auto body=ragdoll_drag::players[0].grab.body;
        Hold(0,dt);assert(Held()&&ragdoll_drag::players[0].grab.body==body); // One toggle per hold.
        Update(0,false,dt);assert(Held());Hold(0,dt);assert(!Held());
        Hold(0,dt);assert(!Held());Update(0,false,dt);Hold(0,dt);assert(Held());
    }
    Seed();Acquire();Acquire(1);Update(0,false);Hold(0);assert(!Held()&&Held(1));
    Seed();selected[1]=0;Acquire();Update(1,false);Hold(1);assert(Held()&&!Held(1));
    Seed(false);Update(1,false);Hold(1);assert(!Held(1)&&queries[1]==0);
    Seed();clobber_result=true;Acquire();assert(Held()); // Borrowed result is copied before another service runs.
}
void GrabTargets(){
    for(bool coop:{false,true})for(unsigned slot=0;slot<(coop?2u:1u);++slot)for(unsigned part=0;part<12;++part){
        Seed(coop);hit_limb=part;Update(slot,false);Hold(slot);
        if(part<2){assert(!Held(slot)&&forces==0);continue;}
        assert(ragdoll_drag::players[slot].grab.body==Limb(slot,part));
        Step(slot);assert(forces==1&&F(Motion(slot,part)+0xd8)>0);
        for(unsigned other=0;other<12;++other)if(other!=part)assert(F(Motion(slot,other)+0xd8)==0);
        Update(slot,false);Hold(slot);assert(!Held(slot));
        F(Actor(slot)+0x9c,100);Update(slot,false);Hold(slot);assert(!Held(slot));
    }
}
void KnockOut(unsigned slot){
    const auto actor=Actor(slot),owner=0x6e0000+slot*0x100,data=0x6f0000+slot*0x100;
    F(actor+0x9c,75);
    auto c=Context(slot);c.ecx=actor;c.fs_base=0x700000;
    U(SP+4,RESULT+0x100);U(SP+8,0);B()[RESULT+0x105]=1;
    sub_000DF380(c,B());assert(c.esp==SP+12&&(U(actor+0x198)&4)&&F(actor+0x9c)==75);
    // The native behavior supplies this owned pool handle after its fall animation.
    U(actor+0x118,owner);U(owner,0x48e6e0);U(owner+12,data);U(data+0x50,handles[slot]);
}
void KnockoutTargets(){
    for(bool coop:{false,true})for(unsigned slot=0;slot<(coop?2u:1u);++slot)
    for(uint32_t type:{0x45c770u,0x47bf50u})for(unsigned part:{2u,4u,8u,10u}){
        Seed(coop);U(Actor(slot),type);KnockOut(slot);hit_limb=part;
        std::array<uint8_t,0x200> actor;std::memcpy(actor.data(),B()+Actor(slot),actor.size());
        Acquire(slot);Step(slot);assert(Held(slot)&&forces==1);
        Update(slot,false);Hold(slot);assert(!Held(slot));
        assert(!std::memcmp(actor.data(),B()+Actor(slot),actor.size())); // Dragging cannot alter health, KO/reward state or AI.
    }
    for(unsigned reason=0;reason<8;++reason){
        Seed();KnockOut(0);Acquire();Update(0,false);
        switch(reason){
        case 0:U(A+0x198,0);break; // Native recovery/reset.
        case 1:U(0x6f0050,UINT32_MAX);break; // The behavior released its ragdoll.
        case 2:U(0x6f0050,handles[1]);break; // Stale/another actor's handle.
        case 3:U(A+0x5c,1);break; // Entity removal.
        case 4:U(A,0x45d040);break; // Healthy co-op player is never a knockout target.
        case 5:F(A+0x9c,std::numeric_limits<float>::quiet_NaN());break;
        case 6:U(0x6e000c,0xfffffff0);break;
        case 7:U(0x6e0000,0);break;
        }
        const auto count=forces;Step();assert(!Held()&&forces==count);
        Update(0,false);Hold(0);assert(!Held());
    }
    Seed();KnockOut(0);KnockOut(1);Acquire();Acquire(1);Update(0,false);Step();Step(1);assert(Held()&&Held(1)&&forces==2);
    Seed();KnockOut(0);selected[1]=0;Acquire();Update(1,false);Hold(1);assert(Held()&&!Held(1));
    Seed();KnockOut(0);U(0x6f0050,UINT32_MAX);Update(0,false);Hold(0);assert(!Held()); // Knockout flag before ragdoll creation.
}
void Retry(){
    for(float fps:{30.f,60.f,120.f,240.f,360.f})for(unsigned rejection=0;rejection<3;++rejection){
        Seed();const float dt=1.f/fps;Update(0,false,dt);
        miss=rejection==0;wall=rejection==1;hit_limb=rejection==2?0:10;
        double elapsed=0,last_ray=0;
        for(unsigned frame=0;frame<unsigned(fps*1.5f);++frame){
            const auto before=rays;Update(0,true,dt);elapsed+=dt;
            assert(!Held()&&!ragdoll_drag::players[0].fired&&forces==0);
            if(rays!=before){
                assert(rays==before+1);
                assert(elapsed-last_ray>=(before?.1:.45)-.000001);
                last_ray=elapsed;
            }
        }
        assert(rays>=9&&rays<=11); // Failed holds stay responsive without raycasting every frame.
        miss=wall=false;hit_limb=10;const auto before=rays;
        for(unsigned frame=0;!Held()&&frame<unsigned(std::ceil(.1/dt))+1;++frame)Update(0,true,dt);
        assert(Held()&&rays==before+1); // Moving onto a limb during the same hold acquires it.
        const auto body=ragdoll_drag::players[0].grab.body;Hold(0,dt);
        assert(Held()&&ragdoll_drag::players[0].grab.body==body);
        Update(0,false,dt);Hold(0,dt);assert(!Held());Hold(0,dt);assert(!Held());
    }
}
void Invalidations(){
    for(unsigned reason=0;reason<15;++reason){
        Seed();Acquire();Update(0,false);
        switch(reason){
        case 0:TrialRagdollDragInput(0,false);TrialRagdollDragInput(0,true);break;
        case 1:F(P+0x9c,0);break;
        case 2:B()[P+0x202]=1;break;
        case 3:U(P+0x170,13);break;
        case 4:U(W+0x10,0x880);break;
        case 5:presentation=2;break;
        case 6:Register(A,300);break;
        case 7:F(A+0x9c,1);break;
        case 8:++handles[0];break;
        case 9:U(R+0x1c,0x6a1000);break;
        case 10:U(R+0x20,0);break;
        case 11:U(L+0x40+(200&1023)*8,Z);break;
        case 12:U(P+0x368,1);break;
        case 13:U(W+4,L+0x10000);break;
        case 14:F(W+8,F(W+8)+1);break;
        }
        const auto count=forces;Step();assert(!Held()&&forces==count);
    }
    Seed();Acquire();wall=true;Update(0,false);assert(!Held());
    Seed();Acquire();origin_hit=true;Update(0,false);assert(!Held()); // Zero-distance static hit is still an obstruction.
    Seed();wall=true;Update(0,false);Hold(0);assert(!Held());
    Seed();Acquire();F(P+0x6e0,1000);Update(0,false);assert(!Held());
    Seed();Acquire();F(Motion(0)+0xd0,std::numeric_limits<float>::quiet_NaN());Step();assert(!Held());
    Seed();Acquire();Step(0,0);assert(!Held());
    Seed();Acquire();Step(0,1);assert(!Held());
    Seed();Acquire();committed=false;Step();assert(!Held());committed=true;
    Seed();U(A,0x45d040);downed=true;Acquire();downed=false;Step();assert(!Held());
    Seed();Acquire();TrialRagdollDragInput(0,false);TrialRagdollDragInput(0,true);Hold(0);assert(!Held());
    Update(0,false);Hold(0);assert(Held());
    Seed();Update(0,false);reenter_cancel=true;Hold(0);assert(!Held());
    Seed();Acquire();auto c=Context();c.esp=0x10000;TrialRagdollDragUpdate(c,B());assert(!Held());
}
void Lift(){
    Seed();
    // Pinned2095A8/29E9AC construct game(0,-1700,0); execute its actual
    // 2A2870 conversion to establish native gravity(0,0,-17), hence physicsZ-up.
    auto c=Context();U(SP+4,RESULT+32);U(SP+8,RESULT);F(RESULT+4,F(0x2095a9));
    sub_002A2870(c,B());assert(F(RESULT+32)==0&&F(RESULT+36)==0&&F(RESULT+40)==-17.f);
    Acquire();const auto m=Motion(0);
    assert(std::abs(ragdoll_drag::players[0].grab.target[2]-.38f)<.00001f);
    // Lift stays world-up, rather than rotating into an added forward offset.
    const float sine=.5f,cosine=std::sqrt(.75f);
    F(P+0x6c0,-sine);F(P+0x6c4,cosine);F(P+0x6d0,cosine);F(P+0x6d4,sine);
    Update(0,false);assert(Held());
    const auto& target=ragdoll_drag::players[0].grab.target;
    assert(std::abs(target[0]-(cosine+.8f*sine))<.00001f);
    assert(std::abs(target[2]-(1.f+sine-.8f*cosine+.18f))<.00001f);
    const auto fixed=target;
    for(unsigned frame=0;frame<120;++frame)Update(0,false);
    assert(ragdoll_drag::players[0].grab.target==fixed); // Lift is not accumulated per frame.
    assert(F(m+0x60)==1.f&&F(m+0x64)==0&&F(m+0x68)==.2f); // Targeting never teleports a limb.
}
void Force(){
    // Native force integration executes; free-limb position/gravity integration models the solver boundary.
    for(float fps:{30.f,60.f,120.f,240.f,360.f})for(float inverse:{.02f,.1f,.5f}){
        Seed();Acquire();Update(0,false);const auto m=Motion(0);F(m+0xc0,inverse);
        const auto partner=std::vector<uint8_t>(B()+Motion(1),B()+Motion(1)+0x100);
        const float dt=1.f/fps;
        B()[ISLAND+0x28]=0;transcript.clear();Step(0,dt);
        assert(F(m+0xd0)==0&&F(m+0xd4)==0&&F(m+0xd8)>0);
        assert(F(m+0x60)==1.f&&F(m+0x64)==0&&F(m+0x68)==.2f);
        assert(transcript==std::vector<uint32_t>({0x2ecaa0,0x2f4ff0})&&activations==1);
        // A half-metre target change must settle promptly without free-limb rebound.
        F(m+0xd8,0);F(P+0x6e0,50);
        float previous=1.f,settled_at=0;
        for(unsigned frame=0;frame<unsigned(fps*2);++frame){
            Update(0,false,dt);Step(0,dt);assert(Held());
            F(m+0xd8,F(m+0xd8)-17.f*dt);
            for(unsigned axis=0;axis<3;++axis)F(m+0x60+axis*4,F(m+0x60+axis*4)+F(m+0xd0+axis*4)*dt);
            const float x=F(m+0x60),height=F(m+0x68);
            assert(std::isfinite(x)&&x>=previous-.00001f&&x<=1.50001f&&std::abs(F(m+0xd0))<11.001f);
            assert(std::isfinite(height)&&height>=.19f&&height<=.38001f);
            previous=x;if(!settled_at&&x>=1.475f)settled_at=float(frame+1)*dt;
        }
        assert(settled_at>0&&settled_at<=.31f);
        assert(std::abs(F(m+0x60)-1.5f)<.002f&&std::abs(F(m+0xd0))<.01f);
        assert(F(m+0x68)>.33f&&F(m+0x68)<.38f&&std::abs(F(m+0xd8))<.01f);
        assert(!std::memcmp(partner.data(),B()+Motion(1),partner.size()));
    }
    // Conservative loaded boundary: native joints share the selected limb's impulse
    // with 4x/16x its mass. The captured 160-mass body has 10/15-mass grab limbs.
    // This measures pull lag/rebound; it does not substitute for native contact solving.
    for(float fps:{30.f,60.f,120.f,240.f,360.f})for(float load:{4.f,16.f}){
        Seed();Acquire();Update(0,false);const auto m=Motion(0);const float dt=1.f/fps;
        F(P+0x6e0,50);float peak=1.f,reached=0;
        for(unsigned frame=0;frame<unsigned(fps*5);++frame){
            Update(0,false,dt);const float previous=F(m+0xd0);Step(0,dt);assert(Held());
            F(m+0xd0,previous+(F(m+0xd0)-previous)/load);
            F(m+0x60,F(m+0x60)+F(m+0xd0)*dt);
            // Floor/other contacts support the vertical load in this horizontal test.
            F(m+0x68,.38f);F(m+0xd8,0);
            const float x=F(m+0x60);assert(std::isfinite(x));peak=std::max(peak,x);
            if(!reached&&x>=1.475f)reached=float(frame+1)*dt;
        }
        assert(reached>0&&reached<=.5f&&peak<1.78f);
        assert(std::abs(F(m+0x60)-1.5f)<.01f&&std::abs(F(m+0xd0))<.04f);
    }
    // Bound acceleration in every direction even for a sharp mouse turn or a heavy limb.
    for(float inverse:{.02f,.1f,.5f}){
        Seed();Acquire();Update(0,false);F(Motion(0)+0xc0,inverse);F(P+0x6e0,140);Update(0,false);
        Step();const auto m=Motion(0);
        const float speed=std::sqrt(F(m+0xd0)*F(m+0xd0)+F(m+0xd4)*F(m+0xd4)+F(m+0xd8)*F(m+0xd8));
        assert(speed<=240.f/60.f+.00001f);
    }
    Seed();Acquire();static std::array<uint8_t,0x800000> before;before=memory;
    TrialRagdollDragInput(0,false);Step();
    assert(memory==before&&forces==0); // Host cancellation observed without another owner update.
}
void MovingTarget(){
    // Deliberately abrupt 4/6m/s travel and stops exceed the old3m/s cap;
    // a90-degree camera turn checks the same tracking while the held offset rotates.
    for(float fps:{30.f,60.f,120.f,240.f,360.f})for(float load:{1.f,16.f})for(unsigned path=0;path<3;++path){
        Seed();Acquire();Update(0,false);const auto m=Motion(0);const float dt=1.f/fps;
        for(unsigned frame=0;frame<unsigned(fps*6);++frame){
            const float time=float(frame+1)*dt;
            if(path<2)F(P+0x6e0,(path?600.f:400.f)*std::min(time,2.f));
            else{
                const float angle=std::min(time,1.f)*1.570796327f,sine=std::sin(angle),cosine=std::cos(angle);
                F(P+0x6b0,sine);F(P+0x6b8,cosine);F(P+0x6d0,cosine);F(P+0x6d8,-sine);
            }
            Update(0,false,dt);assert(Held());
            const float vx=F(m+0xd0),vy=F(m+0xd4);Step(0,dt);assert(Held());
            F(m+0xd0,vx+(F(m+0xd0)-vx)/load);F(m+0xd4,vy+(F(m+0xd4)-vy)/load);
            F(m+0x60,F(m+0x60)+F(m+0xd0)*dt);F(m+0x64,F(m+0x64)+F(m+0xd4)*dt);
            F(m+0x68,.38f);F(m+0xd8,0); // Vertical contacts support this horizontal load model.
        }
        const auto& target=ragdoll_drag::players[0].grab.target;
        if(std::abs(F(m+0xd0))>=.1f||std::abs(F(m+0xd4))>=.1f){
            std::printf("drag moving endpoint: fps=%g load=%g path=%u target=(%g,%g) position=(%g,%g) velocity=(%g,%g) feed=(%g,%g)\n",
                fps,load,path,target[0],target[1],F(m+0x60),F(m+0x64),F(m+0xd0),F(m+0xd4),
                ragdoll_drag::players[0].grab.velocity[0],ragdoll_drag::players[0].grab.velocity[1]);
            std::fflush(stdout);
        }
        assert(std::abs(F(m+0x60)-target[0])<.05f&&std::abs(F(m+0x64)-target[1])<.05f);
        assert(std::abs(F(m+0xd0))<.1f&&std::abs(F(m+0xd4))<.1f);
    }
}
}
int main(){drag_test::InteractMapping();drag_test::Latch();drag_test::GrabTargets();drag_test::KnockoutTargets();drag_test::Retry();drag_test::Invalidations();drag_test::Lift();drag_test::Force();drag_test::MovingTarget();
    std::puts("ragdoll drag: native Interact held query/X exclusion, hold/latch/retry/release, co-op exclusivity, identity cancellation, fixed world-up lift, bounded pull response at 30/60/120/240/360 FPS with free/loaded limbs, native force and context/stack guards passed; native joint/contact feel requires live testing.");}

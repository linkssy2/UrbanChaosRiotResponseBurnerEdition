// Native delivery/stock/receipt and eligibility branches are extracted from
// the pinned game. AI steering, animation playback and saving need live QA.
#undef NDEBUG
#include <array>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x800000> memory{};uint8_t* g_base=memory.data();}
namespace mem {bool IsCommitted(uint32_t page){return page<kernel::memory.size();}}
#include "../common/xbe_health_packs.cpp"
namespace hp_test {
constexpr uint32_t p0=0x610000,p1=0x612000,world=0x620000,medic=0x630000;
constexpr uint32_t behavior=0x631000,controller=0x632000,root=0x633000,manager=0x634000,stack=0x7f0000;
bool stored=false,duplicate=false,revive_success=false,throw_call=false;
uint32_t nearby=0;unsigned stock_calls=0,receipts=0,revives=0;
unsigned injection_sounds=0,self_pulses=0,animation_plays=0,lowers=0,raises=0;
uint32_t animation_owner=0,animation_path=0;
bool animation_complete=false,partner_clip_available=true,medic_root_available=true;
bool hypospray[2]{};
bool downed_position_valid=true;
float downed_position[3]{0,0,100};
unsigned presentation=0;
unsigned mode_hud_mask=1;
uint8_t* Bytes(){return kernel::memory.data();}
uint32_t Word(uint32_t p){uint32_t v;std::memcpy(&v,Bytes()+p,4);return v;}
void Put(uint32_t p,uint32_t v){std::memcpy(Bytes()+p,&v,4);}
void Float(uint32_t p,float v){std::memcpy(Bytes()+p,&v,4);}
X86Context Context(uint32_t owner){X86Context c{};c.esp=stack;c.ecx=owner;c.eax=0x1234;c.esi=0x5678;c.edi=0x9876;c.ebp=0xabcd;c.cf=1;c.pf=1;c.top=2;return c;}
void Setup(bool coop=true){
    kernel::memory.fill(0);health_packs::world=0;health_packs::medics.clear();health_packs::delivery=nullptr;health_packs::medic_owner=0;
    for(unsigned i=0;i<2;++i){health_packs::players[i]={};health_packs::published[i]=0;health_packs::pending[i]=0;health_packs::held[i]=false;health_packs::injections[i]={};hypospray[i]=false;}
    for(auto& r:health_packs::restored)r={};
    nearby=0;presentation=0;mode_hud_mask=1;revive_success=false;throw_call=false;stored=false;duplicate=false;stock_calls=receipts=revives=0;
    injection_sounds=self_pulses=animation_plays=lowers=raises=0;animation_owner=animation_path=0;animation_complete=false;partner_clip_available=medic_root_available=true;
    downed_position_valid=true;downed_position[0]=downed_position[1]=0;downed_position[2]=100;
    Float(0x44bf94,1);Float(0x4500e4,15000);
    Put(0x51ca80,0x730200);
    Put(0x51c4dc,world);std::memcpy(Bytes()+world+0x2c,"1-01",5);Put(world+0xf74,3);Put(world+0xf78,3);Float(0x44c178,100);
    Put(0x509a60,manager);Put(manager,0x44bf20);Put(0x44bf54,0x4af40);Put(manager+0x14,0xffffffff);Bytes()[0x4e71a8]=1;
    for(unsigned i=0;i<(coop?2u:1u);++i){const auto p=i?p1:p0;Put(p,0x46fbe8);Put(p+0x54,101+i);Put(p+0x368,i);Float(p+0x9c,1200);Float(p+0xa0,1200);Float(p+0x40,i?10:100);const uint32_t hud=0x638000+i*0x20;Put(p+0x300,hud);Put(hud,0x4531a0);Put(hud+4,p);Float(hud+0x14,1);Put(p+0x180,p+0x880);Put(p+0x880,p+0x884);auto c=Context(p);c.eax=p;TrialHealthPackCreated(c,Bytes());}
    for(unsigned slot=0;slot<(coop?2u:1u);++slot){
        const uint32_t owner=slot?p1:p0,hud=Word(owner+0x300),node=0x638100+slot*0x200;
        Put(hud+8,node);Put(node,0x453bd0);Put(node+4,node+0x20);
        Put(node+0x20,0x453bd0);Put(node+0x24,node+0x40);
        Put(node+0x40,0x453cd0);Put(node+0x48,node+0x60);
        Put(node+0x60,0x453bf8);Put(node+0x64,node+0x80);
        Put(node+0x80,0x453550);Put(node+0x84,owner);Float(node+0x88,1);Put(node+0x8c,node+0xa0);
        Put(node+0xa0,0x4531c0);Float(node+0xa8,120);
    }
    Put(medic+0x54,201);Put(medic+0x15c,0);Put(behavior+0xd4,medic);Put(behavior+0xd0,controller);Put(behavior+0x54,301);Put(behavior+0xf8,0xffffffff);
    Put(controller+0x1c,401);Put(controller+0x18,402);Put(root,0x46eef0);Put(root+0x88,medic);Put(root+0x54,401);
    for(uint32_t p:{p0,p1}){Float(p+0x6b0,1);Float(p+0x6c4,1);Float(p+0x6d8,1);Float(p+0x6ec,1);Put(p+0x38c,1);}
    Put(manager+0x834,101);Put(0x51c398,0x635000);Put(0x635008,0x636000);Put(0x4f9f18,77);
}
}
uint32_t TrialCoopNearbyDownedPlayer(const uint8_t*,uint32_t){return hp_test::nearby;}
bool TrialCoopDownedPosition(const uint8_t*,uint32_t owner,float (&position)[3]){
    if(!hp_test::downed_position_valid||owner!=hp_test::nearby)return false;
    std::memcpy(position,hp_test::downed_position,sizeof position);return true;
}
unsigned TrialCoopPresentation(){return hp_test::presentation;}
bool TrialCoopRevivePlayer(X86Context&,uint8_t*,uint32_t owner){assert(owner==hp_test::nearby);++hp_test::revives;return hp_test::revive_success;}
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
X86_FUNC(OriginalReceipt);X86_FUNC(StoredReceipt);X86_FUNC(OriginalStock);X86_FUNC(StoredStock);X86_FUNC(OriginalPlayerId);X86_FUNC(StoredPlayerId);
X86_FUNC(OriginalInjectionState);
X86_FUNC(sub_000C1A10){ctx.eax=0;ctx.esp+=4;}
X86_FUNC(sub_000BC6A0){assert(false);}X86_FUNC(sub_000BECB0){assert(false);}
X86_FUNC(sub_000BED30){assert(false);}X86_FUNC(sub_000BC640){assert(false);}X86_FUNC(sub_000BC760){assert(false);}
X86_FUNC(sub_000BFE00){OriginalInjectionState(ctx,base);}
X86_FUNC(sub_000C1E60){
    using namespace hp_test;assert(ctx.ecx==p0||ctx.ecx==p1);
    const auto action=Word(ctx.esp+4);assert(action==1||action==2);
    if(action==1)++lowers;else ++raises;animation_complete=false;ctx.esp+=16;
}
X86_FUNC(sub_000C8930){
    using namespace hp_test;assert(ctx.ecx==p0||ctx.ecx==p1);
    // Actual model binding is the live resource boundary; retain the native
    // owner's state/substate selection, not a shared first-person model.
    hypospray[ctx.ecx==p1]=Word(ctx.ecx+0x170)==3&&Word(ctx.ecx+0x174)==1;ctx.esp+=4;
}
X86_FUNC(sub_001FB4E0){ctx.eax=hp_test::animation_complete;ctx.esp+=4;}
X86_FUNC(sub_0022B930){using namespace hp_test;animation_path=Word(Word(ctx.esp+4));ctx.eax=0x730000;ctx.esp+=4;}
X86_FUNC(sub_001FB180){
    using namespace hp_test;assert((Word(ctx.esp+4)==0x730000||Word(ctx.esp+4)==0x730300)&&Word(ctx.esp+8)==0&&Word(ctx.esp+12)==0&&Word(ctx.esp+16)==0xffffffffu);
    assert((Word(ctx.esp+4)==0x730300)==TrialHealthPackPartnerAnimation(base,ctx.ecx-0x17c));
    animation_owner=ctx.ecx-0x17c;++animation_plays;animation_complete=false;ctx.esp+=20;
}
X86_FUNC(sub_00216BD0){assert(hp_test::Word(ctx.esp+8)==0x458a98);++hp_test::injection_sounds;ctx.esp+=16;}
X86_FUNC(sub_00055EC0){++hp_test::self_pulses;ctx.eax=0x730100;ctx.esp+=28;}
X86_FUNC(sub_000490D0){ctx.esp+=4;}
X86_FUNC(sub_000BDAB0){x87_push(ctx,1200);ctx.esp+=4;}
X86_FUNC(sub_0004A840){ctx.eax=hp_test::manager;ctx.esp+=4;}
X86_FUNC(sub_0004ACD0){if(hp_test::stored)StoredPlayerId(ctx,base);else OriginalPlayerId(ctx,base);}
X86_FUNC(sub_00056260){++hp_test::stock_calls;hp_test::Put(ctx.ecx+0xf74,hp_test::Word(ctx.esp+4));ctx.esp+=8;}
X86_FUNC(sub_000562B0){++hp_test::stock_calls;hp_test::Put(ctx.ecx+0xf78,hp_test::Word(ctx.esp+4));ctx.esp+=8;}
X86_FUNC(sub_001F90A0){
    using namespace hp_test;const uint32_t id=Word(ctx.esp+4);X86Context call=ctx;call.esp-=64;
    if(id==101||id==102){++receipts;call.ecx=id==101?p0:p1;if(stored)StoredReceipt(call,base);else OriginalReceipt(call,base);}
    if(id==401){call.ecx=root;if(stored)StoredStock(call,base);else OriginalStock(call,base);if(duplicate){call.ecx=root;StoredStock(call,base);}}
    ctx.esp+=24;
}
// ORIGINAL_HEALTH_PACK_CONTRACTS
void x86_icall(X86Context& c,uint8_t* base,uint32_t target){
    using namespace hp_test;
    assert(base==Bytes());
    if(target==0x123450){c.eax=mode_hud_mask;c.esp+=4;return;}
    if(target==0x4af40){assert(Word(c.esp+4)==1);OriginalHudAllowed(c,base);return;}
    if(target==0x207d70){assert(Word(c.esp+4)==401);c.eax=medic_root_available?root:0;c.esp+=8;return;}
    if(target==0xbdab0){sub_000BDAB0(c,base);return;}
    if(target==0x22b930){
        assert(!std::strcmp(reinterpret_cast<const char*>(Bytes()+Word(Word(c.esp+4))),"assets_coop\\medical\\eqp_hypospray_anim_heal_partner.kfa"));
        c.eax=partner_clip_available?0x730300:0;c.ecx=0x22b930;c.edx=0x22b8d0;c.ebx=0x1122;c.esi=0x3344;c.edi=0x5566;c.ebp=0x7788;c.esp+=4;return;
    }
    if(target==0x216bd0){sub_00216BD0(c,base);return;}
    assert(target==0xbd650&&Word(c.esp+4)==0&&Word(c.esp+8)==0);const uint32_t owner=c.ecx;
    StoredReceipt(c,base);
    // Prove isolation of a permitted callee's clobbers and private stack.
    c.eax=0xdead;c.ecx=0xbeef;c.cf=0;c.sf=1;c.top=4;c.st[4]=7;c.xmm[0].q[0]=123;
    Bytes()[c.esp-80]=0x5a;_mm_setcsr(0x3f80);
    if(throw_call){Bytes()[owner+0x470]=0;throw 7;}
}
namespace kernel {
// ORIGINAL_CALL_GUEST_CONTRACT
}
namespace hp_test {
void Press(unsigned slot){TrialHealthPackInput(slot,false,true);TrialHealthPackInput(slot,true,true);}
void Update(unsigned slot){auto c=Context(slot?p1:p0);TrialHealthPackUpdate(c,Bytes());}
void InjectionQueue(unsigned slot){auto c=Context(slot?p1:p0);c.ebx=c.ecx;OriginalInjectionQueue(c,Bytes());}
void InjectionTick(unsigned slot,bool patched=true){
    auto c=Context(slot?p1:p0);const auto before=c;
    if(patched)PartnerInjectionTick(c,Bytes());else OriginalInjectionTick(c,Bytes());
    assert(c.esp==before.esp+4&&c.esi==before.esi);
}
void StartInjection(unsigned slot){
    InjectionQueue(slot);const uint32_t p=slot?p1:p0;
    assert(Word(p+0x170)==3&&!Bytes()[p+0x470]);
    Update(slot);InjectionTick(slot);assert(Word(p+0x174)==1);
    animation_complete=true;InjectionTick(slot);assert(Word(p+0x174)==2&&hypospray[slot]);
}
void GuestReturnContract(){
    Setup();auto c=Context(p0);c.eax=p0|1;const auto before=c;
    constexpr uint32_t path=0x730400,key=0x730480;
    std::strcpy(reinterpret_cast<char*>(Bytes()+path),"assets_coop\\medical\\eqp_hypospray_anim_heal_partner.kfa");Put(key,path);
    const auto clip=kernel::CallGuest(c,0x22b930,{key});
    assert(clip==0x730300&&!std::memcmp(&c,&before,sizeof c));
    // The failing build read restored EAX as a clip. A readable actor address
    // passes that check, but is neither the returned resource nor its events.
    assert(TrialGuestReadable(c.eax,16)&&c.eax!=clip);
    assert(health_packs::PartnerClip(c,Bytes())==clip&&!std::memcmp(&c,&before,sizeof c));
    partner_clip_available=false;c.eax=clip;
    assert(!health_packs::PartnerClip(c,Bytes())&&c.eax==clip);
    c=Context(behavior);c.eax=0;assert(health_packs::Root(c,Bytes(),behavior)==root&&c.eax==0);
    medic_root_available=false;c.eax=root;assert(!health_packs::Root(c,Bytes(),behavior)&&c.eax==root);
    c=Context(p0);c.eax=0;assert(health_packs::HudAllowed(c,Bytes(),p0)&&c.eax==0);
    Bytes()[0x4e71a8]=0;c.eax=1;assert(!health_packs::HudAllowed(c,Bytes(),p0)&&c.eax==1);
    for(unsigned slot=0;slot<2;++slot){
        Setup();const auto owner=slot?p1:p0;nearby=slot?p0:p1;Press(slot);
        c=Context(owner);c.eax=owner|1;const auto caller=c;TrialHealthPackUpdate(c,Bytes());
        assert(!std::memcmp(&c,&caller,sizeof c)&&health_packs::injections[slot].clip==clip);
        StartInjection(slot);assert(animation_owner==owner&&animation_plays==1);
        Setup();nearby=slot?p0:p1;partner_clip_available=false;Press(slot);
        c=Context(owner);c.eax=clip;TrialHealthPackUpdate(c,Bytes());
        assert(!Bytes()[owner+0x470]&&!health_packs::injections[slot].target&&TrialHealthPackSnapshot(slot).count==1);
    }
    puts("health pack host bridge: exact production CallGuest return/register contract, stale actor EAX, missing lookup and both owner clip delivery passed");
}
void Deliver(bool patched){
    stored=patched;auto c=Context(behavior);
    if(patched){TrialHealthPackMedicScope select(c,Bytes(),true);StoredDelivery(c,Bytes());}
    else OriginalDelivery(c,Bytes());
}
void Pickup(){
    Setup();Deliver(false);assert(Bytes()[p0+0x470]&&Word(world+0xf74)==2&&receipts==1);assert(TrialHealthPackSnapshot(0).count==1);
    Setup();Deliver(true);assert(!Bytes()[p0+0x470]&&!Bytes()[p1+0x470]);assert(Word(world+0xf74)==2&&stock_calls==1);assert(TrialHealthPackSnapshot(1).count==2&&TrialHealthPackSnapshot(0).count==1);
    duplicate=true;Deliver(true);assert(Word(world+0xf74)==1&&stock_calls==2&&TrialHealthPackSnapshot(1).count==3);
    // Full P2 inventory routes a new collection to P1, then blocks both at cap.
    duplicate=false;Deliver(true);assert(Word(world+0xf74)==0&&TrialHealthPackSnapshot(0).count==2);
    Deliver(true);assert(Word(world+0xf74)==0&&stock_calls==3);
    Setup(false);Put(medic+0x15c,4);Deliver(true);assert(Word(world+0xf78)==2&&Word(world+0xf74)==3&&TrialHealthPackSnapshot(0).count==2);
    Setup(false);Put(root,0x46ffff);Deliver(true);assert(Bytes()[p0+0x470]&&TrialHealthPackSnapshot(0).count==1); // mission-specific root untouched
}
void Eligibility(){
    Setup();auto c=Context(behavior);TrialHealthPackMedicScope scope(c,Bytes(),true);
    c.top=0;c.st[0]=100;c.esp=stack;OriginalEligibility(c,Bytes());assert(!c.eax);
    c.top=0;c.st[0]=100;c.esp=stack;StoredEligibility(c,Bytes());assert(c.eax);
    health_packs::players[1].count=3;c.top=0;c.st[0]=10;c.esp=stack;StoredEligibility(c,Bytes());assert(!c.eax);
}
void Use(){
    Setup();Press(0);Update(0);assert(TrialHealthPackSnapshot(0).count==1&&!Bytes()[p0+0x470]);
    Float(p0+0x9c,600);Press(0);auto c=Context(p0);const auto before=c;std::array<uint8_t,256> scratch{};std::memcpy(scratch.data(),Bytes()+stack-256,256);
    const unsigned oldcsr=_mm_getcsr();_mm_setcsr(0x5f80);TrialHealthPackUpdate(c,Bytes());assert(_mm_getcsr()==0x5f80);_mm_setcsr(oldcsr);
    assert(!std::memcmp(&c,&before,sizeof c)&&!std::memcmp(scratch.data(),Bytes()+stack-256,256));assert(TrialHealthPackSnapshot(0).count==0&&TrialHealthPackSnapshot(1).count==1&&Bytes()[p0+0x470]);
    // Holding, disallowed input, full health, dead state, existing healing and
    // old requests crossing actor creation cannot spend another player's pack.
    Setup();Float(p0+0x9c,100);TrialHealthPackInput(0,true,false);TrialHealthPackInput(0,true,true);Update(0);assert(TrialHealthPackSnapshot(0).count==1);
    Press(0);auto n=Context(p0);n.eax=p0;TrialHealthPackCreated(n,Bytes());Update(0);assert(TrialHealthPackSnapshot(0).count==1);
    for(unsigned state:{3u,11u,12u,13u}){Setup();Float(p0+0x9c,100);Put(p0+0x170,state);Press(0);Update(0);assert(TrialHealthPackSnapshot(0).count==1);}
    Setup();Float(p0+0x9c,100);Float(p0+0x46c,50);Press(0);Update(0);assert(TrialHealthPackSnapshot(0).count==1);
    Setup();Float(p0+0x9c,100);presentation=2;Press(0);Update(0);presentation=1;Update(0);assert(TrialHealthPackSnapshot(0).count==1&&!Bytes()[p0+0x470]);
    Setup();nearby=p1;Press(0);Update(0);assert(!revives&&TrialHealthPackSnapshot(0).count==1&&Bytes()[p0+0x470]);
    Setup();Float(p0+0x9c,100);Put(p0+0x54,999);Press(0);Update(0);assert(!Bytes()[p0+0x470]);
    Setup();Float(p0+0x9c,100);throw_call=true;Press(0);c=Context(p0);const auto saved=c;try{TrialHealthPackUpdate(c,Bytes());assert(false);}catch(int){}assert(!std::memcmp(&c,&saved,sizeof c)&&TrialHealthPackSnapshot(0).count==1);
}
void PartnerInjection(){
    // Execute the native aiming branch as a sign/row witness, including a
    // rotated/scaled eye frame. Behind/sideways requests never become self use.
    for(unsigned slot=0;slot<2;++slot){
        Setup();const uint32_t healer=slot?p1:p0;nearby=slot?p0:p1;
        Float(healer+0x6d0,2);Float(healer+0x6d8,0);Float(healer+0x6e0,40);
        auto c=Context(healer);OriginalAimRay(c,Bytes());
        assert(health_packs::Real(Bytes(),healer+0x454)==15040&&health_packs::Real(Bytes(),healer+0x45c)==0);
        downed_position[0]=140;downed_position[2]=0;Update(slot);assert(TrialHealthPackSnapshot(slot).can_revive);
        const unsigned oldcsr=_mm_getcsr();_mm_setcsr(0x5f80);health_packs::Publish(Bytes(),slot);assert(_mm_getcsr()==0x5f80);_mm_setcsr(oldcsr);
        for(unsigned reason=0;reason<5;++reason){
            downed_position[0]=reason==0?0.f:reason==1?40.f:140.f;
            downed_position_valid=reason!=2;
            Float(healer+0x6d0,reason==3?0.f:reason==4?NAN:2.f);
            Float(healer+0x9c,600);Press(slot);Update(slot);
            assert(!Bytes()[healer+0x470]&&!health_packs::injections[slot].target&&TrialHealthPackSnapshot(slot).count==1&&!TrialHealthPackSnapshot(slot).can_revive);
        }
    }
    for(unsigned slot=0;slot<2;++slot){
        const uint32_t healer=slot?p1:p0,partner=slot?p0:p1;
        // Negative witness: the native contact refills the actor playing it.
        Setup();Put(healer+0x170,3);Put(healer+0x174,2);Bytes()[healer+0x1a2]=1;
        InjectionTick(slot,false);assert(health_packs::Real(Bytes(),healer+0x46c)==1200&&self_pulses==1&&!revives);
        Setup();nearby=partner;revive_success=true;Float(healer+0x9c,600);Press(slot);Update(slot);
        assert(!revives&&TrialHealthPackSnapshot(slot).count==1);
        StartInjection(slot);assert(animation_owner==healer&&animation_plays==1&&!hypospray[1-slot]);
        assert(TrialHealthPackPartnerAnimation(Bytes(),healer)&&!TrialHealthPackPartnerAnimation(Bytes(),partner));
        Press(slot);Update(slot);assert(!revives&&TrialHealthPackSnapshot(slot).count==1); // no second queued use
        Bytes()[healer+0x1a2]=1;InjectionTick(slot);
        assert(revives==1&&TrialHealthPackSnapshot(slot).count==0&&TrialHealthPackSnapshot(1-slot).count==1);
        assert(!Word(healer+0x46c)&&health_packs::Real(Bytes(),healer+0x9c)==600&&!self_pulses&&injection_sounds==1);
        InjectionTick(slot);assert(revives==1&&injection_sounds==1); // repeated event is harmless
        Bytes()[healer+0x1a2]=0;animation_complete=true;InjectionTick(slot);
        assert(Word(healer+0x170)==8&&Word(healer+0x174)==2&&!hypospray[slot]&&raises==1);
        Update(slot);assert(!TrialHealthPackPartnerAnimation(Bytes(),healer)&&!health_packs::injections[slot].target);
    }
    // Contact is transactional: failure, movement, identity loss, hidden HUD,
    // destruction, checkpoint cancellation or zero stock cannot heal either actor.
    for(unsigned reason=0;reason<9;++reason){
        Setup();nearby=p1;revive_success=true;Press(0);Update(0);StartInjection(0);
        if(reason==0)revive_success=false;
        if(reason==1)nearby=0;
        if(reason==2)Put(p1+0x54,999);
        if(reason==3)presentation=2;
        if(reason==4){auto c=Context(p1);TrialHealthPackDestroyed(c,Bytes());}
        if(reason==5)health_packs::injections[0].cancelled=true;
        if(reason==6)health_packs::players[0].count=0;
        if(reason==7){nearby=0;Update(0);nearby=p1;} // cancellation cannot resume when returning
        if(reason==8)downed_position[2]=-100; // turning away before contact cancels without spending
        Bytes()[p0+0x1a2]=1;InjectionTick(0);InjectionTick(0);
        assert(revives==(reason==0?1u:0u)&&!Word(p0+0x46c)&&!self_pulses&&!injection_sounds);
        assert(health_packs::players[0].count==(reason==6?0u:1u));
    }
    Setup();nearby=p1;Press(0);Update(0);nearby=0;Update(0);
    assert(!Bytes()[p0+0x470]&&!health_packs::injections[0].target&&TrialHealthPackSnapshot(0).count==1);
    // A leftover native event during the weapon-lowering phase cannot commit
    // the new injection or accidentally refill the healer.
    Setup();nearby=p1;Press(0);Update(0);InjectionQueue(0);Update(0);Bytes()[p0+0x1a2]=1;InjectionTick(0);
    assert(!revives&&!Word(p0+0x46c)&&!health_packs::injections[0].contact&&TrialHealthPackSnapshot(0).count==1);
    Setup();nearby=p1;partner_clip_available=false;Press(0);Update(0);
    assert(!Bytes()[p0+0x470]&&!health_packs::injections[0].target&&TrialHealthPackSnapshot(0).count==1);
    Setup();nearby=p1;Press(0);Update(0);StartInjection(0);Put(p0+0x170,12);Update(0);
    assert(!health_packs::injections[0].target&&TrialHealthPackSnapshot(0).count==1&&!revives);
    Setup();nearby=p1;Press(0);Update(0);StartInjection(0);auto c=Context(p0);c.eax=p0;TrialHealthPackCreated(c,Bytes());
    assert(!health_packs::injections[0].target&&!health_packs::injections[1].target);
    // Normal self use still runs the exact original injection resource/contact.
    Setup(false);Float(p0+0x9c,600);Press(0);Update(0);StartInjection(0);
    assert(animation_path==0x458a48&&!TrialHealthPackPartnerAnimation(Bytes(),p0));
    Bytes()[p0+0x1a2]=1;InjectionTick(0);assert(Word(p0+0x46c)&&self_pulses==1&&injection_sounds==1&&!revives);
    puts("partner injection: original self-heal failure witness, both owners, native lowering/contact/raising, cancellation, one-time stock and self-heal isolation passed (offline; authored clip playback requires live QA)");
}
void RevivePrompt(){
    for(unsigned slot=0;slot<2;++slot){
        const uint32_t owner=slot?p1:p0,partner=slot?p0:p1;
        for(unsigned reason=0;reason<11;++reason){
            Setup();nearby=partner;Update(slot);assert(TrialHealthPackSnapshot(slot).can_revive);
            if(reason==0){downed_position[0]=150;downed_position[2]=50;}
            if(reason==1)downed_position[2]=-100;
            if(reason==2)health_packs::players[slot].count=0;
            if(reason==3)Float(owner+0x9c,0);
            if(reason==4)Put(owner+0x170,3);
            if(reason==5)Float(owner+0x46c,10);
            if(reason==6)Bytes()[owner+0x470]=1;
            if(reason==7)nearby=0;
            if(reason==8)Put(partner+0x54,0xffffffffu);
            if(reason==9)downed_position_valid=false;
            if(reason==10)presentation=2;
            Update(slot);assert(!TrialHealthPackSnapshot(slot).can_revive);
            assert(health_packs::players[slot].count==(reason==2?0u:1u)&&!revives);
        }
        Setup();nearby=partner;Float(owner+0x6d8,2);downed_position[0]=40;
        Update(slot);assert(TrialHealthPackSnapshot(slot).can_revive); // normalized aiming, near torso
        downed_position[0]=100;Update(slot);assert(!TrialHealthPackSnapshot(slot).can_revive);
        // Looking past the body hides the prompt without changing accepted front-side revive mechanics.
        assert(health_packs::PartnerInFront(Bytes(),owner,partner));
    }
    puts("revive prompt: both owners, aimed downed partner, stock/state/HUD gates and non-unit aim passed");
}
void HealthBarExtent(){
    for(unsigned slot=0;slot<2;++slot){
        const uint32_t owner=slot?p1:p0,node=0x638100+slot*0x200;
        for(float extent:{120.f,140.f,160.f,180.f}){
            Setup();Float(node+0xa8,extent);Update(slot);const auto status=TrialHealthPackSnapshot(slot);
            assert(status.visible&&status.bar_extent==extent&&status.count==1);
            assert(!TrialHealthPackSnapshot(1-slot).visible);
        }
        for(unsigned reason=0;reason<12;++reason){
            Setup();Update(slot);assert(TrialHealthPackSnapshot(slot).visible);
            if(reason<6)Put(node+reason*0x20,0);
            if(reason==6)Put(node+0x84,slot?p0:p1);
            if(reason==7)Float(node+0x88,2);
            if(reason==8)Float(node+0xa8,NAN);
            if(reason==9)Float(node+0xa8,19);
            if(reason==10)Float(node+0xa8,2581);
            if(reason==11)Put(Word(owner+0x300)+8,0xffffffffu);
            Update(slot);const auto status=TrialHealthPackSnapshot(slot);
            assert(!status.visible&&status.bar_extent==0&&status.count==1);
        }
        Setup();Update(slot);auto c=Context(owner);TrialHealthPackDestroyed(c,Bytes());
        assert(!TrialHealthPackSnapshot(slot).visible&&!TrialHealthPackSnapshot(slot).bar_extent);
    }
    puts("health bar extent: original HUD tree, four upgrade widths, owner/shape validation and lifecycle clearing passed");
}
void HudVisibility(){
    // SP scripted mode masks, global/world hiding and HUD fade are native
    // admission gates; a blocked key edge must not execute when the HUD returns.
    for(unsigned reason=0;reason<8;++reason){
        Setup(false);Update(0);assert(TrialHealthPackSnapshot(0).visible);
        const uint32_t hud=Word(p0+0x300);Float(p0+0x9c,600);
        if(reason==0)Put(world+0x10,0x800);
        if(reason==1)Bytes()[0x4e71a8]=0;
        if(reason==2)Put(manager+0x14,0);
        if(reason==3){Put(manager+0x824,0x639000);Put(0x639000,0x639100);Put(0x639114,0x123450);mode_hud_mask=0;}
        if(reason==4)Float(hud+0x14,0);
        if(reason==5)Put(hud+4,p1);
        if(reason==6)presentation=2;
        if(reason==7)Put(0x44bf54,0x4af20);
        Press(0);auto c=Context(p0);const auto before=c;TrialHealthPackUpdate(c,Bytes());
        assert(!std::memcmp(&c,&before,sizeof c));
        assert(!TrialHealthPackSnapshot(0).visible&&TrialHealthPackSnapshot(0).count==1&&!Bytes()[p0+0x470]);
        Put(world+0x10,0);Bytes()[0x4e71a8]=1;Put(manager+0x14,0xffffffff);mode_hud_mask=1;Float(hud+0x14,1);Put(hud+4,p0);Put(0x44bf54,0x4af40);presentation=0;
        Update(0);assert(TrialHealthPackSnapshot(0).visible&&TrialHealthPackSnapshot(0).count==1&&!Bytes()[p0+0x470]);
        Press(0);Update(0);assert(TrialHealthPackSnapshot(0).count==0&&Bytes()[p0+0x470]);
    }
}
void Checkpoints(){
    Setup();
    constexpr uint32_t settings=0x640000,save=0x641000,snapshot=save+4;
    Put(0x5096e0,settings);Put(settings+0x30,save);Put(save,5);Put(snapshot,7);
    Put(0x5152b0,3);std::memcpy(Bytes()+0x5152b4,"Player One",11);
    const auto directory=std::filesystem::temp_directory_path()/
        ("ucrr-health-pack-contract-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    assert(!std::filesystem::exists(directory));TrialHealthPackStorage(directory);
    health_packs::players[0].count=0;health_packs::players[1].count=2;
    auto save_context=Context(p0);save_context.esi=snapshot;
    Put(save_context.esp+0x14,manager);Put(manager+0x0c,0x642000);
    StoredCheckpointTail(save_context,Bytes());
    assert(save_context.edi==snapshot+health_packs::checkpoint_size);
    assert(std::filesystem::is_directory(directory));health_packs::checkpoints.clear();
    auto load=Context(p0);load.ebx=p0;
    health_packs::players[0].count=3;StoredCheckpointRestore(load,Bytes());
    assert(TrialHealthPackSnapshot(0).count==0);load.ebx=p1;
    health_packs::players[1].count=0;StoredCheckpointRestore(load,Bytes());assert(TrialHealthPackSnapshot(1).count==2);
    assert(!health_packs::restored[1].actor);
    // A direct restore must not leave a constructor record for a later new
    // level that happens to reuse the exact actor address and identity.
    auto fresh=Context(p1);fresh.eax=p1;TrialHealthPackCreated(fresh,Bytes());assert(TrialHealthPackSnapshot(1).count==1);
    // Construction happens after original checkpoint restoration. Apply once,
    // including same-address reuse; ordinary subsequent updates never refill.
    health_packs::players[1].actor=0;TrialHealthPackCheckpointLoaded(load,Bytes());
    auto create=Context(p1);create.eax=p1;TrialHealthPackCreated(create,Bytes());assert(TrialHealthPackSnapshot(1).count==2);
    health_packs::players[1].count=0;Update(1);assert(TrialHealthPackSnapshot(1).count==0);
    // Different profiles, mission entries, checkpoint bytes and truncated
    // companions cannot restore packs from another save or mint a start pack.
    for(uint32_t address:{0x5152b4u,world+0x2c,snapshot+0x174}){
        const auto original=Word(address);Put(address,original^1);TrialHealthPackCheckpointLoaded(load,Bytes());assert(TrialHealthPackSnapshot(1).count==0);Put(address,original);
    }
    health_packs::CheckpointKey key{};assert(health_packs::Key(Bytes(),snapshot,key));
    auto dirty=key;dirty.fill(0xaa);assert(health_packs::Key(Bytes(),snapshot,dirty)&&key==dirty);
    {std::ofstream file(health_packs::File(key),std::ios::binary|std::ios::trunc);file<<"bad";}
    TrialHealthPackCheckpointLoaded(load,Bytes());assert(TrialHealthPackSnapshot(1).count==0);
    TrialHealthPackStorage({});
    // Only remove the files created in this unique fixture directory.
    for(const auto& entry:std::filesystem::directory_iterator(directory)){assert(entry.is_regular_file());std::filesystem::remove(entry.path());}
    std::filesystem::remove(directory);
}
}
int main(){hp_test::GuestReturnContract();hp_test::Pickup();hp_test::Eligibility();hp_test::Use();hp_test::PartnerInjection();hp_test::RevivePrompt();hp_test::HealthBarExtent();hp_test::HudVisibility();hp_test::Checkpoints();std::puts("health packs: retail delivery/stock/eligibility/HUD admission, isolated input, healing, revive and checkpoint contracts passed");}

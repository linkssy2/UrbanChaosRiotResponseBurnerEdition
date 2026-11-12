#include "xbe_coop_orders.h"
#include "xbe_player_actions.h"
#include "xbe_coop.h"
#include "xbe_coop_guest.h"
#include "xbe_health_packs.h"
#include "cpu/x86_ops.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <array>
#include <cmath>
#include <cstring>
#include <xmmintrin.h>

namespace {
thread_local uint32_t selecting=0,hud_owner=0,help_owner=0;
thread_local bool copying=false;
uint32_t Word(const uint8_t* b,uint32_t a){uint32_t v;std::memcpy(&v,b+a,4);return v;}
void Put(uint8_t* b,uint32_t a,uint32_t v){std::memcpy(b+a,&v,4);}
uint32_t Entity(const uint8_t* b,uint32_t id){
    if(id==0xffffffffu||!TrialGuestReadable(0x51c4dc,4))return 0;
    const uint32_t world=Word(b,0x51c4dc);
    if(!TrialGuestReadable(world,8))return 0;
    const uint32_t state=Word(b,world+4);
    if(!TrialGuestReadable(state,0x203c))return 0;
    const uint32_t entry=state+0x3c+(id&1023)*8,p=Word(b,entry+4);
    return Word(b,entry)==id&&TrialGuestReadable(p,0x60)&&Word(b,p+0x54)==id&&!(Word(b,p+0x5c)&1)?p:0;
}
uint32_t Manager(const uint8_t* b,unsigned slot){
    if(!b||!trial_coop::Active()||slot>1)return 0;
    const uint32_t p=TrialCoopPlayer(slot);
    if(!TrialGuestReadable(p,0x7f0)||Word(b,p)!=0x46fbe8||Word(b,p+0x368)!=slot||Entity(b,Word(b,p+0x54))!=p)return 0;
    const uint32_t manager=Entity(b,Word(b,p+0x7ec));
    return TrialGuestReadable(manager,0x10c)&&Word(b,manager)==0x46caa8&&Word(b,manager+0x58)==Word(b,p+0x54)&&Word(b,manager+0x98)==slot?manager:0;
}
int Slot(const uint8_t* b,uint32_t manager){
    if(!manager)return -1;
    for(unsigned i=0;i<2;++i)if(Manager(b,i)==manager)return int(i);
    return -1;
}
uint32_t Other(const uint8_t* b,uint32_t manager){const int slot=Slot(b,manager);return slot<0?0:Manager(b,unsigned(slot)^1);}
uint32_t QueryManager(const uint8_t* b,uint32_t client){
    if(!b||!TrialGuestReadable(client,0xa4)||Entity(b,Word(b,client+0x54))!=client)return 0;
    if(trial_coop::Active())return Slot(b,selecting)>=0?selecting:0;
    const uint32_t manager=Entity(b,Word(b,client+0x98));
    if(!TrialGuestReadable(manager,0x10c)||Word(b,manager)!=0x46caa8)return 0;
    const uint32_t actor=Entity(b,Word(b,manager+0x58));
    return TrialGuestReadable(actor,0x7f0)&&Word(b,actor)==0x46fbe8&&
        Word(b,actor+0x7ec)==Word(b,manager+0x54)?manager:0;
}
uint32_t AimedMedic(const uint8_t* b,uint32_t manager,uint32_t owner,bool offer=false){
    struct Restore {unsigned csr=_mm_getcsr();~Restore(){_mm_setcsr(csr);}} restore;
    if(!TrialGuestReadable(manager,0x10c)||!TrialGuestReadable(owner,0x390))return 0;
    // Team medics offering before joining SELECTABLEITEMS have no orders hit;
    // C1020's independent, occlusion-tested friendly reticle hit still identifies them.
    const uint32_t target=Entity(b,Word(b,manager+0xe8)==4?Word(b,manager+0xec):
        offer&&Word(b,owner+0x38c)==2?Word(b,owner+0x388):0xffffffffu);
    if(!TrialGuestReadable(target,0x4c))return 0;
    // The manager's native look ray already checked facing and occlusion.
    // Keep the original dialogue's 300-unit reach for each viewer separately.
    double distance=0;
    for(unsigned i=0;i<3;++i){
        float a,z;std::memcpy(&a,b+owner+0x40+4*i,4);std::memcpy(&z,b+target+0x40+4*i,4);
        if(!std::isfinite(a)||!std::isfinite(z))return 0;
        const double d=double(a)-z;distance+=d*d;
    }
    return distance<=300.0*300.0?target:0;
}
bool Stack(const X86Context& c){return c.esp>=0x11000&&TrialGuestReadable(c.esp-4096,4096)&&TrialGuestReadable(c.esp,0x38);}
struct CopyCall {
    X86Context c;uint8_t* b;uint32_t low,sp;std::array<uint8_t,4096> saved;
    unsigned csr=_mm_getcsr();bool previous=copying,active=false;
    CopyCall(const X86Context& context,uint8_t* base):c(context),b(base),low(c.esp-4096),sp(c.esp-512){}
    ~CopyCall(){if(active){copying=previous;std::memcpy(b+low,saved.data(),saved.size());_mm_setcsr(csr);}}
    void Run(uint32_t fn,uint32_t self,std::initializer_list<uint32_t> args){
        // Matching orders lists make no guest calls and need no stack snapshot.
        if(!active){std::memcpy(saved.data(),b+low,saved.size());copying=true;active=true;}
        c.ecx=self;c.esp=sp;kernel::CallGuest(c,fn,args);
    }
};
bool ClientIn(const uint8_t* b,uint32_t manager,uint32_t id){
    const uint32_t head=Word(b,manager+0xac);if(!TrialGuestReadable(head,8))return false;
    for(uint32_t node=Word(b,head),n=0;node!=head&&n<1024;++n,node=Word(b,node)){
        if(!TrialGuestReadable(node,0x1c))return false;
        if(Word(b,node+8)==id)return true;
    }
    return false;
}
void Synchronize(const X86Context& c,uint8_t* b,uint32_t manager){
    const uint32_t other=Other(b,manager);
    if(copying||!other||!Stack(c))return;
    const uint32_t head=Word(b,other+0xac);if(!TrialGuestReadable(head,8))return;
    CopyCall call(c,b);
    for(uint32_t node=Word(b,head),n=0;node!=head&&n<1024;++n,node=Word(b,node)){
        if(!TrialGuestReadable(node,0x1c))return;
        const uint32_t client=Word(b,node+8),first=Word(b,node+0x10),end=Word(b,node+0x14);
        if(!Entity(b,client)||ClientIn(b,manager,client))continue;
        if(end<first||(end-first)%0x1c||(end-first)>8192*0x1c||(end>first&&!TrialGuestReadable(first,end-first)))continue;
        call.Run(0x159150,manager,{client});
        for(uint32_t action=first;action<end;action+=0x1c){
            const uint32_t payload=call.sp+128,sender=call.sp+160;
            Put(b,sender,client);
            Put(b,payload,Word(b,action+4));Put(b,payload+4,Word(b,action+8));
            Put(b,payload+8,Word(b,action+0xc));Put(b,payload+0xc,Word(b,action+0x10));
            Put(b,payload+0x10,b[action+0x18]);
            call.Run(0x158f30,manager,{payload,sender});
        }
    }
}
}

TrialCoopOrdersScope::TrialCoopOrdersScope(const X86Context& c,uint8_t* b):previous(selecting){
    selecting=Slot(b,c.ecx)<0?0:c.ecx;
    try{if(selecting)Synchronize(c,b,selecting);}
    catch(...){selecting=previous;throw;}
}
TrialCoopOrdersScope::~TrialCoopOrdersScope(){selecting=previous;}
TrialCoopOrdersHudScope::TrialCoopOrdersHudScope(const X86Context& c,const uint8_t* b):previous(hud_owner){
    hud_owner=0;
    if(!b||!trial_coop::Active()||!TrialGuestReadable(c.ecx,0x18)||Word(b,c.ecx)!=0x4531a0)return;
    const uint32_t actor=Word(b,c.ecx+4);
    for(unsigned slot=0;slot<2;++slot)if(actor==TrialCoopPlayer(slot)&&TrialGuestReadable(actor,0x304)&&Word(b,actor+0x300)==c.ecx){hud_owner=Manager(b,slot);if(!hud_owner)hud_owner=0xffffffffu;break;}
}
TrialCoopOrdersHudScope::~TrialCoopOrdersHudScope(){hud_owner=previous;}
uint32_t TrialCoopOrdersHud(const uint8_t* b,uint32_t original){return !hud_owner?original:Slot(b,hud_owner)<0?0:hud_owner+0xb4;}
static uint32_t HelpHudForOwner(const uint8_t* b,uint32_t actor){
    if(!b||!trial_coop::Active()||!TrialGuestReadable(actor,0x7f4)||Word(b,actor)!=0x46fbe8||
       Entity(b,Word(b,actor+0x54))!=actor)return 0;
    const uint32_t slot=Word(b,actor+0x368),root=Word(b,actor+0x300);
    if(slot>1||TrialCoopPlayer(slot)!=actor||!TrialGuestReadable(root,0x18)||
       Word(b,root)!=0x4531a0||Word(b,root+4)!=actor)return 0;
    // 70D90 owns a fresh help list in the main HUD's six-child aggregate.
    // 77500 also publishes a singleton, which otherwise points at P2's list.
    uint32_t node=Word(b,root+8);
    if(!TrialGuestReadable(node,12)||Word(b,node)!=0x453bd0)return 0;
    node=Word(b,node+8);
    if(!TrialGuestReadable(node,0x1c)||Word(b,node)!=0x453c20)return 0;
    node=Word(b,node+4);
    return TrialGuestReadable(node,0x14)&&Word(b,node)==0x453580?node:0;
}
TrialCoopHelpScope::TrialCoopHelpScope(const X86Context& c,const uint8_t* b):previous(help_owner),previous_selecting(selecting){
    help_owner=0;
    if(!b||!trial_coop::Active()||!TrialGuestReadable(c.ecx,0xc0)||Word(b,c.ecx)!=0x46b4d0)return;
    // Update and event publication retain the helper's native parent actor.
    // A stale helper must never clear or publish into the partner's list.
    help_owner=0xffffffffu;selecting=0;
    const uint32_t actor=Entity(b,Word(b,c.ecx+0x58));
    if(!TrialGuestReadable(actor,0x7f4)||Word(b,actor)!=0x46fbe8||
       Entity(b,Word(b,c.ecx+0x54))!=c.ecx||Word(b,actor+0x7f0)!=Word(b,c.ecx+0x54))return;
    const uint32_t slot=Word(b,actor+0x368),manager=Manager(b,slot);
    if(!manager||TrialCoopPlayer(slot)!=actor||!HelpHudForOwner(b,actor))return;
    help_owner=actor;selecting=manager;
}
TrialCoopHelpScope::~TrialCoopHelpScope(){help_owner=previous;selecting=previous_selecting;}
uint32_t TrialCoopHelpHud(const uint8_t* b,uint32_t original){return help_owner?HelpHudForOwner(b,help_owner):original;}
void TrialCoopOrdersProperty(X86Context& c,const uint8_t* b){
    // Clients retain their command owner. Only synchronous eligibility queries
    // use the manager currently checking its own camera and input.
    if(Slot(b,selecting)>=0&&Slot(b,c.ecx)>=0)c.ecx=selecting;
}
void TrialCoopOrdersControl(X86Context& c,const uint8_t* b){if(Slot(b,c.esi)>=0)c.eax=Word(b,c.esi+0x58);}
void TrialCoopOrdersPlayer(X86Context& c,const uint8_t* b){if(Slot(b,selecting)>=0)c.eax=Word(b,selecting+0x58);}
bool TrialCoopOrdersRegister(X86Context& c,uint8_t* b){
    const uint32_t manager=c.ebp,other=Other(b,manager);
    if(!other||!Stack(c))return false;
    if(copying)return true;
    const uint32_t client_id=Word(b,c.esp+0x34),client=Entity(b,client_id);
    if(!TrialGuestReadable(client,0xa4))return false;
    if(!ClientIn(b,other,client_id)){
        CopyCall call(c,b);call.Run(0x159150,other,{client_id});
    }
    // The second registration owns an independent HUD list, not another NPC
    // initializer. Medic stock and authored command initialization run once.
    return Slot(b,Entity(b,Word(b,client+0x98)))>=0;
}
void TrialCoopOrdersEvent(X86Context& c,uint8_t* b,uint32_t callback){
    if(copying)return;
    const uint32_t other=Other(b,c.ecx);
    if(!other||!Stack(c))return;
    const uint32_t sender=Word(b,c.esp+8),payload=Word(b,c.esp+4);
    if(!TrialGuestReadable(sender,4))return;
    CopyCall call(c,b);call.Run(callback,other,{payload,sender});
}
bool TrialCoopMedicPrompt(const X86Context& c,const uint8_t* b,bool offer,bool decline){
    const uint32_t manager=QueryManager(b,c.esi);
    if(!manager)return false;
    const uint32_t owner=Entity(b,Word(b,manager+0x58)),target=AimedMedic(b,manager,owner,offer);
    if(!target)return false;
    return decline?TrialHealthPackMedicOfferOwner(b,owner,c.esi,target):
        TrialHealthPackMedicPrompt(b,owner,c.esi,target,offer);
}
bool TrialCoopOrdersChoose(X86Context& c,uint8_t* b){
    if(!b||!TrialGuestReadable(c.esp,12)||!TrialGuestReadable(c.ecx,0xa4))return true;
    const uint32_t kind=Word(b,c.ecx),payload=Word(b,c.esp+4);
    const bool medic=kind==0x46e960||kind==0x4727a0;
    const uint32_t sender=Word(b,c.esp+8);
    if(!TrialGuestReadable(sender,4)||!TrialGuestReadable(payload,4))return !medic;
    const uint32_t manager=Entity(b,Word(b,sender));
    const bool valid_manager=trial_coop::Active()?Slot(b,manager)>=0:
        TrialGuestReadable(manager,0x10c)&&Word(b,manager)==0x46caa8;
    if(!valid_manager||Entity(b,Word(b,c.ecx+0x54))!=c.ecx||!ClientIn(b,manager,Word(b,c.ecx+0x54)))return !medic;
    const uint32_t command=Word(b,payload),owner=Entity(b,Word(b,manager+0x58));
    if(kind==0x46e960&&command==4){
        const uint32_t target=AimedMedic(b,manager,owner);
        if(!target||!TrialHealthPackMedicClaim(b,owner,c.ecx,target))return false;
    }
    if(kind==0x4727a0&&(command==5||command==6)){
        if(!TrialHealthPackMedicOfferOwner(b,owner,c.ecx))return false;
        if(command==5){const uint32_t target=AimedMedic(b,manager,owner,true);
            if(!target||!TrialHealthPackMedicPrompt(b,owner,c.ecx,target,true))return false;}
    }
    // An actual selected command, unlike a look query, has one initiator;
    // preserve that native owner for asynchronous completion and feedback.
    if(trial_coop::Active()||medic)Put(b,c.ecx+0x98,Word(b,manager+0x54));
    return true;
}

bool TrialPlayerHudVisible(const X86Context& c,const uint8_t* b){
    if(!b||!TrialGuestReadable(c.ecx,8)||Word(b,c.ecx)!=0x4531a0)return true;
    const uint32_t actor=Word(b,c.ecx+4);
    return !TrialGuestReadable(actor,0x36c)||Word(b,actor)!=0x46fbe8||
        trial_player_actions::HudVisible(Word(b,actor+0x368));
}

TrialHiddenHudScope::TrialHiddenHudScope(const X86Context& c,uint8_t* b):base(b){
    if(TrialPlayerHudVisible(c,b))return;
    draw=false;
    if(!TrialGuestReadable(c.ecx,0x18))return;
    uint32_t node=Word(b,c.ecx+8);
    if(!TrialGuestReadable(node,12)||Word(b,node)!=0x453bd0)return;
    node=Word(b,node+8);
    if(!TrialGuestReadable(node,0x1c)||Word(b,node)!=0x453c20)return;
    const uint32_t scope=Word(b,node+0x10);
    if(!TrialGuestReadable(scope,12)||Word(b,scope)!=0x4530bc||
       Word(b,scope+4)!=Word(b,c.ecx+4))return;
    // Keep the native scope draw and restore the full tree before updates resume.
    root=c.ecx;draw=true;
    for(unsigned i=0;i<3;++i)saved[i]=Word(b,root+8+4*i);
    Put(b,root+8,scope);Put(b,root+12,0);Put(b,root+16,0);
}
TrialHiddenHudScope::~TrialHiddenHudScope(){
    if(root)for(unsigned i=0;i<3;++i)Put(base,root+8+4*i,saved[i]);
}

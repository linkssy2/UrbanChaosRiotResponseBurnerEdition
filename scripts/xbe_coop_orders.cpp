#include "xbe_coop_orders.h"
#include "xbe_coop.h"
#include "xbe_coop_guest.h"
#include "cpu/x86_ops.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <array>
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
bool Stack(const X86Context& c){return c.esp>=0x11000&&TrialGuestReadable(c.esp-4096,4096)&&TrialGuestReadable(c.esp,0x38);}
struct CopyCall {
    X86Context c;uint8_t* b;uint32_t low,sp;std::array<uint8_t,4096> saved;
    unsigned csr=_mm_getcsr();bool previous=copying;
    CopyCall(const X86Context& context,uint8_t* base):c(context),b(base),low(c.esp-4096),sp(c.esp-512){std::memcpy(saved.data(),b+low,saved.size());copying=true;}
    ~CopyCall(){copying=previous;std::memcpy(b+low,saved.data(),saved.size());_mm_setcsr(csr);}
    void Run(uint32_t fn,uint32_t self,std::initializer_list<uint32_t> args){c.ecx=self;c.esp=sp;kernel::CallGuest(c,fn,args);}
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
void TrialCoopOrdersChoose(X86Context& c,uint8_t* b){
    if(!b||!trial_coop::Active()||!TrialGuestReadable(c.esp,12)||!TrialGuestReadable(c.ecx,0xa4))return;
    const uint32_t sender=Word(b,c.esp+8);
    if(!TrialGuestReadable(sender,4))return;
    const uint32_t manager=Entity(b,Word(b,sender));
    if(Slot(b,manager)<0||Entity(b,Word(b,c.ecx+0x54))!=c.ecx||!ClientIn(b,manager,Word(b,c.ecx+0x54)))return;
    // An actual selected command, unlike a look query, has one initiator;
    // preserve that native owner for asynchronous completion and feedback.
    Put(b,c.ecx+0x98,Word(b,manager+0x54));
}

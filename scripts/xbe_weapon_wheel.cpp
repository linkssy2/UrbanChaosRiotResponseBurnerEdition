#include "xbe_weapon_wheel.h"
#include "xbe_coop.h"
#include "xbe_coop_guest.h"
#include "cpu/x86_ops.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <mutex>
#include <xmmintrin.h>

namespace trial_weapon_wheel {
namespace {
using Clock=std::chrono::steady_clock;
struct Identity {uint32_t owner=0,id=0,world=0,level=0;unsigned slot=2;};
bool Same(const Identity& a,const Identity& b){return a.owner==b.owner&&a.id==b.id&&a.world==b.world&&a.level==b.level&&a.slot==b.slot;}
struct Command {Identity identity;uint32_t slot=0,definition=0,current=0;bool pending=false;Kind kind=Kind::Guns;};
struct Player {
    Snapshot view;Identity identity;Command command;
    uint32_t current_slot=0xffffffffu;
    uint64_t revision=0;
    Clock::time_point updated{};
    bool down=false,allowed=false,keyboard=false,fresh_release=true,single_player=false;
    unsigned release_publications=0;
};
std::mutex mutex;
Player players[2];
struct PreserveFp {unsigned csr=_mm_getcsr();~PreserveFp(){_mm_setcsr(csr);}};
uint32_t Read(const uint8_t* b,uint32_t at){uint32_t v;std::memcpy(&v,b+at,4);return v;}
void Write(uint8_t* b,uint32_t at,uint32_t value){std::memcpy(b+at,&value,4);}
bool Fresh(const Player& p){return p.updated!=Clock::time_point{}&&Clock::now()-p.updated<std::chrono::milliseconds(500);}
void Stop(Player& p){++p.revision;p.view.open=false;p.view.selected=-1;p.view.x=p.view.y=0;p.command={};p.fresh_release=true;p.release_publications=0;}
void Expire(Player& p){if(!Fresh(p)){Stop(p);p.view.available=false;}}
void Point(Player& p,float x,float y){
    PreserveFp fp;
    if(!std::isfinite(x)||!std::isfinite(y)){x=y=0;}
    const float length=std::sqrt(x*x+y*y);
    if(length>1){x/=length;y/=length;}
    p.view.x=x;p.view.y=y;p.view.selected=-1;
    if(p.view.count&&x*x+y*y>=0.09f){
        constexpr float pi=3.14159265358979323846f;
        float angle=std::atan2(x,-y);if(angle<0)angle+=2*pi;
        p.view.selected=int(std::floor(angle*p.view.count/(2*pi)+0.5f))%int(p.view.count);
    }
}
Identity Identify(const uint8_t* b,uint32_t owner){
    if(!b||!TrialGuestReadable(owner,0x890)||Read(b,owner)!=0x46fbe8u||
       (Read(b,owner+0x5c)&1)||!TrialGuestReadable(0x51c4dc,4))return {};
    const uint32_t slot=Read(b,owner+0x368),id=Read(b,owner+0x54),world=Read(b,0x51c4dc);
    if(slot>1||id==0xffffffffu||!TrialGuestReadable(world,0x14)||
       (slot&&(!trial_coop::Active()||TrialCoopPlayer(1)!=owner)))return {};
    const uint32_t level=Read(b,world+4);
    if(!TrialGuestReadable(level,0x203c))return {};
    const uint32_t at=level+0x3c+(id&1023)*8;
    if(Read(b,at)!=id||Read(b,at+4)!=owner)return {};
    return {owner,id,world,level,slot};
}
bool Available(const uint8_t* b,const Identity& p){
    if(p.slot>1)return false;
    const uint32_t a=p.owner,state=Read(b,a+0x170),health=Read(b,a+0x9c),current=Read(b,a+0x4fc);
    // C4CB0's explicit switch gates and C4E2C reload phase. Lowered/locked
    // weapons retain the retail raise path rather than acquiring a queued wheel request.
    return health>0&&health<0x7f800000u&&!(Read(b,p.world+0x10)&0x800u)&&
        current<25&&!b[a+0x304]&&!b[a+0x202]&&!b[a+0x2ac]&&!b[a+0x2ad]&&
        state!=3&&state!=7&&state!=11&&state!=12&&state!=13&&
        (state!=8||Read(b,a+0x174)==2);
}
struct GuestScratch {
    uint8_t* base;uint32_t at;uint8_t bytes[1024];unsigned csr;
    GuestScratch(uint8_t* b,uint32_t sp):base(b),at(sp-sizeof bytes),csr(_mm_getcsr()){std::memcpy(bytes,b+at,sizeof bytes);}
    ~GuestScratch(){std::memcpy(base+at,bytes,sizeof bytes);_mm_setcsr(csr);}
};
bool Stack(const X86Context& c){return c.esp>=0x10400&&TrialGuestReadable(c.esp-1024,1024);}
uint32_t Call(const X86Context& c,uint32_t object,uint32_t function,std::initializer_list<uint32_t> args={}){
    X86Context query=c;query.ecx=object;query.esp=c.esp-64;
    return kernel::CallGuest(query,function,args);
}
bool Item(const uint8_t* b,uint32_t inventory,uint32_t slot,Entry& out,uint32_t& address){
    if(slot==0||slot>=25||!TrialGuestReadable(inventory,0x100))return false;
    const uint32_t items=Read(b,inventory);
    if(!TrialGuestReadable(items,27*0x5c))return false;
    address=items+slot*0x5c;if(!b[address])return false;
    const uint32_t definition=Read(b,address+4);
    if(definition==0||definition>=25)return false;
    const uint32_t descriptor=0x4e55d0+definition*0xf0;
    if(!TrialGuestReadable(descriptor,0xf0))return false;
    const uint32_t category=Read(b,descriptor+8),reserve=Read(b,address+0x10);
    if(category>2||!TrialGuestReadable(reserve,4))return false;
    out={slot,definition,category,Read(b,address+0xc),Read(b,reserve),false};return true;
}
bool InGroup(Kind kind,uint32_t category){
    // Native category 0 includes the grenade launcher. Categories 1/2 include
    // thrown weapons, the Taser, and chainsaw; shield/turret remain excluded.
    return (kind==Kind::Guns&&category==0)||
        (kind==Kind::MeleeThrowables&&(category==1||category==2));
}
bool Inventory(const X86Context& c,uint8_t* b,const Identity& identity,Snapshot& view){
    const uint32_t inventory=identity.owner+0x498,current=Read(b,inventory+0x64);
    bool seen[27]{};
    for(unsigned category=0;category<3;++category){
        if(!InGroup(view.kind,category))continue;
        const uint32_t list=inventory+0x80+category*16,begin=Read(b,list),end=Read(b,list+4);
        if(end<begin||(end-begin)%4||end-begin>27*4||(end>begin&&!TrialGuestReadable(begin,end-begin)))return false;
        for(uint32_t at=begin;at<end;at+=4){
            const uint32_t slot=Read(b,at);if(slot>=27||seen[slot])return false;seen[slot]=true;
            Entry entry;uint32_t item=0;
            if(!Item(b,inventory,slot,entry,item))continue;
            if(entry.category!=category)return false;
            entry.usable=(Call(c,item,0x42da0)&255)!=0;
            if(slot==current)view.current=int(view.count);
            view.entries[view.count++]=entry;
        }
    }
    return view.count!=0;
}
}

Snapshot GetSnapshot(unsigned slot){std::lock_guard<std::mutex> lock(mutex);if(slot>1)return {};auto& p=players[slot];Expire(p);return p.view;}
void Cancel(unsigned slot){std::lock_guard<std::mutex> lock(mutex);if(slot<2)Stop(players[slot]);}
bool Blocked(unsigned slot){std::lock_guard<std::mutex> lock(mutex);if(slot>1)return false;auto& p=players[slot];Expire(p);return p.view.open||p.command.pending||p.release_publications;}
bool SlowTimeActive(){std::lock_guard<std::mutex> lock(mutex);auto& p=players[0];Expire(p);return p.single_player&&p.allowed&&p.view.available&&p.view.open;}
bool SlowTimeActive(uint32_t world){std::lock_guard<std::mutex> lock(mutex);auto& p=players[0];Expire(p);return p.identity.world==world&&p.single_player&&p.allowed&&p.view.available&&p.view.open;}
void PublishInput(unsigned slot){std::lock_guard<std::mutex> lock(mutex);if(slot<2&&players[slot].release_publications)--players[slot].release_publications;}
void Input(unsigned slot,bool down,bool allowed,bool keyboard,float x,float y,Kind kind){
    if(slot>1)return;std::lock_guard<std::mutex> lock(mutex);auto& p=players[slot];Expire(p);
    const bool pressed=down&&!p.down,released=!down&&p.down;
    if(!allowed||(kind!=Kind::Guns&&kind!=Kind::MeleeThrowables)||
       ((p.view.open||p.command.pending)&&(p.keyboard!=keyboard||p.view.kind!=kind))){
        Stop(p);p.allowed=allowed;p.down=down;p.keyboard=keyboard;return;
    }
    p.allowed=allowed;p.keyboard=keyboard;
    if(!down)p.fresh_release=false;
    if(pressed&&!p.fresh_release&&p.view.available&&Fresh(p)){
        ++p.revision;p.command={};p.view.kind=kind;p.view.open=true;p.view.x=p.view.y=0;p.view.selected=-1;p.view.count=0;p.view.current=-1;
    }
    if(p.view.open&&down&&!keyboard)Point(p,x,y);
    if(released&&p.view.open){
        const int selected=p.view.selected;
        if(selected>=0&&unsigned(selected)<p.view.count&&p.view.entries[selected].usable){
            const auto& e=p.view.entries[selected];
            p.command={p.identity,e.slot,e.definition,p.current_slot,true,p.view.kind};
        }
        ++p.revision;p.view.open=false;p.release_publications=2;
    }
    p.down=down;
}
bool MouseDelta(float dx,float dy){
    PreserveFp fp;
    std::lock_guard<std::mutex> lock(mutex);auto& p=players[0];Expire(p);
    if(!p.view.open||!p.keyboard)return p.command.pending||p.release_publications;
    Point(p,p.view.x+dx/180.f,p.view.y+dy/180.f);return true;
}
void GuestUpdate(X86Context& c,uint8_t* b){
    const auto identity=Identify(b,c.ecx);if(identity.slot>1)return;
    bool refresh=false;uint64_t revision=0;Snapshot view;
    {std::lock_guard<std::mutex> lock(mutex);const auto& p=players[identity.slot];refresh=p.view.open;revision=p.revision;view.kind=p.view.kind;}
    const bool admitted=Available(b,identity);view.available=admitted;
    if(view.available&&refresh){
        if(Stack(c)){GuestScratch scratch(b,c.esp);view.available=Inventory(c,b,identity,view);}
        else view.available=false;
    }
    std::lock_guard<std::mutex> lock(mutex);auto& p=players[identity.slot];
    if(!Same(identity,p.identity)||!admitted)Stop(p);
    p.identity=identity;p.current_slot=Read(b,identity.owner+0x4fc);p.updated=Clock::now();p.single_player=!trial_coop::Active()&&!trial_coop::Requested();
    // Input can cancel/reopen another wheel during a native query. Do not put
    // the previous group's inventory into the new wheel's copied snapshot.
    if(p.revision!=revision||p.view.kind!=view.kind){p.view.available=admitted;return;}
    if(!view.available)Stop(p);
    const bool open=p.view.open;const float x=p.view.x,y=p.view.y;
    p.view=view;p.view.open=open&&view.available;
    if(p.view.open)Point(p,x,y);
}
bool GuestSelect(X86Context& c,uint8_t* b){
    const auto identity=Identify(b,c.esi);if(identity.slot>1)return false;
    Command command;uint64_t revision=0;
    {
        std::lock_guard<std::mutex> lock(mutex);auto& p=players[identity.slot];
        command=p.command;revision=p.revision;p.command={};
    }
    if(!command.pending||!Same(command.identity,identity)||!Available(b,identity)||!Stack(c)||
       Read(b,identity.owner+0x4fc)!=command.current)return false;
    Entry entry;uint32_t item=0;const uint32_t inventory=identity.owner+0x498;
    if(!Item(b,inventory,command.slot,entry,item)||entry.definition!=command.definition||!InGroup(command.kind,entry.category))return false;
    // Revalidate membership too: a checkpoint/pickup can change the ordered list
    // without replacing the actor. Never select a stale previously-owned slot.
    const uint32_t at=inventory+0x80+entry.category*16,begin=Read(b,at),end=Read(b,at+4);
    if(end<begin||(end-begin)%4||end-begin>27*4||!TrialGuestReadable(begin,end-begin))return false;
    bool found=false;for(uint32_t p=begin;p<end;p+=4)found|=Read(b,p)==command.slot;
    if(!found)return false;
    GuestScratch scratch(b,c.esp);
    if(!(Call(c,item,0x42da0)&255))return false;
    {
        std::lock_guard<std::mutex> lock(mutex);const auto& p=players[identity.slot];
        if(p.revision!=revision||p.view.kind!=command.kind)return false;
    }
    if(command.current==command.slot)return true;
    // Same explicit category-switch setup as C4DE6/C4DF4 and 42840. The
    // untouched C4E03 tail owns shield/reload gates and the weapon animation.
    if(command.current==0)Call(c,identity.owner,0xbc560);
    Write(b,identity.owner+0x7f8,0xffffffffu);
    Call(c,inventory,0x42230,{command.slot});
    Write(b,inventory+0xf8,entry.category);Write(b,inventory+0xfc,0);
    return true;
}
void GuestLook(const X86Context& c,uint8_t* b){
    const auto identity=Identify(b,c.edi);
    if(identity.slot>1||!Blocked(identity.slot)||!TrialGuestReadable(c.esp,0x40))return;
    Write(b,c.esp+0x30,0);Write(b,c.esp+0x3c,0);
}
}

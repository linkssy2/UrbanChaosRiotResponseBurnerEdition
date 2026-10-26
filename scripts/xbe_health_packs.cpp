#include "xbe_health_packs.h"
#include "xbe_coop.h"
#include "xbe_coop_death.h"
#include "xbe_coop_cinematics.h"
#include "cpu/x86_ops.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <array>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <exception>
#include <fstream>
#include <string>
#include <vector>
#include <xmmintrin.h>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace health_packs {
constexpr unsigned capacity=3;
struct Player {uint32_t actor=0,id=0xffffffffu;unsigned count=1;bool hud=false;};
Player players[2];
struct Injection {
    uint32_t owner_id=0xffffffffu,target=0,target_id=0xffffffffu,clip=0;
    uint64_t generation=0;
    bool started=false,selected=false,cancelled=false,contact=false;
};
Injection injections[2];
uint32_t world=0;
std::atomic<uint64_t> generation{1},pending[2]{};
std::atomic<uint64_t> published[2]{};
std::atomic<bool> held[2]{};
struct Medic {uint32_t actor,id,owner,owner_id;};
std::vector<Medic> medics;
std::filesystem::path storage;
constexpr size_t checkpoint_size=0x1b8,profile_size=24,level_size=12;
using CheckpointKey=std::array<uint8_t,profile_size+level_size+checkpoint_size>;
struct Checkpoint {CheckpointKey key{};std::array<unsigned,2> counts{};};
std::vector<Checkpoint> checkpoints;
struct Restored {uint32_t actor=0;unsigned count=0;};
Restored restored[2];
thread_local uint32_t medic_owner=0;
thread_local TrialHealthPackDeliveryScope* delivery=nullptr;
uint32_t Read(const uint8_t* b,uint32_t p){uint32_t v;std::memcpy(&v,b+p,4);return v;}
float Real(const uint8_t* b,uint32_t p){float v;std::memcpy(&v,b+p,4);return v;}
bool Actor(const uint8_t* b,uint32_t p){return b&&TrialGuestReadable(p,0x890)&&Read(b,p)==0x46fbe8&&!(Read(b,p+0x5c)&1)&&Read(b,p+0x54)!=0xffffffffu;}
int Slot(const uint8_t* b,uint32_t p){
    if(!Actor(b,p)||!TrialGuestReadable(0x51c4dc,4)||Read(b,0x51c4dc)!=world)return -1;
    const unsigned s=Read(b,p+0x368);
    return s<2&&players[s].actor==p&&players[s].id==Read(b,p+0x54)?int(s):-1;
}
bool Alive(const uint8_t* b,uint32_t p){
    if(Slot(b,p)<0)return false;
    const float h=Real(b,p+0x9c);return std::isfinite(h)&&h>0&&Read(b,p+0x170)!=12;
}
struct HostRestore {unsigned csr=_mm_getcsr();~HostRestore(){_mm_setcsr(csr);}};
bool InjectionOwner(const uint8_t* b,unsigned slot){
    const auto& a=injections[slot];
    return a.target&&a.generation==generation.load()&&Slot(b,players[slot].actor)==int(slot)&&
        a.owner_id==players[slot].id;
}
bool PartnerInFront(const uint8_t* b,uint32_t owner,uint32_t partner,bool prompt=false){
    HostRestore host;
    float target[3];if(!TrialCoopDownedPosition(b,partner,target))return false;
    // C7894..C79E3 constructs the native aim ray from eye +6E0 along
    // normalized +6D0, multiplied by +15000. Use that verified direction;
    // model-space hypospray axes are different. The existing nearby query
    // retains its range limit; this only prevents injecting behind the player.
    double length=0,dot=0,distance=0;
    for(unsigned axis=0;axis<3;++axis){
        const float eye=Real(b,owner+0x6e0+axis*4),forward=Real(b,owner+0x6d0+axis*4);
        if(!std::isfinite(eye)||!std::isfinite(forward)||!std::isfinite(target[axis]))return false;
        const double delta=double(target[axis])-eye;
        length+=double(forward)*forward;dot+=delta*forward;distance+=delta*delta;
    }
    // The prompt follows a 30-degree aim cone; injection admission is unchanged.
    return length>1.e-8&&dot>0&&(!prompt||dot*dot>=.75*length*distance);
}
bool InjectionTarget(const uint8_t* b,unsigned slot){
    const auto& a=injections[slot];
    return Slot(b,a.target)==int(1-slot)&&Read(b,a.target+0x54)==a.target_id&&
        TrialCoopNearbyDownedPlayer(b,players[slot].actor)==a.target&&
        PartnerInFront(b,players[slot].actor,a.target);
}
bool CanInject(const uint8_t* b,uint32_t owner){
    const uint32_t state=Read(b,owner+0x170);
    return Real(b,owner+0x46c)==0&&!b[owner+0x470]&&state!=3&&state!=11&&state!=12&&state!=13;
}
struct Scratch {
    uint8_t* b;uint32_t address;std::array<uint8_t,256> saved;
    Scratch(uint8_t* base,uint32_t sp):b(base),address(sp-256){std::memcpy(saved.data(),b+address,256);}
    ~Scratch(){std::memcpy(b+address,saved.data(),256);}
};
bool Stack(const X86Context& c){return c.esp>=0x10100&&TrialGuestReadable(c.esp-256,256);}
uint32_t PartnerClip(const X86Context& c,uint8_t* b){
    if(!Stack(c)||!TrialGuestReadable(Read(b,0x51ca80),12))return 0;
    // 22B930/22B8D0 look up this name in the preloaded animation registry;
    // neither keeps the temporary path. The resource remains level-owned.
    constexpr char name[]="assets_coop\\medical\\eqp_hypospray_anim_heal_partner.kfa";
    HostRestore host;Scratch scratch(b,c.esp);
    const uint32_t path=c.esp-128,key=c.esp-136;
    std::memcpy(b+path,name,sizeof name);std::memcpy(b+key,&path,4);
    X86Context temp=c;temp.esp=c.esp-160;
    const uint32_t clip=kernel::CallGuest(temp,0x22b930,{key});
    return TrialGuestReadable(clip,16)?clip:0;
}
bool Key(const uint8_t* b,uint32_t snapshot,CheckpointKey& key){
    if(!b||!TrialGuestReadable(snapshot,checkpoint_size)||Read(b,snapshot)==0xffffffffu||
       !TrialGuestReadable(0x5152b0,profile_size)||!TrialGuestReadable(0x5096e0,4)||
       !TrialGuestReadable(0x51c4dc,4))return false;
    const uint32_t settings=Read(b,0x5096e0);
    if(!TrialGuestReadable(settings,0x34))return false;
    const uint32_t save=Read(b,settings+0x30);
    if(save+4!=snapshot||!TrialGuestReadable(save,4))return false;
    const uint32_t current_world=Read(b,0x51c4dc);
    if(!TrialGuestReadable(current_world,0x2c+level_size))return false;
    const auto* name=b+current_world+0x2c;
    const auto* end=static_cast<const uint8_t*>(std::memchr(name,0,level_size));
    if(!end||end==name)return false;
    // 2431B0 returns the level name; 54BF0 parses it for mission-type checks.
    // Preserve the exact profile identity/name and checkpoint, but normalize
    // unused bytes after the level-name terminator.
    key.fill(0);
    std::memcpy(key.data(),b+0x5152b0,profile_size);
    std::memcpy(key.data()+profile_size,name,size_t(end-name));
    std::memcpy(key.data()+profile_size+level_size,b+snapshot,checkpoint_size);
    return true;
}
std::filesystem::path File(const CheckpointKey& key){
    uint64_t hash=14695981039346656037ull;
    for(uint8_t byte:key){hash^=byte;hash*=1099511628211ull;}
    char name[32];std::snprintf(name,sizeof name,"%016llx.hpk",static_cast<unsigned long long>(hash));
    return storage/name;
}
bool Load(const CheckpointKey& key,std::array<unsigned,2>& counts){
    for(const auto& entry:checkpoints)if(entry.key==key){counts=entry.counts;return true;}
    if(storage.empty())return false;
    std::ifstream file(File(key),std::ios::binary);std::array<uint8_t,8> header{};CheckpointKey stored_key{};
    file.read(reinterpret_cast<char*>(header.data()),header.size());
    file.read(reinterpret_cast<char*>(stored_key.data()),stored_key.size());
    if(!file||header[0]!='U'||header[1]!='H'||header[2]!='P'||header[3]!=1||
       header[4]>capacity||header[5]>capacity||header[6]||header[7]||stored_key!=key||file.peek()!=EOF)return false;
    counts={header[4],header[5]};return true;
}
void Save(const CheckpointKey& key,const std::array<unsigned,2>& counts){
    for(auto& entry:checkpoints)if(entry.key==key){entry.counts=counts;goto disk;}
    if(checkpoints.size()==128)checkpoints.erase(checkpoints.begin());
    checkpoints.push_back({key,counts});
disk:
    if(storage.empty())return;
    std::error_code error;std::filesystem::create_directories(storage,error);if(error)return;
    const auto target=File(key);auto temporary=target;temporary+=".tmp";
    const std::array<uint8_t,8> header{'U','H','P',1,uint8_t(counts[0]),uint8_t(counts[1]),0,0};
    std::ofstream file(temporary,std::ios::binary|std::ios::trunc);
    file.write(reinterpret_cast<const char*>(header.data()),header.size());
    file.write(reinterpret_cast<const char*>(key.data()),key.size());file.flush();const bool written=bool(file);file.close();
    if(!written){std::filesystem::remove(temporary,error);return;}
#ifdef _WIN32
    if(!MoveFileExW(temporary.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))std::filesystem::remove(temporary,error);
#else
    std::filesystem::rename(temporary,target,error);if(error)std::filesystem::remove(temporary,error);
#endif
}
uint32_t Root(X86Context& c,uint8_t* b,uint32_t behavior){
    if(!TrialGuestReadable(behavior,0xd8))return 0;
    const uint32_t controller=Read(b,behavior+0xd0);
    if(!TrialGuestReadable(controller,0x20)||!Stack(c))return 0;
    HostRestore host;Scratch scratch(b,c.esp);X86Context temp=c;
    temp.esp=c.esp-64;temp.ecx=world;
    const uint32_t root=kernel::CallGuest(temp,0x207d70,{Read(b,controller+0x1c)});
    // These two original roots register the stock-limited 169570 callback.
    // Mission-specific patient healing roots keep their original behavior.
    return TrialGuestReadable(root,0x140)&&(Read(b,root)==0x46eef0||Read(b,root)==0x46f8e8)?root:0;
}
uint32_t MedicActor(const uint8_t* b,uint32_t behavior){
    if(!TrialGuestReadable(behavior,0xd8))return 0;
    const uint32_t actor=Read(b,behavior+0xd4);
    return TrialGuestReadable(actor,0x160)&&!(Read(b,actor+0x5c)&1)&&Read(b,actor+0x54)!=0xffffffffu?actor:0;
}
uint32_t Stock(const uint8_t* b,uint32_t medic){
    return TrialGuestReadable(world,0xf7c)?world+(Read(b,medic+0x15c)==4?0xf78:0xf74):0;
}
bool HudAllowed(const X86Context& c,uint8_t* b,uint32_t owner){
    // The original 73D80 HUD wrapper uses the world hide flag and
    // 4AF40(1), which also consults the active scripted mode masks.
    // Ask that predicate rather than guessing which cameras hide the HUD.
    if(!Stack(c)||!TrialGuestReadable(world,0x14)||(Read(b,world+0x10)&0x800)||
       !TrialGuestReadable(0x509a60,4)||!TrialGuestReadable(0x4e71a8,1)||TrialCoopPresentation()==2)return false;
    const uint32_t hud=Read(b,owner+0x300),controller=Read(b,0x509a60);
    if(!TrialGuestReadable(hud,0x18)||Read(b,hud)!=0x4531a0||Read(b,hud+4)!=owner||
       !TrialGuestReadable(controller,0x830))return false;
    const uint32_t type=Read(b,controller);
    // Campaign/frontend constructors derive from 450588 and preserve +34.
    if(type!=0x44bf20&&type!=0x4526f0&&type!=0x450588)return false;
    if(!TrialGuestReadable(type,0x38)||Read(b,type+0x34)!=0x4af40)return false;
    const float alpha=Real(b,hud+0x14);if(!std::isfinite(alpha)||alpha<=0)return false;
    HostRestore host;Scratch scratch(b,c.esp);X86Context temp=c;
    temp.esp=c.esp-64;temp.ecx=controller;
    return uint8_t(kernel::CallGuest(temp,0x4af40,{1}))!=0;
}
float BarExtent(const uint8_t* b,uint32_t owner){
    HostRestore host;
    const uint32_t hud=Read(b,owner+0x300);
    if(!TrialGuestReadable(hud,0xc)||Read(b,hud)!=0x4531a0||Read(b,hud+4)!=owner)return 0;
    uint32_t node=Read(b,hud+8);
    // 70D90's validated HUD tree ends in 76A50's profile-sized health fill.
    for(const auto step:{std::array<uint32_t,2>{0x453bd0,4},{0x453bd0,4},{0x453cd0,8},{0x453bf8,4}}){
        if(!TrialGuestReadable(node,step[1]+4)||Read(b,node)!=step[0])return 0;
        node=Read(b,node+step[1]);
    }
    if(!TrialGuestReadable(node,0x10)||Read(b,node)!=0x453550||Read(b,node+4)!=owner||Real(b,node+8)!=1)return 0;
    const uint32_t fill=Read(b,node+0xc);
    if(!TrialGuestReadable(fill,0xc)||Read(b,fill)!=0x4531c0)return 0;
    const float extent=Real(b,fill+8);
    return std::isfinite(extent)&&extent>=20&&extent<=2580?extent:0;
}
void Publish(const uint8_t* b,unsigned slot){
    const uint32_t owner=players[slot].actor;
    const bool valid=Slot(b,owner)==int(slot);
    const float extent=valid&&players[slot].hud?BarExtent(b,owner):0;
    const bool visible=extent>0;
    const uint32_t partner=visible?TrialCoopNearbyDownedPlayer(b,owner):0;
    const bool revive=partner&&Slot(b,partner)==int(1-slot)&&players[slot].count&&Alive(b,owner)&&
        CanInject(b,owner)&&!injections[slot].target&&PartnerInFront(b,owner,partner,true);
    uint32_t bits;std::memcpy(&bits,&extent,4);
    published[slot].store(valid?(uint64_t(bits)<<32)|players[slot].count|(visible?0x100u:0)|(revive?0x200u:0):0);
}
uint32_t Choose(const uint8_t* b,uint32_t medic){
    HostRestore host;
    double best=INFINITY;uint32_t result=0;
    for(unsigned slot=0;slot<2;++slot){
        const auto& p=players[slot];if(!Alive(b,p.actor)||p.count>=capacity)continue;
        double distance=0;bool finite=true;
        for(unsigned axis=0;axis<3;++axis){
            const float a=Real(b,p.actor+0x40+axis*4),m=Real(b,medic+0x40+axis*4);
            if(!std::isfinite(a)||!std::isfinite(m)){finite=false;break;}
            const double delta=double(a)-m;distance+=delta*delta;
        }
        if(finite&&distance<best){best=distance;result=p.actor;}
    }
    // A full inventory still needs a scoped owner so eligibility fails closed.
    if(!result&&Alive(b,players[0].actor))result=players[0].actor;
    return result;
}
uint32_t Select(X86Context& c,uint8_t* b,bool choosing){
    if(!b||!world||Read(b,0x51c4dc)!=world)return 0;
    const uint32_t actor=MedicActor(b,c.ecx);
    if(!actor||!Root(c,b,c.ecx))return 0;
    auto found=medics.end();
    for(auto it=medics.begin();it!=medics.end();++it)if(it->actor==actor){found=it;break;}
    if(found!=medics.end()&&(found->id!=Read(b,actor+0x54)||Slot(b,found->owner)<0||
       Read(b,found->owner+0x54)!=found->owner_id)){medics.erase(found);found=medics.end();}
    const bool choose=choosing&&TrialGuestReadable(c.ecx,0xfc)&&Read(b,c.ecx+0xf0)==0&&Read(b,c.ecx+0xf8)==0xffffffffu;
    if(found==medics.end()){
        if(medics.size()>=128)return 0;
        const uint32_t owner=Choose(b,actor);if(!owner)return 0;
        medics.push_back({actor,Read(b,actor+0x54),owner,Read(b,owner+0x54)});
        return owner;
    }
    if(choose){const uint32_t owner=Choose(b,actor);if(owner){found->owner=owner;found->owner_id=Read(b,owner+0x54);}}
    return found->owner;
}
}

void TrialHealthPackInput(unsigned slot,bool down,bool allowed){
    using namespace health_packs;if(slot>1)return;
    const bool previous=held[slot].exchange(down);
    if(!allowed){pending[slot]=0;return;}
    if(down&&!previous)pending[slot]=generation.load();
}
TrialHealthPackStatus TrialHealthPackSnapshot(unsigned slot){
    const uint64_t v=slot<2?health_packs::published[slot].load():0;
    const uint32_t bits=uint32_t(v>>32);float extent;std::memcpy(&extent,&bits,4);
    return {unsigned(v)&0xffu,health_packs::capacity,(v&0x100)!=0,(v&0x200)!=0,extent};
}
void TrialHealthPackStorage(const std::filesystem::path& directory){
    health_packs::storage=directory;health_packs::checkpoints.clear();
}
void TrialHealthPackCheckpointSaved(const X86Context& c,const uint8_t* b){
    using namespace health_packs;
    // The 4C342 rep movsd has advanced EDI past all 0x1b8 saved bytes.
    if(c.edi<checkpoint_size||Slot(b,players[0].actor)!=0)return;
    CheckpointKey key;if(!Key(b,c.edi-uint32_t(checkpoint_size),key))return;
    std::array<unsigned,2> counts{};
    for(unsigned i=0;i<2;++i)if(Slot(b,players[i].actor)==int(i))counts[i]=players[i].count;
    Save(key,counts);
}
void TrialHealthPackCheckpointLoaded(const X86Context& c,const uint8_t* b){
    using namespace health_packs;
    // CCA6C is reached only after the original valid-checkpoint checks.
    if(!Actor(b,c.ebx))return;const unsigned slot=Read(b,c.ebx+0x368);if(slot>1)return;
    CheckpointKey key;if(!Key(b,c.edi,key))return;
    std::array<unsigned,2> counts{};Load(key,counts);
    // Old/missing companion records restore zero, rather than minting a new
    // level-start pack every time an existing checkpoint is reloaded.
    if(Slot(b,c.ebx)==int(slot)){
        injections[slot].cancelled=true;
        restored[slot]={};players[slot].count=counts[slot];pending[slot]=0;Publish(b,slot);
    }else restored[slot]={c.ebx,counts[slot]};
}
void TrialHealthPackCreated(const X86Context& c,const uint8_t* b){
    using namespace health_packs;if(!Actor(b,c.eax))return;
    const unsigned slot=Read(b,c.eax+0x368);if(slot>1)return;
    if(slot==0){
        world=Read(b,0x51c4dc);players[0]={};players[1]={};medics.clear();++generation;
        restored[1]={};
        for(unsigned i=0;i<2;++i){pending[i]=0;published[i]=0;injections[i]={};}
    }
    for(auto& a:injections)if(a.target==c.eax)a.cancelled=true;
    injections[slot]={};
    const unsigned count=restored[slot].actor==c.eax?restored[slot].count:1;
    restored[slot]={};players[slot]={c.eax,Read(b,c.eax+0x54),count};Publish(b,slot);
}
void TrialHealthPackDestroyed(const X86Context& c,const uint8_t* b){
    using namespace health_packs;const int slot=Slot(b,c.ecx);if(slot<0)return;
    players[slot].actor=0;published[slot]=0;pending[slot]=0;
    injections[slot]={};
    for(auto& a:injections)if(a.target==c.ecx)a.cancelled=true;
    restored[slot]={};
    if(slot==0){++generation;medics.clear();world=0;published[1]=0;pending[1]=0;injections[1]={};}
}
void TrialHealthPackUpdate(X86Context& c,uint8_t* b){
    using namespace health_packs;HostRestore host;const int slot=Slot(b,c.ecx);
    if(slot<0){for(unsigned i=0;i<2;++i)if(players[i].actor==c.ecx){published[i]=0;pending[i]=0;}return;}
    const uint32_t owner=c.ecx;auto& p=players[slot];
    p.hud=HudAllowed(c,b,owner);
    auto& injection=injections[slot];
    if(injection.target){
        if(!InjectionOwner(b,unsigned(slot)))injection={};
        else{
            const bool animating=Read(b,owner+0x170)==3;
            if(animating)injection.started=true;
            else if(injection.started||!b[owner+0x470])injection={};
            if(injection.target&&(!Alive(b,owner)||!p.hud||!InjectionTarget(b,unsigned(slot)))){
                injection.cancelled=true;
                // Before native state entry we own the pending receipt. Once
                // playing, retain cancellation through the full animation so
                // its Heal event cannot fall back to a self-health refill.
                if(!animating){b[owner+0x470]=0;injection={};}
            }
        }
    }
    const uint64_t request=pending[slot].exchange(0);
    if(request&&request==generation.load()&&p.count&&p.hud&&Alive(b,owner)&&Stack(c)&&!injection.target){
        const uint32_t partner=TrialCoopNearbyDownedPlayer(b,owner);
        if(partner){
            if(Slot(b,partner)==1-slot&&CanInject(b,owner)&&PartnerInFront(b,owner,partner)){
                const uint32_t clip=PartnerClip(c,b);if(!clip){Publish(b,unsigned(slot));return;}
                Scratch scratch(b,c.esp);X86Context temp=c;temp.esp=c.esp-64;
                injection={p.id,partner,Read(b,partner+0x54),clip,generation.load()};
                try{kernel::CallGuest(temp,0xbd650,{0,0});}
                catch(...){injection={};throw;}
                if(!b[owner+0x470])injection={};
            }
        }
        else{
            Scratch scratch(b,c.esp);X86Context temp=c;temp.esp=c.esp-64;
            kernel::CallGuest(temp,0xbdab0,{});
            const float maximum=float(temp.st[temp.top&7]);
            const float health=Real(b,owner+0x9c),remaining=Real(b,owner+0x46c);
            const uint32_t state=Read(b,owner+0x170);
            if(std::isfinite(maximum)&&health<maximum&&remaining==0&&!b[owner+0x470]&&state!=3&&state!=11&&state!=13){
                temp=c;temp.esp=c.esp-64;
                // Original Heal receipt queues the first-person hypospray injection;
                // CD980 supplies BDAB0() health and D2AB0 applies its refill.
                kernel::CallGuest(temp,0xbd650,{0,0});
                if(b[owner+0x470])--p.count;
            }
        }
    }
    Publish(b,unsigned(slot));
}
bool TrialHealthPackPartnerAnimation(const uint8_t* b,uint32_t owner){
    using namespace health_packs;const int slot=Slot(b,owner);
    return slot>=0&&InjectionOwner(b,unsigned(slot))&&Read(b,owner+0x170)==3;
}
void TrialHealthPackInjectionClip(X86Context& c,const uint8_t* b){
    using namespace health_packs;const int slot=Slot(b,c.esi);
    // CDA75 supplies the original owner-list animation start, after lookup
    // and before pushing its result. Only replace this owner's action clip.
    if(slot>=0&&TrialHealthPackPartnerAnimation(b,c.esi)&&Read(b,c.esi+0x174)==1){
        c.eax=injections[slot].clip;injections[slot].selected=true;
    }
}
bool TrialHealthPackInjectionContact(X86Context& c,uint8_t* b){
    using namespace health_packs;const uint32_t owner=c.esi;const int slot=Slot(b,owner);
    if(slot<0||!TrialHealthPackPartnerAnimation(b,owner))return false;
    auto& a=injections[slot];
    // CD98E is reached only for the original named Heal event. A stale event
    // while lowering the weapon must neither finish this new action nor heal self.
    if(Read(b,owner+0x174)!=2||!a.selected||a.contact)return true;
    a.contact=true;
    if(a.cancelled||!Alive(b,owner)||!players[slot].count||!InjectionTarget(b,unsigned(slot))||
       !HudAllowed(c,b,owner))return true;
    HostRestore host;
    if(TrialCoopRevivePlayer(c,b,a.target)){
        --players[slot].count;
        // Retain the original injection sound, without its self-heal screen
        // pulse or pending refill. Stock is spent only after a successful revive.
        if(Stack(c)){
            Scratch scratch(b,c.esp);X86Context temp=c;temp.ecx=owner;temp.esp=c.esp-64;
            kernel::CallGuest(temp,0x216bd0,{Read(b,owner+0x2d4),0x458a98,0});
        }
        Publish(b,unsigned(slot));
    }
    return true;
}
TrialHealthPackMedicScope::TrialHealthPackMedicScope(X86Context& c,uint8_t* b,bool choosing){
    previous=health_packs::medic_owner;health_packs::medic_owner=health_packs::Select(c,b,choosing);
}
TrialHealthPackMedicScope::~TrialHealthPackMedicScope(){health_packs::medic_owner=previous;}
void TrialHealthPackMedicPlayer(X86Context& c,const uint8_t* b,bool pointer){
    using namespace health_packs;if(Slot(b,medic_owner)<0)return;
    if(pointer)c.ecx=Read(b,medic_owner+0x54);else c.eax=Read(b,medic_owner+0x54);
}
bool TrialHealthPackMedicNeeds(const uint8_t* b,bool original){
    using namespace health_packs;const int slot=Slot(b,medic_owner);
    return slot<0?original:Alive(b,medic_owner)&&players[slot].count<capacity;
}
TrialHealthPackDeliveryScope::TrialHealthPackDeliveryScope(X86Context& c,uint8_t* b){
    using namespace health_packs;previous=medic_owner;owner=Select(c,b,false);base=b;
    if(!owner)return;
    intercept=true;root=Root(c,b,c.ecx);const int slot=Slot(b,owner);const uint32_t medic=MedicActor(b,c.ecx);
    stock=Stock(b,medic);if(!stock)return;before=Read(b,stock);
    accepted=slot>=0&&Alive(b,owner)&&players[slot].count<capacity&&before>0&&before<=0x7fffffffu&&!delivery;
    if(accepted){medic_owner=owner;delivery=this;exceptions=std::uncaught_exceptions();}
}
TrialHealthPackDeliveryScope::~TrialHealthPackDeliveryScope(){
    using namespace health_packs;
    if(delivery==this){
        const int slot=Slot(base,owner);
        if(std::uncaught_exceptions()==exceptions&&received&&slot>=0&&Read(base,stock)+1==before&&players[slot].count<capacity){
            ++players[slot].count;Publish(base,unsigned(slot));
        }
        delivery=nullptr;
    }
    medic_owner=previous;
}
bool TrialHealthPackMedicReceipt(const X86Context& c,const uint8_t* b){
    using namespace health_packs;
    if(!delivery||!delivery->accepted||c.ecx!=delivery->owner||Slot(b,c.ecx)<0)return false;
    delivery->received=true;return true;
}
bool TrialHealthPackMedicStockBlocked(const X86Context& c,const uint8_t* b){
    using namespace health_packs;
    return delivery&&c.ecx==delivery->root&&Read(b,delivery->stock)!=delivery->before;
}

#include "xbe_coop_progression.h"
#include "xbe_coop_progression_assets.h"
#include "xbe_coop.h"
#include "xbe_coop_guest.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include "cpu/x86_ops.h"
#include "kernel/kernel.h"
#include <array>
#include <cstring>
#include <iterator>
#include <xmmintrin.h>

namespace {
using namespace coop_progression_assets;
uint32_t Read(const uint8_t* b,uint32_t p){uint32_t v;std::memcpy(&v,b+p,4);return v;}
struct Cache {
    uint32_t actor=0,id=0,descriptor=0,scripts=0,array=0;
    uint64_t pointers=0;
    bool matches=false;
};
std::array<Cache,std::size(kGates)> cache{};
uint32_t world=0,provider=0,players[2]{},ids[2]{};
struct HostRestore {unsigned value=_mm_getcsr();~HostRestore(){_mm_setcsr(value);}};
uint64_t Hash(const uint8_t* b,uint32_t size){
    uint64_t h=14695981039346656037ull;
    for(uint32_t i=0;i<size;++i)h=(h^b[i])*1099511628211ull;
    return h;
}
bool Pair(const uint8_t* b){
    if(!b||!trial_coop::Active())return false;
    const uint32_t w=Read(b,0x51c4dc);
    if(!TrialGuestReadable(w,8))return false;
    const uint32_t state=Read(b,w+4);
    if(!TrialGuestReadable(state,0x2058))return false;
    const uint32_t p=Read(b,state+0x2054);
    if(!TrialGuestReadable(p,0x3c))return false;
    uint32_t actors[2],identities[2];
    for(unsigned i=0;i<2;++i){
        actors[i]=TrialCoopPlayer(i);
        if(!TrialGuestReadable(actors[i],0x890)||Read(b,actors[i])!=0x46fbe8||
           Read(b,actors[i]+0x368)!=i||(Read(b,actors[i]+0x5c)&1))return false;
        identities[i]=Read(b,actors[i]+0x54);
        if(identities[i]==0xffffffff)return false;
    }
    if(actors[0]==actors[1]||identities[0]==identities[1])return false;
    if(world!=w||provider!=p||players[0]!=actors[0]||players[1]!=actors[1]||
       ids[0]!=identities[0]||ids[1]!=identities[1]){
        TrialCoopProgressionReset();world=w;provider=p;
        for(unsigned i=0;i<2;++i){players[i]=actors[i];ids[i]=identities[i];}
    }
    return true;
}
bool Alive(const uint8_t* b,uint32_t actor){
    const uint32_t hp=Read(b,actor+0x9c);
    return hp>0&&hp<0x7f800000&&Read(b,actor+0x170)!=12;
}
bool Authored(const uint8_t* b,size_t index,uint32_t actor,uint32_t desc){
    const auto& gate=kGates[index];
    const uint32_t scripts=Read(b,desc+0x64);
    if(!TrialGuestReadable(scripts,8)||Read(b,scripts)!=gate.script_count)return false;
    const uint32_t array=Read(b,scripts+4);
    if(!gate.script_count||gate.script_count>64||!TrialGuestReadable(array,gate.script_count*4))return false;
    // Only pointer identities are checked on subsequent frames. Disc script
    // buffers are immutable until level teardown; MainPlayer creation resets
    // this cache even when the allocator reuses every old address and ID.
    const uint64_t pointers=Hash(b+array,gate.script_count*4);
    auto& saved=cache[index];const uint32_t id=Read(b,actor+0x54);
    if(saved.actor==actor&&saved.id==id&&saved.descriptor==desc&&
       saved.scripts==scripts&&saved.array==array&&saved.pointers==pointers)return saved.matches;
    saved={actor,id,desc,scripts,array,pointers,false};
    if(gate.proof_count!=gate.script_count||gate.proof_begin>std::size(kProofs)||
       gate.proof_count>std::size(kProofs)-gate.proof_begin)return false;
    for(uint32_t i=0;i<gate.proof_count;++i){
        const auto& proof=kProofs[gate.proof_begin+i];
        if(proof.index>=gate.script_count||!proof.byte_count||proof.byte_count>65536)return false;
        const uint32_t code=Read(b,array+proof.index*4);
        if(!TrialGuestReadable(code,proof.byte_count)||Hash(b+code,proof.byte_count)!=proof.hash)return false;
    }
    saved.matches=true;return true;
}
const coop_progression_assets::Gate* GateFor(const uint8_t* b,const X86Context& c){
    if(!Pair(b)||!TrialGuestReadable(c.esi,0xe0)||Read(b,c.esi)!=0x480888||
       (Read(b,c.esi+0x5c)&1)||!TrialGuestReadable(c.ebp,0xc0))return nullptr;
    const auto kind=Read(b,c.ebp+0x50);if(kind!=37&&kind!=17)return nullptr;
    const uint32_t rows=Read(b,c.esi+0xd8),list=Read(b,c.esi+0xdc),phase=Read(b,c.esi+0xd4);
    if((phase!=0&&phase!=2)||rows>64||c.edi>=rows||!TrialGuestReadable(list,rows*8)||
       Read(b,list+c.edi*8)!=1)return nullptr;
    const uint32_t group=Read(b,list+c.edi*8+4),source=Read(b,c.esi+0x50);
    const uint32_t count=Read(b,provider+0x10),slots=Read(b,provider+4),definitions=Read(b,provider+0x1c);
    if(count>16384||source>=count||!TrialGuestReadable(slots,count*4)||c.ebp<definitions||
       (c.ebp-definitions)%0xc0)return nullptr;
    const uint32_t ordinal=(c.ebp-definitions)/0xc0,entry=Read(b,slots+source*4);
    if(!TrialGuestReadable(entry,0x14)||Read(b,entry+4)!=Read(b,c.esi+0x54)||
       !(b[entry+8]&8)||Read(b,entry+0x10)!=c.ebp)return nullptr;
    const uint32_t desc=Read(b,entry+0xc);
    if(!TrialGuestReadable(desc,0x68))return nullptr;
    for(size_t i=0;i<std::size(kGates);++i){
        const auto& gate=kGates[i];
        if(gate.provider_slot_count==count&&gate.source_index==source&&
           gate.volume_ordinal==ordinal&&gate.enter_group==group&&
           Read(b,c.ebp+0x50)==gate.trigger_kind&&Authored(b,i,c.esi,desc)){
            return &gate;
        }
    }
    return nullptr;
}
bool PartnerPastRear(const uint8_t* b,const coop_progression_assets::Gate& gate){
    for(unsigned axis=0;axis<3;++axis)
        if((Read(b,players[1]+0x40+axis*4)&0x7f800000)==0x7f800000)return false;
    const uint32_t xbits=Read(b,players[1]+0x40),radiusbits=Read(b,players[1]+0x88);
    if(radiusbits>=0x7f800000)return false;
    float x,y,z,radius_squared;std::memcpy(&x,&xbits,4);std::memcpy(&radius_squared,&radiusbits,4);
    std::memcpy(&y,b+players[1]+0x44,4);std::memcpy(&z,b+players[1]+0x48,4);
    if(y<gate.partner_min_y||y>gate.partner_max_y||z>gate.partner_max_z)return false;
    const double distance=double(x)-gate.partner_min_x;
    return distance>0&&distance*distance>double(radius_squared);
}
bool BarrierMetadata(const uint8_t* b,const X86Context& c,const coop_progression_assets::Gate& gate){
    const auto subjects=Read(b,c.ebp+0x94),end=Read(b,c.ebp+0x98);
    const auto boxes=Read(b,c.ebp+0x74),last=Read(b,c.ebp+0x78);
    if(gate.trigger_kind!=17||gate.subject_count!=3||
       end<subjects||end-subjects!=gate.subject_count*4||
       !TrialGuestReadable(subjects,gate.subject_count*4)||
       Hash(b+subjects,gate.subject_count*4)!=gate.subject_hash||
       Read(b,c.ebp+0x54)!=3||last<boxes||last-boxes!=48||
       !TrialGuestReadable(boxes,48)||Hash(b+boxes,24)!=gate.partner_volume_hash)return false;
    const auto count=Read(b,provider+0x10),slots=Read(b,provider+4);
    if(gate.barrier_source>=count||!TrialGuestReadable(slots+gate.barrier_source*4,4))return false;
    const auto entry=Read(b,slots+gate.barrier_source*4);
    if(!TrialGuestReadable(entry,0x10))return false;
    const auto desc=Read(b,entry+0xc);
    // The immutable descriptor starts with its 64-byte original transform.
    // 20BEAE..20BEC9 copies it to actor+10 before 111200 builds the box.
    return TrialGuestReadable(desc,64)&&Hash(b+desc,64)==gate.barrier_matrix_hash;
}
bool Completed(const uint8_t* b,uint32_t flag,bool& completed){
    // Script commands use local byte indices (217C81 -> 210770), not the
    // separate 2107F0 global-ID map. Provider+30 is the flag allocation length.
    const auto size=Read(b,provider+0x30),data=Read(b,provider+0x2c);
    if(!size||size>65536||flag>=size||!TrialGuestReadable(data,size))return false;
    completed=b[data+flag]!=0;return true;
}

bool PastBarrier(const uint8_t* b,uint32_t actor,float front){
    for(unsigned axis=0;axis<3;++axis)
        if((Read(b,actor+0x40+axis*4)&0x7f800000)==0x7f800000)return false;
    const auto radiusbits=Read(b,actor+0x88);if(radiusbits>=0x7f800000)return false;
    float z,radius_squared;std::memcpy(&z,b+actor+0x48,4);std::memcpy(&radius_squared,&radiusbits,4);
    const double distance=double(z)-front;
    return distance>0&&distance*distance>double(radius_squared);
}
}

void TrialCoopProgressionReset(){
    world=provider=0;players[0]=players[1]=ids[0]=ids[1]=0;cache.fill({});
}
void TrialCoopProgressionEnter(X86Context& c,uint8_t* b){
    if(!(c.eax&255))return;
    const auto* gate=GateFor(b,c);if(!gate)return;
    HostRestore host;
    const bool barrier=gate->completion_flag!=0xffffffffu;
    if(barrier){
        bool completed=false;
        if(!BarrierMetadata(b,c,*gate)||!Completed(b,gate->completion_flag,completed)||completed||
           !PastBarrier(b,players[0],gate->barrier_front_z)||
           !PastBarrier(b,players[1],gate->barrier_front_z)){c.eax&=0xffffff00;return;}
    }
    if(!Alive(b,players[0])||!Alive(b,players[1])){c.eax&=0xffffff00;return;}
    if(c.esp<0x18000||!TrialGuestReadable(c.esp-0x7000,0x7004)||
       Read(b,0x46fbe8+0x48)!=0x2187a0){if(barrier)c.eax&=0xffffff00;return;}
    const uint32_t shape=Read(b,c.ebp+0x54);
    if(shape>3)return;
    if(shape==3){
        const uint32_t first=Read(b,c.ebp+0x74),last=Read(b,c.ebp+0x78);
        if(last<=first||last-first>65536||(last-first)%48||!TrialGuestReadable(first,last-first))return;
    }
    X86Context temporary=c;
    const uint32_t sp=(c.esp-0x600)&~15u;
    const bool rear_boundary=gate->partner_min_x>0&&shape==3&&
        Read(b,c.ebp+0x78)-Read(b,c.ebp+0x74)==48&&
        Hash(b+Read(b,c.ebp+0x74),24)==gate->partner_volume_hash;
    for(unsigned i=0;i<2;++i){
        // Hope Street's hostage tripwire is only 30cm deep. Keep P1's native
        // overlap, but admit P2 wholly beyond the rear checkpoint entry plane.
        // Its floor/near-side limits exclude earlier areas, while no far-side
        // Z limit prevents a partner who crossed first from blocking the scene.
        if(i==1&&rear_boundary){
            if(!PartnerPastRear(b,*gate)){c.eax&=0xffffff00;return;}
            continue;
        }
        temporary.esp=sp;temporary.ecx=players[i];temporary.df=0;
        if(!(kernel::CallGuest(temporary,0x2187a0,{c.ebp})&255)){c.eax&=0xffffff00;return;}
    }
}

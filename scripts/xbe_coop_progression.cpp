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
bool GateFor(const uint8_t* b,const X86Context& c){
    if(!Pair(b)||!TrialGuestReadable(c.esi,0xe0)||Read(b,c.esi)!=0x480888||
       (Read(b,c.esi+0x5c)&1)||!TrialGuestReadable(c.ebp,0xc0)||Read(b,c.ebp+0x50)!=37)return false;
    const uint32_t rows=Read(b,c.esi+0xd8),list=Read(b,c.esi+0xdc),phase=Read(b,c.esi+0xd4);
    if((phase!=0&&phase!=2)||rows>64||c.edi>=rows||!TrialGuestReadable(list,rows*8)||
       Read(b,list+c.edi*8)!=1)return false;
    const uint32_t group=Read(b,list+c.edi*8+4),source=Read(b,c.esi+0x50);
    const uint32_t count=Read(b,provider+0x10),slots=Read(b,provider+4),definitions=Read(b,provider+0x1c);
    if(count>16384||source>=count||!TrialGuestReadable(slots,count*4)||c.ebp<definitions||
       (c.ebp-definitions)%0xc0)return false;
    const uint32_t ordinal=(c.ebp-definitions)/0xc0,entry=Read(b,slots+source*4);
    if(!TrialGuestReadable(entry,0x14)||Read(b,entry+4)!=Read(b,c.esi+0x54)||
       !(b[entry+8]&8)||Read(b,entry+0x10)!=c.ebp)return false;
    const uint32_t desc=Read(b,entry+0xc);
    if(!TrialGuestReadable(desc,0x68))return false;
    for(size_t i=0;i<std::size(kGates);++i){
        const auto& gate=kGates[i];
        if(gate.provider_slot_count==count&&gate.source_index==source&&
           gate.volume_ordinal==ordinal&&gate.enter_group==group&&Authored(b,i,c.esi,desc))return true;
    }
    return false;
}
}

void TrialCoopProgressionReset(){
    world=provider=0;players[0]=players[1]=ids[0]=ids[1]=0;cache.fill({});
}
void TrialCoopProgressionEnter(X86Context& c,uint8_t* b){
    if(!(c.eax&255)||!GateFor(b,c))return;
    if(!Alive(b,players[0])||!Alive(b,players[1])){c.eax&=0xffffff00;return;}
    if(c.esp<0x18000||!TrialGuestReadable(c.esp-0x7000,0x7004)||
       Read(b,0x46fbe8+0x48)!=0x2187a0)return;
    const uint32_t shape=Read(b,c.ebp+0x54);
    if(shape>3)return;
    if(shape==3){
        const uint32_t first=Read(b,c.ebp+0x74),last=Read(b,c.ebp+0x78);
        if(last<=first||last-first>65536||(last-first)%48||!TrialGuestReadable(first,last-first))return;
    }
    HostRestore host;X86Context temporary=c;
    const uint32_t sp=(c.esp-0x600)&~15u;
    for(unsigned i=0;i<2;++i){
        temporary.esp=sp;temporary.ecx=players[i];temporary.df=0;
        if(!(kernel::CallGuest(temporary,0x2187a0,{c.ebp})&255)){c.eax&=0xffffff00;return;}
    }
}

#include "xbe_coop_shields.h"
#include "xbe_coop.h"
#include "xbe_coop_guest.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include "cpu/x86_ops.h"
#include "kernel/kernel.h"
#include <cstring>
#include <xmmintrin.h>

namespace {
uint32_t Read(const uint8_t* b,uint32_t p){uint32_t v;std::memcpy(&v,b+p,4);return v;}
// These are the original two "give shield" scripts, not synthesized commands.
// 30 removes prop585 and disables32; 32 removes578 and disables30. Both set87.
constexpr uint32_t scripts[2][31]={
    {17,1062,52,20,17,585,10,87,22,0xffffffff,53,0xffffffff,53,32,21,0x3e4ccccd,
     10,95,22,0xffffffff,21,0x3f800000,23,1301,10,37,21,0,17,334,0xffffffff},
    {17,1062,52,20,17,578,10,87,22,0xffffffff,53,30,53,0xffffffff,21,0x3e4ccccd,
     10,95,22,0xffffffff,21,0x3f800000,23,1301,10,37,17,334,0xffffffff}
};
constexpr unsigned words[2]={31,29},ordinals[2]={30,32};
uint32_t world=0,players[2]{},ids[2]{},provider=0;
uint32_t candidate[2]{},candidate_id[2]{};
uint32_t candidate_trigger[2]{},candidate_trigger_id[2]{};
unsigned acquired=0;
bool armed=false;
struct HostRestore {unsigned value=_mm_getcsr();~HostRestore(){_mm_setcsr(value);}};

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
        identities[i]=Read(b,actors[i]+0x54);if(identities[i]==0xffffffff)return false;
    }
    if(actors[0]==actors[1]||identities[0]==identities[1])return false;
    if(world!=w||provider!=p||players[0]!=actors[0]||players[1]!=actors[1]||
       ids[0]!=identities[0]||ids[1]!=identities[1]){
        world=w;provider=p;armed=false;acquired=0;
        for(unsigned i=0;i<2;++i){players[i]=actors[i];ids[i]=identities[i];
            candidate[i]=candidate_id[i]=candidate_trigger[i]=candidate_trigger_id[i]=0;}
    }
    return true;
}
bool Alive(const uint8_t* b,unsigned slot){
    const uint32_t health=Read(b,players[slot]+0x9c);
    return health>0&&health<0x7f800000&&Read(b,players[slot]+0x170)!=12;
}
bool AuthoredGate(const uint8_t* b){
    for(unsigned i=0;i<2;++i){
        const uint32_t f=Read(b,players[i]+0x84c);
        if(Read(b,players[i]+0x850)!=1||!TrialGuestReadable(f,4)||Read(b,f)!=87)return false;
    }
    const uint32_t f=Read(b,provider+0x2c);
    return f<=UINT32_MAX-87&&TrialGuestReadable(f+87,1);
}
// Follow exactly 211450/210F30's provider slot and script descriptor chains.
int Trigger(const uint8_t* b,uint32_t actor,uint32_t& volume){
    if(!Pair(b)||!AuthoredGate(b)||!TrialGuestReadable(actor,0xe0)||
       Read(b,actor)!=0x480888||(Read(b,actor+0x5c)&1))return -1;
    const uint32_t index=Read(b,actor+0x50),count=Read(b,provider+0x10),slots=Read(b,provider+4);
    if(index>=count||count>16384||!TrialGuestReadable(slots,count*4))return -1;
    const uint32_t entry=Read(b,slots+index*4);
    if(!TrialGuestReadable(entry,0x14)||Read(b,entry+4)!=Read(b,actor+0x54)||!(b[entry+8]&8))return -1;
    const uint32_t desc=Read(b,entry+0xc);
    volume=Read(b,entry+0x10);
    if(!TrialGuestReadable(desc,0x68)||!TrialGuestReadable(volume,0xc0)||
       Read(b,volume+0x50)!=37||Read(b,volume+0x54)!=3)return -1;
    const uint32_t definitions=Read(b,provider+0x1c);
    int which=-1;
    for(unsigned i=0;i<2;++i)if(uint64_t(definitions)+ordinals[i]*0xc0==volume)which=int(i);
    if(which<0)return -1;
    const uint32_t scripts_desc=Read(b,desc+0x64);
    if(!TrialGuestReadable(scripts_desc,8)||Read(b,scripts_desc)!=1)return -1;
    const uint32_t array=Read(b,scripts_desc+4);
    if(!TrialGuestReadable(array,4))return -1;
    const uint32_t code=Read(b,array);
    if(!TrialGuestReadable(code,words[which]*4)||std::memcmp(b+code,scripts[which],words[which]*4))return -1;
    const uint32_t boxes=Read(b,volume+0x74),end=Read(b,volume+0x78);
    if(end<boxes||end-boxes!=48||!TrialGuestReadable(boxes,48))return -1;
    return which;
}
void Arm(const uint8_t* b){
    if(armed)return;
    // A restored checkpoint with the original flag already set keeps its
    // restored equipment. A fresh locked gate begins with separate ownership.
    if(b[Read(b,provider+0x2c)+87])acquired=3;
    else for(unsigned i=0;i<2;++i)if(!b[players[i]+0x854])acquired|=1u<<i;
    armed=true;
}
void ClearCandidate(unsigned which){
    candidate[which]=candidate_id[which]=candidate_trigger[which]=candidate_trigger_id[which]=0;
}
bool Inside(X86Context& c,uint32_t sp,uint32_t player,uint32_t volume){
    c.esp=sp;c.ecx=player;c.df=0;
    return (kernel::CallGuest(c,0x2187a0,{volume})&255)!=0;
}
void RefreshCandidate(X86Context& c,uint8_t* b,uint32_t sp,unsigned which){
    if(!candidate[which])return;
    uint32_t volume=0;const uint32_t trigger=candidate_trigger[which];
    if(Trigger(b,trigger,volume)!=int(which)||Read(b,trigger+0x54)!=candidate_trigger_id[which]){
        ClearCandidate(which);return;
    }
    // Once the original enter script starts, leaving its volume must not give
    // the same player the other shield while its first commands are pending.
    if(Read(b,trigger+0xd4))return;
    for(unsigned slot=0;slot<2;++slot)if(candidate[which]==players[slot]&&candidate_id[which]==ids[slot]){
        if(!Alive(b,slot)||!Inside(c,sp,players[slot],volume))ClearCandidate(which);
        return;
    }
    ClearCandidate(which);
}
}

void TrialCoopShieldReset(){
    world=provider=acquired=0;armed=false;
    for(unsigned i=0;i<2;++i){players[i]=ids[i]=0;ClearCandidate(i);}
}
bool TrialCoopShieldOverlap(X86Context& c,uint8_t* b){
    uint32_t volume=0;const int which=Trigger(b,c.ecx,volume);
    if(which<0||c.esp<0x18000||!TrialGuestReadable(c.esp-0x7000,0x7004)||
       Read(b,0x46fbe8+0x48)!=0x2187a0)return false;
    Arm(b);HostRestore host;X86Context temporary=c;bool inside=false;
    const uint32_t sp=(c.esp-0x600)&~15u;
    for(unsigned i=0;i<2;++i)RefreshCandidate(temporary,b,sp,i);
    for(unsigned slot=0;slot<2;++slot){
        if((acquired&(1u<<slot))||!Alive(b,slot)||
           (candidate[1-which]==players[slot]&&candidate_id[1-which]==ids[slot]))continue;
        if(!Inside(temporary,sp,players[slot],volume))continue;
        inside=true;
        if(!Read(b,c.ecx+0xd4)){
            candidate[which]=players[slot];candidate_id[which]=ids[slot];
            candidate_trigger[which]=c.ecx;candidate_trigger_id[which]=Read(b,c.ecx+0x54);
        }
        break;
    }
    if(!inside&&!Read(b,c.ecx+0xd4))ClearCandidate(unsigned(which));
    c.eax=(c.eax&0xffffff00u)|unsigned(inside);c.esp+=4;return true;
}
bool TrialCoopShieldCommand(X86Context& c,uint8_t* b){
    if(!TrialGuestReadable(c.esp,8))return false;
    const uint32_t command=Read(b,c.esp+4);
    if(!TrialGuestReadable(command,16))return false;
    const uint32_t opcode=Read(b,command+4),arg=Read(b,command+8);
    if((opcode!=10||arg!=87)&&(opcode!=53||(arg!=30&&arg!=32)))return false;
    uint32_t volume=0;const int which=Trigger(b,c.ecx,volume);
    if(which<0||!armed||Read(b,c.ecx+0xc0)!=command)return false;
    if(opcode==10&&b[command+0xc]){
        // 217BB0 immediately executes its original non-failing flag87 setter.
        // Ownership follows the actor that satisfied this script's enter test.
        for(unsigned i=0;i<2;++i)if(candidate[which]==players[i]&&candidate_id[which]==ids[i])acquired|=1u<<i;
        ClearCandidate(unsigned(which));
    }else if(opcode==53&&!b[command+0xc]&&arg==ordinals[1-which]){
        // Retain the other authored pickup. This command's own cursor/free
        // remain owned by 216FA0, which observes the normal successful return.
        c.eax=(c.eax&0xffffff00u)|1u;c.esp+=8;return true;
    }
    return false;
}
void TrialCoopShieldPermission(X86Context& c,const uint8_t* b){
    if(!Pair(b)||!armed||!AuthoredGate(b))return;
    for(unsigned i=0;i<2;++i)if(c.esi==players[i]&&!(acquired&(1u<<i))){c.eax&=0xffffff00u;return;}
}

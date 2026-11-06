#include "xbe_corpse_shield_impact.h"
#include "xbe_corpse_shield.h"
#include "cpu/x86_ops.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <array>
#include <cmath>
#include <cstring>
#include <initializer_list>
#include <xmmintrin.h>

namespace corpse_shield_impact {
struct FP {unsigned csr=_mm_getcsr();~FP(){_mm_setcsr(csr);}};
uint32_t Word(const uint8_t* b,uint32_t p){uint32_t v;std::memcpy(&v,b+p,4);return v;}
void Put(uint8_t* b,uint32_t p,uint32_t v){std::memcpy(b+p,&v,4);}
bool Vector(const uint8_t* b,uint32_t p,std::array<float,3>& v){
    if(!TrialGuestReadable(p,12))return false;
    std::memcpy(v.data(),b+p,12);
    return std::isfinite(v[0])&&std::isfinite(v[1])&&std::isfinite(v[2]);
}
struct Calls {
    X86Context original,c;
    uint8_t* b;uint32_t low,sp,data;
    std::array<uint8_t,4096> saved;
    Calls(const X86Context& context,uint8_t* base):original(context),c(context),b(base),low(c.esp-4096),
        sp((c.esp-512)&~15u),data((c.esp-256)&~15u){std::memcpy(saved.data(),b+low,saved.size());}
    ~Calls(){std::memcpy(b+low,saved.data(),saved.size());}
    void Run(uint32_t fn,std::initializer_list<uint32_t> args){
        c=original;c.esp=sp;
        for(auto it=args.end();it!=args.begin();){c.esp-=4;Put(b,c.esp,*--it);}
        c.esp-=4;Put(b,c.esp,0xc9402);x86_icall(c,b,fn);
    }
};
}

uint32_t TrialCorpseShieldSound(const uint8_t* b,uint32_t owner,uint32_t original){
    return (original==0x458798||original==0x4587ac)&&TrialCorpseShieldActive(b,owner)?0x45f69c:original;
}

uint32_t TrialCorpseShieldDecalCount(const uint8_t* b,uint32_t batch,uint32_t original){
    using namespace corpse_shield_impact;
    if(!b||!TrialGuestReadable(batch,0x1c)||Word(b,batch)!=0x457b8c)return original;
    const uint32_t owner=Word(b,batch+4);
    if(!TrialCorpseShieldActive(b,owner)||!TrialGuestReadable(owner,0x2a8))return original;
    const uint32_t first=Word(b,owner+0x2a4),offset=batch-first;
    if(!TrialGuestReadable(first,5*0x1c)||batch<first||offset>=5*0x1c||offset%0x1c)return original;
    // Hide shield-plane decals without erasing the ordinary shield's stored damage.
    return 0;
}

bool TrialCorpseShieldImpact(X86Context& c,uint8_t* b){
    using namespace corpse_shield_impact;
    FP fp;
    if(!TrialCorpseShieldActive(b,c.esi)||c.ecx!=c.esi||c.esp<0x11000||
       !TrialGuestReadable(c.esp-4096,4096)||!TrialGuestReadable(c.esp,0xc0)||
       Word(b,c.esp+4)!=c.esp+0xbc||Word(b,c.esp+8)!=3)return false;
    std::array<float,3> point,normal;
    if(!Vector(b,c.esp+0x1c,point)||!Vector(b,c.esi+0x6d0,normal))return false;
    const float square=normal[0]*normal[0]+normal[1]*normal[1]+normal[2]*normal[2];
    if(!std::isfinite(square)||square<.5f||square>1.5f)return false;
    Calls call(c,b);
    const uint32_t frame=call.data,n=frame+64,p=frame+80,bank=frame+96;
    for(auto& v:normal)v/=std::sqrt(square);
    std::memcpy(b+n,normal.data(),12);std::memcpy(b+p,point.data(),12);Put(b,bank,0);
    call.Run(0x5e780,{frame,n,0,p});
    // Retail material 9 retains the blood setting and authored blood particles.
    // Sound is replaced at the shield-only caller; a null bank prevents a second hit sound.
    call.Run(0xeb150,{frame,9,0,bank});
    // Replace only AddActor(ShieldBulletSpark), retaining descriptor cleanup and recoil.
    c.esp+=12;c.eax=0;return true;
}

#undef NDEBUG
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>
#include <vector>
#include "xbe_corpse_shield_impact.cpp"

namespace impact_test {
alignas(16) std::array<uint8_t,0x800000> memory{};
constexpr uint32_t P=0x600000,Q=0x602000,W=0x610000,SETTINGS=0x620000,
    SESSION=0x630000,SESSION_VT=0x631000,MANAGER=0x640000,MANAGER_VT=0x641000,SP=0x780000;
uint32_t ready=0;bool committed=true,multiplayer=false;
std::array<float,16> frame{};
std::vector<std::string> particles;
unsigned frames=0;
uint8_t* B(){return memory.data();}
uint32_t U(uint32_t p){return corpse_shield_impact::Word(B(),p);}
void U(uint32_t p,uint32_t v){corpse_shield_impact::Put(B(),p,v);}
void F(uint32_t p,float v){std::memcpy(B()+p,&v,4);}
}
namespace mem {bool IsCommitted(uint32_t p){return impact_test::committed&&p<impact_test::memory.size();}}
bool TrialCorpseShieldActive(const uint8_t* b,uint32_t owner){return b&&owner&&owner==impact_test::ready;}
X86_FUNC(sub_0005E780);X86_FUNC(sub_000EB150);
X86_FUNC(sub_000643E0){ctx.eax=impact_test::SETTINGS;ctx.esp+=4;}
X86_FUNC(sub_0004A840){ctx.eax=impact_test::SESSION;ctx.esp+=4;}
X86_FUNC(sub_00208C50){assert(ctx.ecx==impact_test::W);ctx.eax=impact_test::MANAGER;ctx.esp+=4;}
X86_FUNC(sub_00219450){
    using namespace impact_test;
    std::memcpy(frame.data(),B()+U(ctx.esp+4),sizeof frame);++frames;
    ctx.eax=0x650000;ctx.esp+=4;
}
// Material9 must not issue metal/decal/quota/sound requests.
X86_FUNC(sub_00055ED0){std::abort();}
X86_FUNC(sub_00214F90){std::abort();}
X86_FUNC(sub_00208980){std::abort();}
X86_FUNC(sub_002088D0){std::abort();}
X86_FUNC(sub_002D37B0){std::abort();}
void x86_icall(X86Context& c,uint8_t* b,uint32_t target){
    using namespace impact_test;
    if(target==0x5e780){sub_0005E780(c,b);return;}
    if(target==0xeb150){assert(U(c.esp+8)==9&&U(c.esp+12)==0&&U(U(c.esp+16))==0);sub_000EB150(c,b);return;}
    if(target==0x700000){assert(c.ecx==SESSION);c.eax=multiplayer?1:0;c.esp+=4;return;}
    if(target==0x700010){
        assert(c.ecx==MANAGER&&U(c.esp+4)==0x650000);
        particles.emplace_back(reinterpret_cast<const char*>(b+U(U(c.esp+8))));c.esp+=12;return;
    }
    std::abort();
}
[[noreturn]]void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
void x86_resume(X86Context&,uint8_t*,const X86Resume&){std::abort();}
// NATIVE_CORPSE_SHIELD_IMPACT_CONTRACTS

namespace impact_test {
void Seed(uint32_t owner,bool blood=true,bool multi=false){
    memory.fill(0);ready=owner;committed=true;multiplayer=multi;frames=0;particles.clear();
    // NATIVE_CORPSE_SHIELD_IMPACT_DATA
    U(0x51c4dc,W);B()[SETTINGS+0x32]=blood?1:0;U(SESSION,SESSION_VT);U(SESSION_VT+0x20,0x700000);
    U(MANAGER,MANAGER_VT);U(MANAGER_VT+0x10,0x700010);
    std::fill(memory.begin()+SP-4096,memory.begin()+SP,uint8_t(0x5a));
    U(SP,0x660000);U(SP+4,SP+0xbc);U(SP+8,3);
    F(SP+0x1c,12);F(SP+0x20,34);F(SP+0x24,56);
    F(owner+0x6d0,.6f);F(owner+0x6d4,.8f);F(owner+0x6d8,0);
}
X86Context Context(uint32_t owner){
    X86Context c{};c.ecx=c.esi=owner;c.esp=SP;c.eax=71;c.ebx=83;c.ebp=97;c.edi=107;
    c.top=3;c.st[3]=17;return c;
}
void Check(uint32_t owner,bool blood,bool multi){
    Seed(owner,blood,multi);auto c=Context(owner),before=c;
    std::array<uint8_t,4096> stack;std::memcpy(stack.data(),B()+SP-4096,stack.size());const auto csr=_mm_getcsr();
    assert(TrialCorpseShieldImpact(c,B()));before.esp+=12;before.eax=0;
    assert(!std::memcmp(&c,&before,sizeof c));assert(_mm_getcsr()==csr);
    assert(!std::memcmp(stack.data(),B()+SP-4096,stack.size()));
    assert(frames==(blood?3u:0u)&&particles.size()==frames);
    if(blood){
        const std::string prefix=multi?"bullet_hit\\multiplayer-":"bullet_hit\\";
        assert(particles[0]==prefix+"blood.simpleparticlesystem"&&particles[1]==particles[0]);
        assert(particles[2]==prefix+"bloodsplat.simpleparticlesystem");
        assert(std::abs(frame[0]-.6f)<1e-6f&&std::abs(frame[1]-.8f)<1e-6f&&frame[2]==0);
        assert(frame[12]==12&&frame[13]==34&&frame[14]==56&&frame[15]==1);
        for(unsigned a=0;a<3;++a)for(unsigned z=0;z<3;++z){
            float dot=0;for(unsigned i=0;i<3;++i)dot+=frame[a*4+i]*frame[z*4+i];
            assert(std::abs(dot-(a==z?1.f:0.f))<1e-5f);
        }
    }
}
void CheckDecals(uint32_t owner){
    Seed(owner);
    constexpr uint32_t batches=0x680000,outputs=0x690000;
    U(owner+0x2a4,batches);
    for(unsigned i=0;i<5;++i){
        const uint32_t batch=batches+i*0x1c,count=i+7,texture=0x700000+i;
        U(batch,0x457b8c);U(batch+4,owner);U(batch+8,texture);U(batch+0x18,count);
        for(bool active:{true,false}){
            ready=active?owner:0;
            auto c=Context(owner);c.ecx=batch;
            U(SP+4,outputs);U(SP+8,outputs+4);U(SP+12,outputs+8);
            sub_000BC080(c,B());
            assert(c.esp==SP+16&&U(outputs)==(active?0:count));
            assert(U(outputs+4)==2&&U(outputs+8)==texture&&U(batch+0x18)==count);
        }
    }
    ready=owner;
    const uint32_t batch=batches;
    assert(TrialCorpseShieldDecalCount(B(),batch,7)==0);
    U(batch+4,owner==P?Q:P);assert(TrialCorpseShieldDecalCount(B(),batch,7)==7);U(batch+4,owner);
    U(batch,0x457b6c);assert(TrialCorpseShieldDecalCount(B(),batch,7)==7);U(batch,0x457b8c);
    for(uint32_t first:{batch+0x1cu,batch-5*0x1cu,batch-1u,0xffffff80u}){
        U(owner+0x2a4,first);assert(TrialCorpseShieldDecalCount(B(),batch,7)==7);
    }
    U(owner+0x2a4,batch);committed=false;
    assert(TrialCorpseShieldDecalCount(B(),batch,7)==7);committed=true;
    assert(TrialCorpseShieldDecalCount(nullptr,batch,7)==7);
    assert(TrialCorpseShieldDecalCount(B(),0xfffffff0,7)==7);
}
}
int main(){
    using namespace impact_test;
    for(uint32_t owner:{P,Q})for(bool blood:{false,true})for(bool multi:{false,true})Check(owner,blood,multi);
    for(unsigned guard=0;guard<9;++guard){
        Seed(P);auto c=Context(P);
        switch(guard){
        case 0:ready=0;break;
        case 1:ready=Q;break;
        case 2:c.ecx=Q;break;
        case 3:c.esp=0xffff;break;
        case 4:committed=false;break;
        case 5:U(SP+4,SP+0xb8);break;
        case 6:U(SP+8,2);break;
        case 7:F(SP+0x1c,std::numeric_limits<float>::quiet_NaN());break;
        case 8:F(P+0x6d0,0);F(P+0x6d4,0);break;
        }
        const auto before=c;assert(!TrialCorpseShieldImpact(c,B()));assert(!std::memcmp(&c,&before,sizeof c));assert(particles.empty());
    }
    for(uint32_t owner:{P,Q}){
        CheckDecals(owner);
        Seed(owner);for(uint32_t sound:{0x458798u,0x4587acu}){
            assert(TrialCorpseShieldSound(B(),owner,sound)==0x45f69c);
            assert(TrialCorpseShieldSound(B(),owner==P?Q:P,sound)==sound);
            ready=0;assert(TrialCorpseShieldSound(B(),owner,sound)==sound);ready=owner;
        }
        assert(TrialCorpseShieldSound(B(),owner,0x123456)==0x123456);
    }
    std::puts("corpse shield native blood/settings, isolated sound/decals, stack/register and fallback checks passed");
}

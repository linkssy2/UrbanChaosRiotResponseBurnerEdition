#undef NDEBUG
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <vector>
#include "xbe_corpse_shield_physics.cpp"

namespace shield_test {
using corpse_physics::Matrix;
alignas(16) std::array<uint8_t,0x800000> ram{};
constexpr uint32_t SYSTEM=0x600000,WORLD=0x601000,SOURCE=0x602000,CLONE=0x603000,
    MODEL=0x604000,RENDER=0x605000,VTABLE=0x606000,SKELETON=0x607000,METHODS=0x608000,
    REST=0x609000,OWNER=0x60a000,LIST=0x610000,JOINTS=0x611000,BODIES=0x620000,
    MOTIONS=0x630000,ISLAND=0x640000,TOP=0x780000,FS=0x790000;
unsigned allocated=0,freed=0,created=0,destroyed=0,seeds=0,syncs=0,keys=0,properties=0,conversions=0;
bool no_stack=false,no_heap=false,no_skeleton=false;
std::vector<uint32_t> calls;
uint8_t* B(){return ram.data();}
uint32_t U(uint32_t a){return corpse_physics::Word(B(),a);}
void U(uint32_t a,uint32_t v){corpse_physics::Put(B(),a,v);}
float F(uint32_t a){float v;std::memcpy(&v,B()+a,4);return v;}
void F(uint32_t a,float v){std::memcpy(B()+a,&v,4);}
uint32_t Body(unsigned i){return BODIES+i*0x100;}
uint32_t Motion(unsigned i){return MOTIONS+i*0x100;}
Matrix Identity(){Matrix m{};m[0]=m[5]=m[10]=m[15]=1;return m;}
Matrix Swap(){auto m=Identity();m[5]=m[10]=0;m[6]=m[9]=1;return m;}
X86Context Context(){X86Context c{};c.esp=0x760000;c.fs_base=FS;c.fcw=0x37f;c.mxcsr=0x1f80;return c;}
void Ret(X86Context& c,uint32_t result=0,unsigned bytes=0){c.eax=result;c.esp+=4+bytes;}
}
namespace mem {
bool IsCommitted(uint32_t page){return page<shield_test::ram.size();}
uint32_t AllocStack(uint32_t size){using namespace shield_test;assert(size==0x10000);if(no_stack)return 0;++allocated;return TOP;}
void FreeStack(uint32_t top){using namespace shield_test;assert(top==TOP);++freed;}
}
X86_FUNC(sub_002D5640);X86_FUNC(sub_002E84A0);X86_FUNC(sub_002EC5C0);X86_FUNC(sub_002EC5E0);
// The platform CRT acos and island queue are explicit boundaries; actual
// hard-keyframe, quaternion, COM and motion-velocity arithmetic runs below.
X86_FUNC(sub_002D681C){ctx.st[ctx.top]=std::acos(ctx.st[ctx.top]);ctx.esp+=4;}
X86_FUNC(sub_002F6CC0){std::abort();}
void x86_icall(X86Context& c,uint8_t* b,uint32_t fn){
    using namespace shield_test;calls.push_back(fn);
    const auto arg=[&](unsigned n){return U(c.esp+4+n*4);};
    switch(fn){
    case 0x11100:assert(c.ecx==RENDER);Ret(c,no_skeleton?0:SKELETON);return;
    case 0x11200:assert(c.ecx==SKELETON&&arg(0)==5&&arg(1)==1);Ret(c,REST,8);return;
    case 0x2d57fb:assert(arg(0)==0xa0);Ret(c,no_heap?0:CLONE);return;
    case 0x2a6130:
        assert(c.ecx==CLONE&&arg(0)==WORLD&&arg(1)==SOURCE+0x70&&arg(2)==SOURCE+0x7c);
        ++created;U(CLONE,0x48dd60);U(CLONE+0x88,WORLD);U(CLONE+0x70,LIST);U(CLONE+0x74,12);
        U(CLONE+0x7c,JOINTS);U(CLONE+0x80,11);
        for(unsigned i=0;i<12;++i){U(LIST+i*4,Body(i));U(Body(i)+0x50,Motion(i));U(Motion(i),0x49a8b8);
            U(Body(i)+0x30,0x24800000u|i);F(Motion(i)+0xd0,float(i+1));F(Motion(i)+0xe0,float(i+2));}
        Ret(c,CLONE,12);return;
    case 0x2a3120:{
        const uint32_t body=arg(0);assert(body>=BODIES&&body<Body(12)&&arg(1)==0x18&&arg(2)==77);
        assert(!U(CLONE+0x1c));++properties;Ret(c);return;}
    case 0x2f1880:
        assert(c.ecx==Body(4)&&arg(0)==6&&arg(1)==1&&arg(2)==0&&!U(CLONE+0x1c));
        assert(properties==12);++conversions;U(Motion(4),0x49ae58);Ret(c,0,12);return;
    case 0x2a5f00:
        assert(c.ecx==CLONE&&arg(0)==MODEL&&arg(1)==0&&arg(4)==0&&arg(5)==0&&conversions==1);
        assert(F(arg(3))==0&&F(arg(3)+4)==0&&F(arg(3)+8)==0);
        U(CLONE+0x1c,MODEL);U(CLONE+0x20,SKELETON);B()[CLONE+0x24]=0;Ret(c,0,24);return;
    case 0x2a5da0:
        assert(c.ecx==CLONE&&arg(1));
        if(!B()[CLONE+0x24]){++seeds;assert(arg(0)==0);B()[CLONE+0x24]=1;
            // Start at identity with a nonzero local COM, stressing the native
            // helper's COM rotation rather than only its origin translation.
            F(Motion(4)+0x80,0);F(Motion(4)+0x84,0);F(Motion(4)+0x88,0);F(Motion(4)+0x8c,-1);
            F(Motion(4)+0x90,.15f);F(Motion(4)+0x94,.05f);F(Motion(4)+0x98,-.08f);
            F(Motion(4)+0x60,.15f);F(Motion(4)+0x64,.05f);F(Motion(4)+0x68,-.08f);
        }else ++syncs;
        Ret(c,0,8);return;
    case 0x2a6920:assert(c.ecx==CLONE&&arg(0)==1&&U(SYSTEM+0x10)==WORLD);++destroyed;U(CLONE,0);Ret(c,0,4);return;
    case 0x2d5640:sub_002D5640(c,b);return;
    case 0x2e84a0:assert(arg(3)==Body(4));++keys;sub_002E84A0(c,b);return;
    case 0x2ec5c0:assert(c.ecx==Motion(4));sub_002EC5C0(c,b);return;
    case 0x2ec5e0:assert(c.ecx==Motion(4));sub_002EC5E0(c,b);return;
    default:std::fprintf(stderr,"unexpected native call %08x\n",fn);std::abort();
    }
}
[[noreturn]]void x86_trap(X86Context&,uint32_t,const char* message){throw std::runtime_error(message);}
void x86_resume(X86Context&,uint8_t*,const X86Resume&){std::abort();}
// NATIVE_SHIELD_KEYFRAME

namespace shield_test {
void Seed(){
    ram.fill(0);allocated=freed=created=destroyed=seeds=syncs=keys=properties=conversions=0;
    no_stack=no_heap=no_skeleton=false;calls.clear();
    U(0x51d910,SYSTEM);U(SYSTEM+0x10,WORLD);U(0x51da48,SOURCE);B()[0x51da6c]=1;
    U(SOURCE,0x48dd60);U(SOURCE+0x70,LIST);U(SOURCE+0x74,12);U(SOURCE+0x7c,JOINTS);U(SOURCE+0x80,11);
    U(MODEL,RENDER);U(RENDER,VTABLE);U(VTABLE+0x24,0x11100);U(SKELETON,METHODS);U(METHODS+8,0x11200);
    U(OWNER+0x94,77);U(FS,0x12345678);U(0x49ae58+0x54,0x2ec5c0);U(0x49ae58+0x58,0x2ec5e0);
    const auto swap=Swap();std::memcpy(B()+REST,swap.data(),64);std::memcpy(B()+0x51da80,swap.data(),64);
    // NATIVE_SHIELD_CONSTANTS
}
void Near(float a,float expected,float tolerance=.0001f){
    if(std::abs(a-expected)>tolerance){std::fprintf(stderr,"actual %.9g expected %.9g\n",a,expected);std::abort();}
}
void Lifecycle(){
    for(unsigned fail=0;fail<5;++fail){Seed();auto c=Context();auto world=Identity();TrialCorpseShieldPhysics s;
        if(fail==0)no_stack=true;if(fail==1)no_heap=true;if(fail==2)no_skeleton=true;
        if(fail==3)U(OWNER+0x94,0xffff);if(fail==4)U(SOURCE+0x74,11);
        assert(!TrialCorpseShieldPhysicsCreate(c,B(),s,MODEL,world,OWNER));
        assert(!s.stack&&!s.ragdoll&&allocated==freed&&created==0&&destroyed==0);
    }
    Seed();auto c=Context();const auto original=c;auto world=Identity();TrialCorpseShieldPhysics s;
    assert(TrialCorpseShieldPhysicsCreate(c,B(),s,MODEL,world,OWNER));assert(TrialCorpseShieldPhysicsValid(B(),s));
    assert(seeds==1&&created==1&&properties==12&&conversions==1&&allocated==1&&freed==0);
    assert(std::memcmp(&c,&original,sizeof c)==0&&U(FS)==0x12345678);
    for(unsigned i=0;i<12;++i){assert(U(Body(i)+0x30)==(0x24800000u|i));assert(U(Motion(i))==(i==4?0x49ae58:0x49a8b8));}
    assert(!TrialCorpseShieldPhysicsCreate(c,B(),s,MODEL,world,OWNER));
    const auto count=calls.size();assert(!TrialCorpseShieldPhysicsBefore(c,B(),s,world,0));assert(calls.size()==count);
    U(SYSTEM+0x10,WORLD+0x100);assert(!TrialCorpseShieldPhysicsValid(B(),s));
    assert(!TrialCorpseShieldPhysicsBefore(c,B(),s,world,1.f/60));assert(calls.size()==count);U(SYSTEM+0x10,WORLD);
    TrialCorpseShieldPhysicsDestroy(c,B(),s);assert(!s.ragdoll&&!s.stack&&freed==1&&destroyed==1);
    TrialCorpseShieldPhysicsDestroy(c,B(),s);assert(freed==1&&destroyed==1);
}
void TranslationAndTurn(){
    for(unsigned fps:{30u,60u,120u,240u}){
        Seed();auto c=Context();TrialCorpseShieldPhysics s;auto world=Identity();const float dt=1.f/fps;
        assert(TrialCorpseShieldPhysicsCreate(c,B(),s,MODEL,world,OWNER));
        // Native game Y-up: +50cm X, +20cm Y, -30cm Z -> (+.5,-.3,+.2)m.
        world[12]=50;world[13]=20;world[14]=-30;
        assert(TrialCorpseShieldPhysicsBefore(c,B(),s,world,dt));
        Near(F(Motion(4)+0xd0)*dt,.5f);Near(F(Motion(4)+0xd4)*dt,-.3f);Near(F(Motion(4)+0xd8)*dt,.2f);
        Near(F(Motion(4)+0xe0),0);Near(F(Motion(4)+0xe4),0);Near(F(Motion(4)+0xe8),0);
        assert(TrialCorpseShieldPhysicsAfter(c,B(),s,world,dt));assert(seeds==1&&syncs==1);
        // This row-vector world-Y rotation maps game +X to -Z, hence physics
        // +X to -Y: -90 about Z. COM(.15,.05,-.08) becomes(.05,-.15,-.08).
        world=Identity();world[0]=0;world[2]=-1;world[8]=1;world[10]=0;
        assert(TrialCorpseShieldPhysicsBefore(c,B(),s,world,dt));
        Near(F(Motion(4)+0xd0)*dt,-.1f);Near(F(Motion(4)+0xd4)*dt,-.2f);Near(F(Motion(4)+0xd8)*dt,0);
        Near(F(Motion(4)+0xe0)*dt,0);Near(F(Motion(4)+0xe4)*dt,0);Near(F(Motion(4)+0xe8)*dt,-1.570796327f,.0003f);
        // No backend writes to free head/arms/legs; only shared native solver
        // may change these velocities and their constraints. Post sync cannot reseed.
        for(unsigned i=0;i<12;++i)if(i!=4){Near(F(Motion(i)+0xd0),float(i+1));Near(F(Motion(i)+0xe0),float(i+2));}
        assert(TrialCorpseShieldPhysicsAfter(c,B(),s,world,dt));assert(seeds==1&&syncs==2&&keys==2);
        TrialCorpseShieldPhysicsDestroy(c,B(),s);
    }
}
}
int main(){shield_test::Lifecycle();shield_test::TranslationAndTurn();
    std::puts("PASS native chest keyframe, COM/axes/rotation at30/60/120/240, loose-body isolation, lifecycle contracts");}

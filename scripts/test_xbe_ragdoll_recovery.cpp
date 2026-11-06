#undef NDEBUG
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>
#include "cpu/x86_ops.h"
#include "xbe_ragdoll_recovery.h"
#include "xbe_death_spasm.cpp"
namespace trial_graphics {bool DeathConvulsions(){std::abort();}}
bool TrialCoopImmediateRagdoll(const uint8_t*,uint32_t){std::abort();}
bool TrialCoopKeepDownedBody(const uint8_t*,uint32_t){std::abort();}
alignas(16) static std::array<uint8_t,0xa00000> memory{},initial{},expected{};
constexpr uint32_t DATA=0x600000,CONTEXT=0x610000,ACC=0x620000,TRANSFORM=0x630000,
    CURSOR=0x640000,ROWS=0x650000,OPS=0x660000,RUNTIME=0x670000,CFG=0x690000,IMP=0x691000,
    INSTANCE=0x700000,A=0x701000,BODY_B=0x702000,PROPS=0x703000,CONTACT=0x704000,
    CONTACT_PROPS=0x705000,FLAGS=0x706000,SP=0x9f0000;
uint8_t* B(){return memory.data();}
uint32_t Word(uint32_t p){return trial_ragdoll_recovery::Word(B(),p);}
float Real(uint32_t p){return trial_ragdoll_recovery::Real(B(),p);}
void Put(uint32_t p,uint32_t v){std::memcpy(B()+p,&v,4);}
void Float(uint32_t p,float v){std::memcpy(B()+p,&v,4);}
namespace mem {bool IsCommitted(uint32_t p){return p<memory.size();}}
// NATIVE_DECLARATIONS
X86_FUNC(sub_0034B850){std::abort();} // Diagnostic callback is disabled, not simulated.
void x86_icall(X86Context&,uint8_t*,uint32_t fn){std::fprintf(stderr,"unexpected callback %x\n",fn);std::abort();}
[[noreturn]]void x86_trap(X86Context&,uint32_t pc,const char* why){std::fprintf(stderr,"trap %x %s\n",pc,why);std::abort();}
void x86_resume(X86Context&,uint8_t*,const X86Resume&){std::abort();}
// NATIVE_BODIES
static std::vector<uint8_t> Read(const char* path){
    std::ifstream f(path,std::ios::binary);assert(f);return {std::istreambuf_iterator<char>(f),{}};
}
static uint32_t U32(const uint8_t* p){uint32_t v;std::memcpy(&v,p,4);return v;}
static void LoadImage(const char* path){
    const auto x=Read(path);assert(x.size()>0x180&&U32(x.data())==0x48454258);
    const uint32_t base=U32(x.data()+0x104),count=U32(x.data()+0x11c),table=U32(x.data()+0x120)-base;
    assert(count<100&&uint64_t(table)+count*56<=x.size());
    for(unsigned i=0;i<count;++i){const auto s=x.data()+table+i*56;
        const uint32_t va=U32(s+4),raw=U32(s+12),n=U32(s+16);
        assert(uint64_t(raw)+n<=x.size()&&uint64_t(va)+n<=0x600000);
        std::memcpy(B()+va,x.data()+raw,n);
    }
}
static X86Context Setup(unsigned kind,float fps,unsigned iterations,unsigned side,const uint8_t* authored=nullptr){
    std::memset(B()+DATA,0,memory.size()-DATA);
    Float(CONTEXT,1.f/(fps*iterations));Float(CONTEXT+4,fps*iterations);
    Float(CONTEXT+8,1.f/fps);Float(CONTEXT+12,fps);Float(CONTEXT+16,1.f/iterations);
    Put(CONTEXT+20,ACC);Put(CONTEXT+24,ACC+128);Put(CONTEXT+28,TRANSFORM);Put(CONTEXT+32,TRANSFORM+256);
    Put(CONTEXT+36,INSTANCE);Put(CONTEXT+40,RUNTIME);Put(INSTANCE+16,A);Put(INSTANCE+20,BODY_B);
    if(side<2){const auto body=side?BODY_B:A;Put(body+0x44,PROPS);Put(body+0x48,1);Put(PROPS,1);Put(PROPS+8,4);}
    for(unsigned side=0;side<2;++side){
        const auto a=ACC+side*128,t=TRANSFORM+side*256;
        Float(a+48,.3f);Float(a+52,.5f);Float(a+56,.7f);Float(a+60,.9f);
        Float(a+80,1);Float(a+100,1);Float(a+120,1);Float(t,1);Float(t+20,1);Float(t+40,1);
    }
    if(authored){
        std::memcpy(B()+DATA,authored+4,144);std::memcpy(B()+TRANSFORM,authored+148,64);std::memcpy(B()+TRANSFORM+256,authored+212,64);
        for(unsigned side=0;side<2;++side){const auto a=ACC+side*128,t=TRANSFORM+side*256;
            std::memcpy(B()+a+48,authored+276+side*16,16);std::memcpy(B()+a+64,authored+308+side*16,16);
            for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col)Float(a+80+row*16+col*4,Real(t+col*16+row*4));
        }
        Float(TRANSFORM+48,Real(TRANSFORM+48)+.03f); // Same initial joint displacement at every frame rate.
    }
    Float(CONTACT+24,1);Float(CONTACT+28,-.1f); // Flat contact, 10 cm penetration, no restitution.
    Float(CONTACT_PROPS+4,.05f);
    Put(CURSOR,ROWS);Put(CURSOR+4,OPS);
    B()[0x51f5a8]=B()[0x51f5e5]=B()[ACC-128]=1;B()[ACC+256]=2;B()[0x51c178]=0;
    for(unsigned i=0;i<0x104;i+=4)Float(CFG+i,1);
    Put(CFG+0xfc,iterations);Float(CFG+4,.6f);Float(CFG+8,1);Float(CFG+12,.5f);
    Float(CFG,0); // Keep the isolated contact awake to measure its recovery impulse.
    Float(CFG+16,0);Float(CFG+20,-.01f);Float(CFG+24,0);Float(CFG+28,0);
    X86Context c{};c.ecx=DATA;c.esp=SP;c.ebp=0x1111;c.ebx=0x2222;c.esi=0x3333;c.edi=0x4444;
    Put(SP,0xbeef);
    if(kind==3){Put(SP+4,CONTACT);Put(SP+8,1);Put(SP+12,CONTACT_PROPS);Put(SP+16,FLAGS);Put(SP+20,CONTEXT);Put(SP+24,CURSOR);}
    else{Put(SP+4,CONTEXT);Put(SP+8,CURSOR);}
    return c;
}
static void Build(X86Context& c,unsigned kind,bool fixed){
    const auto before=Word(CONTEXT+4);
    if(kind==1)(fixed?Fixed_0031E840:Original_0031E840)(c,B());
    else if(kind==2)(fixed?Fixed_002F4AF0:Original_002F4AF0)(c,B());
    else (fixed?Fixed_0034A3C0:Original_0034A3C0)(c,B());
    assert(Word(CONTEXT+4)==before);
    assert(c.esp==SP+(kind==3?4:12)&&c.ebp==0x1111&&c.ebx==0x2222&&c.esi==0x3333&&c.edi==0x4444);
}
static void Compare(unsigned kind,float fps,unsigned iterations,unsigned side,const uint8_t* authored){
    auto start=Setup(kind,fps,iterations,side,authored);initial=memory;
    float spasm_angle=0;
    if(kind==2){
        std::memcpy(B()+A+0x10,B()+TRANSFORM,48);std::memcpy(B()+BODY_B+0x10,B()+TRANSFORM+256,48);
        spasm_angle=HingeAngle(B(),A,BODY_B,DATA);initial=memory;
    }
    // Independent reference: original builders with the 60 Hz inverse substep,
    // retaining actual elapsed time, friction, mass, pose and warm-start state.
    if(fps>60&&side<2)Float(CONTEXT+4,60.f/Real(CONTEXT+16));
    auto want=start;Build(want,kind,false);Put(CONTEXT+4,U32(initial.data()+CONTEXT+4));expected=memory;
    // Native atan approximation and the spasm's geometric angle must agree.
    if(kind==2)assert(std::fabs(std::remainder(spasm_angle-Real(RUNTIME+0x38),6.2831853f))<.003f);
    memory=initial;auto got=start;Build(got,kind,true);
    if(memory!=expected){
        for(unsigned p=DATA;p<memory.size();++p)if(memory[p]!=expected[p]){std::fprintf(stderr,"row mismatch kind%u fps%.0f iter%u at%x\n",kind,fps,iterations,p);break;}
        std::abort();
    }
    assert(!std::memcmp(&want,&got,sizeof got));
}
static float ContactVelocity(float fps,bool fixed){
    auto c=Setup(3,fps,1,0);Build(c,3,fixed);Put(Word(CURSOR+4),0x400);
    c={};c.esp=SP;Put(SP+4,CFG);Put(SP+8,OPS);Put(SP+12,ACC-128);Put(SP+16,IMP);
    sub_00348250(c,B());return Real(ACC+24);
}
static void Guards(){
    for(unsigned test=0;test<17;++test){
        auto c=Setup(3,240,4,0);
        switch(test){
            case 0:Float(CONTEXT+8,0);break;case 1:Float(CONTEXT+8,-.01f);break;
            case 2:Put(CONTEXT+8,0x7f800000);break;case 3:Put(CONTEXT+8,0x7fc12345);break;
            case 4:Put(CONTEXT+4,0x7f800000);break;case 5:Float(CONTEXT+4,-1);break;
            case 6:c.esp=0xfffffff0;break;case 7:Put(SP+20,0xfffffffc);break;
            case 8:Put(CONTEXT+36,0xfffffff8);break;case 9:Put(A+0x44,0xfffffff0);break;
            case 10:Put(A+0x48,0x10000000);break;case 11:Put(PROPS+8,3);break;
            case 12:Put(PROPS+12,1);break;case 13:Put(A+0x48,0);break;
            case 14:Float(CONTEXT+16,0);break;case 15:Put(CONTEXT+16,0x7fc12345);break;
            case 16:Float(CONTEXT+16,2);break;
        }
        initial=memory;const auto before=c;_mm_setcsr(0x1fa1);
        {trial_ragdoll_recovery::Scope guard(c,B(),20);assert(memory==initial);}
        assert(memory==initial&&!std::memcmp(&c,&before,sizeof c)&&_mm_getcsr()==0x1fa1);
    }
    auto c=Setup(3,240,4,1);initial=memory;_mm_setcsr(0x1fa1);
    {trial_ragdoll_recovery::Scope guard(c,B(),20);assert(Real(CONTEXT+4)==240&&_mm_getcsr()==0x1fa1);
        expected=initial;Float(CONTEXT+4,960);assert(memory==expected);Float(CONTEXT+4,240);}
    assert(memory==initial&&_mm_getcsr()==0x1fa1);_mm_setcsr(0x1f80);
}
int main(int argc,char** argv){
    assert(argc==3);LoadImage(argv[1]);const auto authored=Read(argv[2]);assert(authored.size()>=4);
    const unsigned count=U32(authored.data());assert(count>0&&count<128&&authored.size()==4+count*340);
    Guards();unsigned pairs=0;
    for(float fps:{30.f,60.f,120.f,144.f,240.f,360.f})for(unsigned steps:{1u,3u,4u,7u})for(unsigned side:{0u,1u,2u}){
        Compare(3,fps,steps,side,nullptr);++pairs;
        for(unsigned j=0;j<count;++j){const auto record=authored.data()+4+j*340;const auto kind=U32(record);assert(kind==1||kind==2);
            Compare(kind,fps,steps,side,record);++pairs;
        }
    }
    const float reference=ContactVelocity(60,false);assert(reference>0);
    for(float fps:{60.f,120.f,144.f,240.f,360.f}){
        const float original=ContactVelocity(fps,false),fixed=ContactVelocity(fps,true);
        assert(std::fabs(fixed-reference)<1.e-5f&&std::fabs(original/reference-fps/60)<1.e-4f);
        std::printf("%.0f FPS contact: original %.6f, corrected %.6f, reference %.6f m/s\n",fps,original,fixed,reference);
    }
    std::printf("ragdoll recovery: %u original-row full-state pairs across %u authored joints, guards and original contact solver passed. Live collision generation remains separate.\n",pairs,count);
}

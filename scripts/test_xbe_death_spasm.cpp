#undef NDEBUG
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <vector>
#include "xbe_death_spasm.cpp"
alignas(16) static std::array<uint8_t,0x800000> memory{};
constexpr uint32_t R=0x600000,A=0x601000,LIST=0x602000,JOINTS=0x603000,
    BODIES=0x610000,MOTIONS=0x620000,DATA=0x630000,INSTANCES=0x640000,SP=0x780000,BP=SP+0x100;
bool enemy=true,downed=false;unsigned wakes=0,impulses=0;
bool enabled=true;
unsigned touched[12]={};
namespace trial_graphics {bool DeathConvulsions(){return enabled;}}
float maximum_momentum_error=0;
namespace mem {bool IsCommitted(uint32_t p){return p<memory.size();}}
bool TrialCoopImmediateRagdoll(const uint8_t*,uint32_t a){return downed&&a==A;}
bool TrialCoopKeepDownedBody(const uint8_t*,uint32_t a){return downed&&a==A;}
uint8_t* B(){return memory.data();}
void U(uint32_t p,uint32_t v){Put(B(),p,v);}
void F(uint32_t p,float v){std::memcpy(B()+p,&v,4);}
void Bend(unsigned bone,float angle){
    const auto m=MOTIONS+bone*0x100;const float cs=std::cos(angle),sn=std::sin(angle);
    F(m+0x10,cs);F(m+0x14,-sn);F(m+0x20,sn);F(m+0x24,cs);
}
X86_FUNC(sub_002F5240);X86_FUNC(sub_0020A690);X86_FUNC(sub_002ECAA0);
X86_FUNC(sub_002F6CC0){std::abort();} // The fixture's islands start awake.
void x86_icall(X86Context& c,uint8_t* b,uint32_t fn){
    if(fn==0x2f5240){++impulses;assert(c.ecx>=MOTIONS&&c.ecx<MOTIONS+12*0x100);++touched[(c.ecx-MOTIONS)/0x100];sub_002F5240(c,b);return;}
    if(fn==0x20a690){sub_0020A690(c,b);return;}
    if(fn==0x2ecaa0){++wakes;sub_002ECAA0(c,b);return;}
    if(fn==0xf001){
        assert(c.ecx==A&&Word(b,c.esp+4)==0x44da54);
        const auto value=Word(b,c.esp+8);b[value+4]=enemy;c.eax=0xffffff01;c.esp+=12;return;
    }
    std::abort();
}
[[noreturn]]void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
void x86_resume(X86Context&,uint8_t*,const X86Resume&){std::abort();}
// NATIVE_BODIES
X86Context Setup(bool coop=false){
    memory.fill(0);reactions={};active=wakes=impulses=0;maximum_momentum_error=0;enemy=enabled=true;downed=coop;
    std::fill(std::begin(touched),std::end(touched),0);
    U(R,0x48dd60);U(R+0x1c,0x650000);U(R+0x20,0x650100);U(R+0x70,LIST);U(R+0x74,12);
    U(R+0x7c,JOINTS);U(R+0x80,11);U(0x51c4dc,0x660000);
    U(A,0x45c770);U(A+0x54,42);U(0x45c778,0xf001);F(A+0x9c,0);
    U(A+0x114,0x680000);
    for(unsigned i=0;i<12;++i){
        const auto body=BODIES+i*0x100,m=MOTIONS+i*0x100;
        U(LIST+i*4,body);U(body+0x50,m);U(body+0x54,0x670000);B()[0x670028]=1;U(m,0x49a8b8);
        F(m+0x10,1);F(m+0x24,1);F(m+0x38,1);
        // Different inertias independently exercise conservation on each side.
        for(unsigned axis=0;axis<3;++axis){F(m+0xf0+axis*4,.25f+.05f*i);F(m+0xd0+axis*4,.4f);}
    }
    constexpr unsigned pair[4][2]={{5,2},{6,3},{10,9},{11,7}};
    for(unsigned i=0;i<4;++i){const auto j=INSTANCES+i*0x100,d=DATA+i*0x100;
        U(JOINTS+i*4,j);U(j+12,d);U(j+16,BODIES+pair[i][0]*0x100);U(j+20,BODIES+pair[i][1]*0x100);
        U(d,0x49a9d4);F(d+12,.174533f);F(d+16,1.745329f);
        F(d+0x38,-1);F(d+0x44,1);F(d+0x78,-1);F(d+0x80,1);Bend(pair[i][0],.4f);
    }
    X86Context c{};c.esi=R;c.ecx=R;c.esp=SP;c.ebp=BP;c.edi=0x123;c.eax=0x456;
    U(BP+8,0x650000);U(BP+12,A);return c;
}
void Activate(X86Context& c){
    const auto before=c;const auto csr=_mm_getcsr();TrialDeathSpasmActivated(c,B());
    assert(!std::memcmp(&c,&before,sizeof c)&&csr==_mm_getcsr());
}
void Step(X86Context& c,float dt){
    std::memcpy(&c.ebx,&dt,4);const auto before=c;const auto csr=_mm_getcsr();TrialDeathSpasmStep(c,B());
    assert(!std::memcmp(&c,&before,sizeof c)&&csr==_mm_getcsr());
    for(unsigned i=0;i<12;++i)for(unsigned axis=0;axis<3;++axis){
        assert(Real(B(),MOTIONS+i*0x100+0xd0+axis*4)==.4f);
    }
    Vector momentum{};
    for(unsigned i=0;i<12;++i){const auto m=MOTIONS+i*0x100;
        for(unsigned col=0;col<3;++col){
            float local=0;for(unsigned row=0;row<3;++row)local+=Real(B(),m+0x10+col*16+row*4)*Real(B(),m+0xe0+row*4);
            local/=Real(B(),m+0xf0+col*4);
            for(unsigned row=0;row<3;++row)momentum[row]+=Real(B(),m+0x10+col*16+row*4)*local;
        }
    }
    for(float value:momentum){
        maximum_momentum_error=std::max(maximum_momentum_error,std::abs(value));
        assert(std::isfinite(value)&&std::abs(value)<.001f);
    }
}
int main(int argc,char** argv){
    assert(argc==2);std::ifstream f(argv[1],std::ios::binary);assert(f);
    const std::vector<uint8_t> authored{std::istreambuf_iterator<char>(f),{}};
    assert(authored.size()==4+11*340&&Word(authored.data(),0)==11);
    // Real native angular-impulse/query/wake routines; world contact solving is
    // covered separately by test_ragdoll_recovery, not simulated here.
    for(bool coop:{false,true})for(uint32_t actor:{42u,77u,1234u})for(float start:{.4f,1.725329f})for(float fps:{30.f,60.f,120.f,240.f,360.f}){
        auto c=Setup(coop);U(A+0x54,actor);Activate(c);assert(active==1);float t=0,peak=0;
        float angles[12]={};for(unsigned i:{5u,6u,10u,11u}){angles[i]=start;Bend(i,start);}
        unsigned reversals=0;float previous_velocity=0;
        // Isolated hinges integrate the real impulse routine, not world contacts.
        while(t<5.4f){const float dt=std::min(1.f/fps,5.4f-t);Step(c,dt);t+=dt;
            for(unsigned i=0;i<12;++i){angles[i]-=Real(B(),MOTIONS+i*0x100+0xe8)*dt;Bend(i,angles[i]);}
            const float bend=HingeAngle(B(),MOTIONS+11*0x100,MOTIONS+7*0x100,DATA+3*0x100);
            assert(std::fabs(bend-(angles[11]-angles[7]))<.00001f);
            if(!(bend>.15f&&bend<1.77f))std::fprintf(stderr,"Joint overshoot: actor=%u start=%f fps=%f time=%f angle=%f\n",actor,start,fps,t,bend);
            assert(bend>.15f&&bend<1.77f);
            const float displacement=std::abs(bend-start);peak=std::max(peak,displacement);
            const float velocity=Real(B(),MOTIONS+11*0x100+0xe8)-Real(B(),MOTIONS+7*0x100+0xe8);
            if(std::abs(velocity)>.5f){if(previous_velocity*velocity<0)++reversals;previous_velocity=velocity;}
        }
        assert(!active&&wakes==impulses&&impulses>0);
        std::printf("downed=%d actor=%u start=%.3f fps=%.0f peak=%.3f reversals=%u momentum_error=%.7f\n",coop,actor,start,fps,peak,reversals,maximum_momentum_error);std::fflush(stdout);
        assert(peak>.05f&&peak<.585f&&reversals>=10); // 30% stronger travel, same joint bounds.
        for(unsigned i:{0u,1u,4u,8u})assert(touched[i]==0); // Isolated four-hinge fixture.
        const auto count=impulses;Step(c,.2f);assert(impulses==count);
    }
    // Real twelve-body topology, reflected bases and anisotropic inertia. Poses
    // stay fixed here; this checks injection, not a simulated contact world.
    constexpr unsigned pairs[11][2]={{1,0},{2,0},{3,0},{4,1},{5,2},{6,3},{7,4},{8,4},{9,4},{10,9},{11,7}};
    for(bool coop:{false,true})for(float fps:{30.f,60.f,120.f,240.f,360.f}){
        auto c=Setup(coop);
        for(unsigned i=0;i<11;++i){const auto record=authored.data()+4+i*340;const auto j=INSTANCES+i*0x100,d=DATA+i*0x100;
            const auto kind=Word(record,0);assert(kind==1||kind==2);
            U(JOINTS+i*4,j);U(j+12,d);U(j+16,BODIES+pairs[i][0]*0x100);U(j+20,BODIES+pairs[i][1]*0x100);
            std::memcpy(B()+d,record+4,144);U(d,kind==1?0x49de88:0x49a9d4);
            for(unsigned side=0;side<2;++side){const auto m=MOTIONS+pairs[i][side]*0x100;
                std::memcpy(B()+m+0x10,record+148+side*64,48);
                std::memcpy(B()+m+0xf0,record+276+side*16,16);
            }
        }
        Activate(c);float peak[12]={},head_min=0,head_max=0,previous_head=0;unsigned head_reversals=0;
        for(float t=0;t<5.4f;){const float dt=std::min(1.f/fps,5.4f-t);Step(c,dt);t+=dt;
            for(unsigned i=0;i<12;++i)for(unsigned axis=0;axis<3;++axis){
                const float v=Real(B(),MOTIONS+i*0x100+0xe0+axis*4);assert(std::isfinite(v)&&std::abs(v)<30);
                peak[i]=std::max(peak[i],std::abs(v));
            }
            const auto axis=Rotate(B(),MOTIONS+8*0x100,DATA+7*0x100+0x20);
            float head=0;for(unsigned row=0;row<3;++row)head+=axis[row]*Real(B(),MOTIONS+8*0x100+0xe0+row*4);
            head_min=std::min(head_min,head);head_max=std::max(head_max,head);
            if(std::abs(head)>.75f){if(head*previous_head<0)++head_reversals;previous_head=head;}
        }
        assert(!active&&impulses==wakes);
        for(unsigned i=0;i<12;++i)assert(touched[i]>0&&peak[i]>.1f);
        assert(head_min<-1.f&&head_max>1.f&&head_reversals>=10);
        std::printf("authored whole body downed=%d fps=%.0f: all 12 bodies active; head=[%.3f,%.3f] reversals=%u momentum_error=%.7f\n",coop,fps,head_min,head_max,head_reversals,maximum_momentum_error);
    }
    // Every body gets three short groups, with quiet gaps shared by all joints.
    float body_difference=0,limb_difference=0;
    for(unsigned i=0;i<=5400;++i){const float t=float(i)/1000;
        const float a=Pulse(t,42,0),b=Pulse(t,77,0),c=Pulse(t,42,1);
        assert(a>=-1&&a<=1&&a==Pulse(t,42,0));
        body_difference+=std::abs(a-b);limb_difference+=std::abs(a-c);
        if((t>1.31f&&t<2.05f)||(t>2.76f&&t<3.5f)||t>=4.21f)assert(a==0&&b==0&&c==0);
    }
    assert(body_difference>100&&limb_difference>100);
    auto c=Setup();enabled=false;Activate(c);assert(!active&&!impulses);
    c=Setup();Activate(c);Step(c,.8f);const auto previous=impulses;
    enabled=false;Step(c,.1f);assert(!active&&impulses==previous);
    enabled=true;Step(c,.1f);assert(!active&&impulses==previous); // No restart of an expired reaction.
    Activate(c);assert(active); // Later deaths can react again.
    c=Setup();enemy=false;Activate(c);assert(!active);
    c=Setup();U(A+0x114,0);Activate(c);assert(!active); // Retired NPC script owner.
    c=Setup();F(A+0x9c,1);Activate(c);assert(!active); // Live/injured/KO actor.
    c=Setup();F(A+0x9c,std::numeric_limits<float>::quiet_NaN());Activate(c);assert(!active);
    c=Setup(true);Activate(c);Step(c,.8f);downed=false;Step(c,.01f);assert(!active); // Revived.
    c=Setup();Activate(c);U(A+0x54,43);Step(c,.2f);assert(!active&&!impulses); // Reused actor.
    c=Setup();Activate(c);U(BP+8,0);Activate(c);assert(!active); // Native pool release.
    c=Setup();Activate(c);U(0x51c4dc,0x660100);Step(c,.2f);assert(!active&&!impulses);
    c=Setup();Activate(c);Step(c,0);Step(c,-1);Step(c,std::numeric_limits<float>::infinity());assert(active&&!impulses);
    U(R+0x74,1);Step(c,.2f);assert(!impulses); // Unknown/nonhuman rig.
    c=Setup();Activate(c);Step(c,2);assert(!active&&!impulses); // Long loading hitch.
    c=Setup();Activate(c);Step(c,.6f);Step(c,.05f);
    const float velocity=Real(B(),MOTIONS+11*0x100+0xe8)-Real(B(),MOTIONS+7*0x100+0xe8);
    assert(std::abs(velocity)<=338.f*.05f+.00001f); // Bounded relative acceleration.
    std::puts("Death spasm: small fast hinge contractions and whole-body bursts, native paired impulses, 30-360 FPS, toggle/eligibility/revive/reuse/expiry, CPU/FP and momentum guards passed");
}

// Executes the original named-head getter/axis conversion. Bone resource lookup
// is a controlled boundary; live ragdoll motion and clipping need visual testing.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include <limits>
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x800000> memory{};}
namespace mem {bool IsCommitted(uint32_t p){return p<kernel::memory.size();}}
#include "../common/xbe_coop_downed_camera.cpp"
using coop_downed_camera::Matrix;
using coop_downed_camera::Multiply;
using coop_downed_camera::Inverse;
using coop_downed_camera::Word;
using coop_downed_camera::Load;
static constexpr uint32_t players[]={0x610000,0x612000},bodies[]={0x630000,0x632000},cameras[]={0x650000,0x652000};
static uint32_t current_bodies[2];
static bool downed[2],missing_head=false;
static unsigned queries=0;
uint8_t* B(){return kernel::memory.data();}
void Put(uint32_t p,uint32_t v){std::memcpy(B()+p,&v,4);}
void Store(uint32_t p,const Matrix& m){std::memcpy(B()+p,m.v,sizeof m.v);}
Matrix Identity(){Matrix m;for(unsigned i=0;i<4;++i)m.v[i*5]=1;return m;}
Matrix Pose(float x,float y,float z){auto m=Identity();m.v[12]=x;m.v[13]=y;m.v[14]=z;return m;}
Matrix Swap(){auto m=Identity();m.v[5]=m.v[10]=0;m.v[6]=m.v[9]=1;return m;}
void Same(const Matrix& a,const Matrix& b){for(unsigned i=0;i<16;++i)assert(std::fabs(a.v[i]-b.v[i])<.005f);}
uint32_t TrialCoopPlayer(unsigned slot){return slot<2?players[slot]:0;}
uint32_t TrialCoopBodyForPlayer(const uint8_t*,uint32_t owner){for(unsigned i=0;i<2;++i)if(owner==players[i])return current_bodies[i];return 0;}
bool TrialCoopPlayerDowned(const uint8_t*,uint32_t owner){for(unsigned i=0;i<2;++i)if(owner==players[i])return downed[i];return false;}
X86_FUNC(sub_0021AFB0);X86_FUNC(sub_0021AFD0);X86_FUNC(sub_0021B0D0);
// ORIGINAL_0020B950
// ORIGINAL_0021AFB0
// ORIGINAL_0021AFD0
// ORIGINAL_0021B0D0
// ORIGINAL_000B5EC0
X86_FUNC(sub_0021AFB0){__imp__sub_0021AFB0(ctx,base);}
X86_FUNC(sub_0021AFD0){__imp__sub_0021AFD0(ctx,base);}
X86_FUNC(sub_0021B0D0){__imp__sub_0021B0D0(ctx,base);}
void x86_icall(X86Context& c,uint8_t* b,uint32_t fn){
    if(fn==0xb5ec0){__imp__sub_000B5EC0(c,b);return;}
    const unsigned slot=c.ecx==0x670008?0:1;assert(c.ecx==0x670008+slot*0x1000);
    if(fn==0x700001){
        assert(Word(b,c.esp+4)==0x45c6dc);++queries;c.eax=missing_head?0xffffffffu:7;c.esp+=8;return;
    }
    assert(fn==0x700002&&Word(b,c.esp+4)==7&&Word(b,c.esp+8)==1);
    c.eax=0x680000+slot*0x1000;c.esp+=12;
}
namespace kernel {
uint32_t CallGuest(X86Context& c,uint32_t fn,const uint32_t* args,size_t n){
    assert(fn==0x20b950&&n==2);
    // Match production: the return carries guest EAX, while the caller's GPRs
    // are restored. Leaving EAX changed would conceal a stale-result caller.
    const std::array<uint32_t,8> saved{c.eax,c.ecx,c.edx,c.ebx,c.esi,c.edi,c.ebp,c.esp};
    for(size_t i=n;i;--i){c.esp-=4;Put(c.esp,args[i-1]);}c.esp-=4;Put(c.esp,0);
    __imp__sub_0020B950(c,B());assert(c.esp==saved[7]);const uint32_t result=c.eax;
    c.eax=saved[0];c.ecx=saved[1];c.edx=saved[2];c.ebx=saved[3];
    c.esi=saved[4];c.edi=saved[5];c.ebp=saved[6];c.esp=saved[7];return result;
}
}
void Setup(){
    kernel::memory.fill(0);trial_coop::Reset();trial_coop::Request();trial_coop::SetActive(true);
    for(auto& s:coop_downed_camera::states)s={};missing_head=false;queries=0;
    Put(0x51c4dc,0x600000);Put(0x457880+0x24,0xb5ec0);
    Put(0x690008,0x700002);Put(0x690010,0x700001);Put(0x44bf94,0x3f800000);
    for(unsigned i=0;i<2;++i){
        downed[i]=false;current_bodies[i]=bodies[i];
        Put(players[i],0x46fbe8);Put(players[i]+0x54,100+i);Put(players[i]+0x368,i);Put(players[i]+0x308,300+i);
        Put(bodies[i],0x45d040);Put(bodies[i]+0x54,200+i);Put(bodies[i]+0x58,100+i);
        Put(bodies[i]+0x74,0x660000+i*0x1000);Put(0x660000+i*0x1000,0x670000+i*0x1000);
        Put(0x670000+i*0x1000,0x457880);Put(0x670008+i*0x1000,0x690000);
        Store(bodies[i]+0x10,Pose(300+200*i,0,400));
        // 20B950 pre-multiplies the skeleton pose by the retail Y/Z conversion.
        Store(0x680000+i*0x1000,Multiply(Swap(),Pose(0,175,0)));
        Store(players[i]+0x6b0,Pose(300+200*i,180,410));
        Put(cameras[i],0x47c780);Put(cameras[i]+0x54,300+i);Put(cameras[i]+0x58,100+i);
        Put(cameras[i]+0x22c,100+i);Put(cameras[i]+0x210,2);
        Put(cameras[i]+0xe0,0x42aa0000);Put(cameras[i]+0x228,0x42540000);
        Store(cameras[i]+0x10,Pose(-100,-200,-300));
    }
}
X86Context Context(unsigned i){X86Context c{};c.esp=0x7e0000;c.ecx=players[i];c.esi=cameras[i];c.eax=0x12345678;return c;}
void Capture(unsigned i){auto c=Context(i),before=c;TrialCoopDownedCameraCapture(c,B());assert(std::memcmp(&c,&before,sizeof c)==0);}
void Update(unsigned i){auto c=Context(i),before=c;TrialCoopDownedCameraUpdate(c,B());assert(std::memcmp(&c,&before,sizeof c)==0);}
int main(){
    // Head success must come from CallGuest's return, never the restored EAX.
    // Exercise both contradictory incoming values against the original getter.
    for(bool absent:{false,true}){
        Setup();missing_head=absent;auto c=Context(0);c.eax=absent?1:0;
        const auto before=c;Matrix head;
        assert(coop_downed_camera::Head(c,B(),bodies[0],head)==!absent);
        assert(std::memcmp(&c,&before,sizeof c)==0&&queries==1);
        if(!absent)Same(head,Pose(300,175,400));
    }
    Setup();Capture(0);Capture(1);assert(queries==2);
    for(unsigned i=0;i<2;++i){
        assert(coop_downed_camera::states[i].valid);downed[i]=true;Put(players[i]+0x170,12);
        auto head=Pose(10,25,30);head.v[0]=0;head.v[1]=1;head.v[4]=-1;head.v[5]=0;
        Store(0x680000+i*0x1000,Multiply(Swap(),head));
        const auto expected=Multiply(Pose(0,5,10),Multiply(head,Load(B(),bodies[i]+0x10)));
        const auto other=Load(B(),cameras[1-i]+0x10);Update(i);
        Same(Load(B(),cameras[i]+0x10),expected);Same(Load(B(),cameras[1-i]+0x10),other);
        assert(Word(B(),cameras[i]+0xe0)==0x42aa0000&&Word(B(),cameras[i]+0x228)==0x42540000);
    }
    const auto last=Load(B(),cameras[0]+0x10);missing_head=true;Store(cameras[0]+0x10,Identity());Update(0);Same(Load(B(),cameras[0]+0x10),last);
    missing_head=false;Put(0x680000,0x7fc00000);Update(0);Same(Load(B(),cameras[0]+0x10),last);
    downed[0]=false;Store(cameras[0]+0x10,Identity());Update(0);Same(Load(B(),cameras[0]+0x10),Identity());assert(!coop_downed_camera::states[0].valid);
    // A revived/replaced body or recycled world cannot inherit the old head link.
    for(unsigned variant=0;variant<4;++variant){
        Setup();Capture(0);downed[0]=true;
        if(variant==0)current_bodies[0]=bodies[1];else if(variant==1)Put(bodies[0]+0x54,999);
        else if(variant==2)Put(0x51c4dc,0x600100);else Put(players[0]+0x54,999);
        auto before=Load(B(),cameras[0]+0x10);Update(0);Same(Load(B(),cameras[0]+0x10),before);assert(!coop_downed_camera::states[0].valid);
    }
    Setup();Capture(0);downed[0]=true;Put(cameras[0]+0x22c,101);auto before=Load(B(),cameras[0]+0x10);Update(0);Same(Load(B(),cameras[0]+0x10),before);
    Setup();Capture(0);downed[0]=true;Put(cameras[0],0x46fd70);before=Load(B(),cameras[0]+0x10);Update(0);Same(Load(B(),cameras[0]+0x10),before);
    // Mission construction explicitly clears calibration even with exact ID/address reuse.
    Setup();Capture(0);Capture(1);downed[0]=downed[1]=true;TrialCoopDownedCameraReset();
    for(unsigned i=0;i<2;++i){before=Load(B(),cameras[i]+0x10);Update(i);Same(Load(B(),cameras[i]+0x10),before);assert(!coop_downed_camera::states[i].valid);}
    Setup();missing_head=true;Capture(0);assert(!coop_downed_camera::states[0].valid);
    Setup();Store(players[0]+0x6b0,Pose(9999,9999,9999));Capture(0);assert(!coop_downed_camera::states[0].valid);
    // Noncommuting head/body rotations prove local * world and calibration order.
    Setup();auto body=Pose(400,20,600);body.v[0]=0;body.v[2]=-1;body.v[8]=1;body.v[10]=0;
    Store(bodies[0]+0x10,body);Store(players[0]+0x6b0,Multiply(Pose(0,5,10),Multiply(Pose(0,175,0),body)));
    Capture(0);assert(coop_downed_camera::states[0].valid);downed[0]=true;
    auto head=Pose(15,30,25);head.v[0]=0;head.v[1]=1;head.v[4]=-1;head.v[5]=0;
    Store(0x680000,Multiply(Swap(),head));Update(0);
    Same(Load(B(),cameras[0]+0x10),Multiply(Pose(0,5,10),Multiply(head,body)));
    Setup();trial_coop::SetActive(false);Capture(0);Update(0);assert(queries==0);
    std::puts("co-op downed head camera contracts passed");
}

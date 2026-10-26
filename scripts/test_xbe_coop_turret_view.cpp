// Native hierarchy and bone lookup execute; GPU image quality remains live QA.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cmath>
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x800000> memory{};}
namespace mem {bool IsCommitted(uint32_t p){return p<kernel::memory.size();}}
#include "../common/xbe_coop_turret_view.cpp"
namespace test {
constexpr uint32_t owners[]={0x610000,0x612000},heli=0x620000;
constexpr uint32_t wrappers[]={0x630000,0x630100,0x630200};
constexpr uint32_t renders[]={0x640000,0x641000,0x642000};
constexpr uint32_t models[]={0x650000,0x650100,0x650200};
constexpr uint32_t bones[]={0x660000,0x661000,0x662000};
constexpr uint32_t locals[]={0x670000,0x671000,0x672000};
constexpr uint32_t worlds[]={0x680000,0x681000,0x682000};
constexpr uint32_t world=0x700000,state=0x701000,layers=0x702000,stack=0x7f0000;
bool mounted[2]={true,true};unsigned refreshes=0;bool throwing=false;
uint8_t* Bytes(){return kernel::memory.data();}
void Write(uint32_t p,uint32_t v){Put(Bytes(),p,v);}
uint32_t Read(uint32_t p){return Word(Bytes(),p);}
Matrix Identity(float x=0,float y=0,float z=0){return {1,0,0,0,0,1,0,0,0,0,1,0,x,y,z,1};}
void MatrixAt(uint32_t p,const Matrix& m){std::memcpy(Bytes()+p,m.data(),64);}
Matrix MatrixAt(uint32_t p){Matrix m{};std::memcpy(m.data(),Bytes()+p,64);return m;}
void Near(const Matrix& a,const Matrix& b){for(unsigned i=0;i<16;++i)assert(std::abs(a[i]-b[i])<.002f);}
void Bone(unsigned model,unsigned index,const char* name,unsigned parent,const Matrix& pose){
    auto p=bones[model]+index*0xa0;std::strcpy(reinterpret_cast<char*>(Bytes()+p+4),name);
    uint16_t value=uint16_t(parent);std::memcpy(Bytes()+p+0x16,&value,2);MatrixAt(locals[model]+index*64,pose);
}
X86Context Setup(unsigned slot=0){
    kernel::memory.fill(0);trial_coop::Reset();trial_coop::SetAvailable(true);
    assert(trial_coop::Request());trial_coop::SetActive(true);mounted[0]=mounted[1]=true;refreshes=0;throwing=false;
    for(unsigned i=0;i<3;++i){
        const uint32_t entity=i<2?owners[i]:heli;
        Write(entity,i<2?0x46fbe8:0x4753a0);Write(entity+0x54,100+i);Write(entity+0x74,wrappers[i]);
        Write(wrappers[i],renders[i]);Write(renders[i],0x457880);Write(renders[i]+8,0x457830);
        Write(renders[i]+0x60,0x1b4);Bytes()[renders[i]+0x66]=0;
        Write(renders[i]+0x68,models[i]);Write(renders[i]+0x6c,locals[i]);Write(renders[i]+0x70,worlds[i]);
        Write(models[i]+0xc,4);Write(models[i]+0x40,bones[i]);
        Bone(i,0,"SceneRoot",0,Identity(10,20,30));Bone(i,1,"Parent",0,Identity(1,2,3));
        Bone(i,2,i<2?"MachinePistol":"Chopper-Body",1,Identity(-120,-5,123));
        Bone(i,3,"DummyFlare",2,Identity(0,-140,0));
        MatrixAt(entity+0x10,Identity(1000,2000,3000));MatrixAt(entity+0x6f0,Identity(8,9,10));
        if(i==2)continue;
        Write(entity+0x368,i);Write(entity+0x4fc,25);Write(entity+0x418,3);
        const uint32_t camera=0x710000+i*0x1000,layer=0x720000+i*0x1000,cs=0x730000+i*0x1000;
        Write(entity+0x308,200+i);Write(camera,0x47c780);Write(camera+0xd0,0x478d20);Write(camera+0x54,200+i);Write(camera+0x58,100+i);
        Write(camera+0x210,2);Write(camera+0x22c,100+i);Write(layers+i*4,layer);
        Write(layer+4,entity);Write(layer+0x24,i);Write(layer+8,cs);Write(cs+8,1);
        Write(cs+4,cs+0x10);Write(cs+0x14,cs+0x20);Write(cs+0x28,camera);
    }
    Write(0x51c4dc,world);Write(world+4,state);Write(state+4,layers);Write(state+8,layers+8);
    Write(0x457880+0x24,0xb5ec0);Write(0x457830+8,0xb5570);
    X86Context c{};c.esp=stack;c.esi=owners[slot];c.edi=c.esi+0x6f0;c.ebx=world;
    c.eax=123;c.ecx=456;c.edx=789;c.ebp=0x7e0000;c.cf=1;c.pf=1;c.zf=1;c.sf=1;c.of=1;c.df=1;c.xflags=0x200000;
    c.fcw=0x37f;c.fsw=0x1234;c.mxcsr=0x1f80;c.top=2;
    for(unsigned i=0;i<8;++i){c.st[i]=i+.125;c.mm[i]=0x1234567800000000ull+i;c.xmm[i].q[0]=i+1;c.xmm[i].q[1]=i+9;}
    return c;
}
void Invoke(X86Context& c,void(*fn)(X86Context&,uint8_t*),const uint32_t* args,unsigned count){
    for(unsigned i=count;i>0;--i){c.esp-=4;Write(c.esp,args[i-1]);}c.esp-=4;Write(c.esp,0);fn(c,Bytes());
}
}
uint32_t TrialCoopPlayer(unsigned i){return i<2?test::owners[i]:0;}
uint32_t TrialCoopTurretMount(const uint8_t*,uint32_t p){for(unsigned i=0;i<2;++i)if(p==test::owners[i]&&test::mounted[i])return test::heli;return 0;}
[[noreturn]]void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
X86_FUNC(sub_0021AFD0);
// ORIGINAL_TURRET_FUNCTIONS
X86_FUNC(sub_0021AFD0){__imp__sub_0021AFD0(ctx,base);}
X86_FUNC(sub_0020A2D0){__imp__sub_0020A2D0(ctx,base);}
void x86_icall(X86Context& c,uint8_t* b,uint32_t target){
    if(target==0xb5ec0)__imp__sub_000B5EC0(c,b);
    else if(target==0xb5570)__imp__sub_000B5570(c,b);
    else assert(false);
}
// ORIGINAL_TURRET_MUZZLE_SELECTOR
namespace kernel {
uint32_t CallGuest(X86Context& c,uint32_t target,const uint32_t* args,size_t count){
    using namespace test;assert(target==0xb8b60&&count==2);++refreshes;
    Invoke(c,__imp__sub_000B8B60,args,unsigned(count));
    // Retained calls may clobber every guest/host arithmetic register.
    c.eax=1;c.ebx=2;c.ecx=3;c.edx=4;c.esi=5;c.edi=6;c.ebp=7;c.esp-=80;
    c.cf^=1;c.pf^=1;c.zf^=1;c.sf^=1;c.of^=1;c.df^=1;c.xflags^=0x200000;
    c.fcw=0x27f;c.fsw=0x4321;c.mxcsr=0x3f80;c.top=3;
    for(unsigned i=0;i<8;++i){c.st[i]=i+.25;c.mm[i]^=0x12340000;c.xmm[i].q[0]+=7;c.xmm[i].q[1]+=9;}
    _mm_setcsr(0x3f80);if(throwing)throw 17;return 0;
}
}
int main(){
    using namespace test;
    for(unsigned slot=0;slot<2;++slot)for(unsigned phase=0;phase<3;++phase){
        auto c=Setup(slot);const uint32_t owner=owners[slot],render=renders[slot];
        // Distinct changing poses ensure this is not a bind-pose/ordinal fix.
        Matrix parent=Identity(11+phase*7,22,33);parent[0]=0;parent[1]=1;parent[4]=-1;parent[5]=0;
        MatrixAt(locals[slot]+64,parent);MatrixAt(locals[slot]+3*64,Identity(phase*2.f,-140,phase*3.f));
        Matrix world_matrix=Identity(50,60,70);world_matrix[0]=0;world_matrix[1]=-1;world_matrix[4]=1;world_matrix[5]=0;
        MatrixAt(owner+0x6f0,world_matrix);
        auto native=c;native.ecx=render;const uint32_t args[]={worlds[slot],owner+0x6f0};
        Invoke(native,__imp__sub_000B8B60,args,2);const Matrix old_muzzle=MatrixAt(worlds[slot]+3*64);
        const auto saved=c;std::array<uint8_t,0x200> scratch_before{};
        std::memcpy(scratch_before.data(),Bytes()+stack-0x200,scratch_before.size());
        const unsigned host=_mm_getcsr();_mm_setcsr(0x5f80);
        TrialCoopTurretWeaponMatrix(c,Bytes());assert(_mm_getcsr()==0x5f80);_mm_setcsr(host);
        assert(!std::memcmp(&c,&saved,sizeof c)&&refreshes==1);
        assert(!std::memcmp(scratch_before.data(),Bytes()+stack-0x200,scratch_before.size()));
        const auto corrected=MatrixAt(owner+0x6f0);Near(MatrixAt(render+0x20),corrected);
        for(unsigned row=0;row<3;++row)for(unsigned axis=0;axis<3;++axis)
            assert(corrected[row*4+axis]==world_matrix[row*4+axis]*.5f);
        auto cabin=c;cabin.ecx=renders[2];const uint32_t cabin_args[]={worlds[2],heli+0x10};
        Invoke(cabin,__imp__sub_000B8B60,cabin_args,2);
        const auto chopper=MatrixAt(worlds[2]+2*64),gun=MatrixAt(worlds[slot]+2*64);
        for(unsigned axis=0;axis<3;++axis){const auto& p=coop_turret_mount::kGunPivot[slot];
            const float wanted=chopper[12+axis]+p[0]*chopper[axis]+p[1]*chopper[4+axis]+p[2]*chopper[8+axis];
            assert(std::abs(gun[12+axis]-wanted)<.002f);}
        assert(std::abs(old_muzzle[12]-MatrixAt(worlds[slot]+3*64)[12])>1.f);
        // Original C01C0 selector11 ->20A2D0->21AFD0->B5570 must return
        // the same new world-space muzzle as the actual rendered gun children.
        auto query=c;query.eax=11;query.esi=owner;query.ebp=0x7e0000;Write(query.ebp+12,owner+0x260);
        OriginalTurretMuzzleSelector(query,Bytes());Near(MatrixAt(owner+0x260),MatrixAt(worlds[slot]+3*64));
        Pose pose;assert(pose.Read(Bytes(),owner));Matrix host_pose;assert(pose.Bone(Bytes(),"machinepistol",host_pose));
        native=c;native.ecx=render;MatrixAt(0x7d0000,Identity());const uint32_t local_args[]={0x7c0000,0x7d0000};
        Invoke(native,__imp__sub_000B8B60,local_args,2);Near(host_pose,MatrixAt(0x7c0000+2*64));
    }
    for(unsigned slot=0;slot<2;++slot){
        auto c=Setup(slot);c.edi=owners[slot];c.esi=0x710000+slot*0x1000;const auto saved=c;
        const uint32_t own=renders[slot],other=renders[slot^1];Bytes()[own+0x66]=2;
        {TrialCoopTurretView view;view.Select(c,Bytes());
            assert(Bytes()[other+0x66]==1&&(Read(other+0x60)&0x1b0)==0);
            assert(Bytes()[own+0x66]==2&&Read(own+0x60)==0x1b4);
            auto visible=c;visible.ecx=other;Invoke(visible,__imp__sub_000B5E90,nullptr,0);assert((visible.eax&255)==1);
            Write(other+0x60,Read(other+0x60)|0x200);}
        assert(Bytes()[other+0x66]==0&&Read(other+0x60)==0x3b4&&!std::memcmp(&c,&saved,sizeof c));
        try{TrialCoopTurretView view;view.Select(c,Bytes());throw 7;}catch(int){}
        assert(!Bytes()[other+0x66]&&Read(other+0x60)==0x3b4);
        mounted[slot^1]=false;{TrialCoopTurretView view;view.Select(c,Bytes());assert(!Bytes()[other+0x66]);}
        mounted[slot^1]=true;Write(c.esi+0x210,1);{TrialCoopTurretView view;view.Select(c,Bytes());assert(!Bytes()[other+0x66]);}
        Write(c.esi+0x210,2);{TrialCoopTurretView view;view.Select(c,Bytes());Write(owners[slot^1]+0x54,999);}
        assert(Bytes()[other+0x66]==1); // Reused actor identity is never overwritten on unwind.
    }
    for(unsigned bad=0;bad<7;++bad){
        auto c=Setup();const auto original=MatrixAt(owners[0]+0x6f0);
        if(bad==0)trial_coop::SetActive(false);
        if(bad==1)mounted[0]=false;
        if(bad==2)Write(owners[0]+0x4fc,24);
        if(bad==3)Bytes()[bones[0]+2*0xa0+4]='X';
        if(bad==4)Write(models[0]+0xc,257);
        if(bad==5){uint16_t parent=2;std::memcpy(Bytes()+bones[0]+2*0xa0+0x16,&parent,2);}
        if(bad==6)Write(owners[0]+0x5c,1);
        TrialCoopTurretWeaponMatrix(c,Bytes());assert(!refreshes);Near(original,MatrixAt(owners[0]+0x6f0));
    }
    auto c=Setup();const auto saved=c;const unsigned csr=_mm_getcsr();throwing=true;
    try{TrialCoopTurretWeaponMatrix(c,Bytes());assert(false);}catch(int value){assert(value==17);}
    assert(!std::memcmp(&c,&saved,sizeof c)&&_mm_getcsr()==csr);
    puts("co-op turret: actual posed hierarchy, native muzzle queries, both view scopes, metadata exclusion and arithmetic preservation passed (offline; live clearance pending)");
}

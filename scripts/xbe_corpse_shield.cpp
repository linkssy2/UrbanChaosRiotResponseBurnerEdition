#include "xbe_corpse_shield.h"
#include "xbe_corpse_shield_physics.h"
#include "xbe_cheats.h"
#include "xbe_coop_guest.h"
#include "xbe_coop_body.h"
#include "cpu/x86_ops.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <exception>
#include <filesystem>
#include <xmmintrin.h>

namespace corpse_shield {
constexpr const char* original[]={
    "assets_playerweaponsandequipment\\eqp_shield\\eqp_shield_skin.amf",
    "assets_characters\\cha_multiplayer\\cha_multiplayer_tzero\\cha_multiplayer_tzero_shield.amf"};
constexpr const char* pickup_original="assets_playerweaponsandequipment\\eqp_shield\\eqp_shield.amf";
constexpr const char* alias[]={
    "assets_cheats\\corpse_shield\\corpse_shield_fp.amf",
    "assets_cheats\\corpse_shield\\corpse_shield_tp.amf",
    "assets_cheats\\corpse_shield\\corpse_shield_native.amf",
    "assets_cheats\\corpse_shield\\corpse_shield_pickup.amf"};
constexpr const char* upgrades[]={
    "assets_playerweaponsandequipment\\eqp_shield\\eqp_shield_upgrade_skin.amf",
    "assets_playerweaponsandequipment\\eqp_shield\\eqp_shield_burner_skin.amf"};
constexpr const char* tags[]={"CorpseShieldFP","CorpseShieldTP","","CorpseShieldPickup"};
constexpr unsigned counts[]={121,18,73,121},first_joint[]={79,3};
constexpr unsigned joints[]={2,3,5,6,8,10,11,12,24,25,26,37,38,41,42};
using Matrix=std::array<float,16>;
struct Ready {
    uint32_t world=0,owner=0,id=0,entity=0,entity_id=0,wrapper=0,render=0,resource=0;
    const char* fallback=nullptr;
};
struct Held {
    uint32_t owner=0,id=0,wrapper=0,render=0;
    TrialCorpseShieldPhysics physics;
    Matrix world{};
    std::array<Matrix,73> rest{},pose{};
    bool posed=false;
};
std::array<Ready,4> ready{};
std::array<Held,2> held{};
std::array<uint32_t,4> resources{};
thread_local bool loading=false,restoring=false;
thread_local std::array<uint32_t,4> observed{};
uint32_t Word(const uint8_t* b,uint32_t p){uint32_t v;std::memcpy(&v,b+p,4);return v;}
void Put(uint8_t* b,uint32_t p,uint32_t v){std::memcpy(b+p,&v,4);}
struct FP {unsigned value=_mm_getcsr();~FP(){_mm_setcsr(value);}};
bool Name(const uint8_t* b,uint32_t p,const char* name){
    const size_t n=std::strlen(name);
    if(!TrialGuestReadable(p,uint32_t(n+1))||b[p+n])return false;
    for(size_t i=0;i<n;++i){unsigned char c=b[p+i];if(c>='A'&&c<='Z')c+='a'-'A';if(c=='/')c='\\';if(c!=name[i])return false;}
    return true;
}
bool Owner(const uint8_t* b,uint32_t p){
    return b&&TrialGuestReadable(p,0x890)&&Word(b,p)==0x46fbe8&&
        Word(b,p+0x54)!=UINT32_MAX&&!(Word(b,p+0x5c)&1)&&Word(b,p+0x368)<2;
}
bool Pickup(const uint8_t* b,uint32_t p,uint32_t world,uint32_t name){
    // Only the two original 12-01 table props have this class/index/model
    // combination. The preparer verifies that against every authored level.
    if(!TrialGuestReadable(world,8)||!TrialGuestReadable(p,0x152)||Word(b,p)!=0x47e278||
       Word(b,p+0x54)==UINT32_MAX||(Word(b,p+0x5c)&1))return false;
    const uint32_t index=Word(b,p+0x50);
    if(index!=585&&index!=578)return false;
    const uint32_t level=Word(b,world+4);
    if(!TrialGuestReadable(level,0x2058))return false;
    const uint32_t provider=Word(b,level+0x2054);
    if(!TrialGuestReadable(provider,0x14))return false;
    const uint32_t count=Word(b,provider+0x10),slots=Word(b,provider+4);
    if(count>16384||index>=count||!TrialGuestReadable(slots,count*4))return false;
    const uint32_t entry=Word(b,slots+index*4);
    if(!TrialGuestReadable(entry,0x14))return false;
    const uint32_t descriptor=Word(b,entry+0xc);
    // 210977 assigns the entry's live ID only after this model load returns.
    return TrialGuestReadable(descriptor,0x58)&&Word(b,descriptor+0x4c)==name&&Name(b,name,pickup_original);
}
bool Resource(const uint8_t* b,uint32_t p,unsigned kind){
    return TrialGuestReadable(p,0x50)&&Word(b,p)==0x457008&&Word(b,p+0xc)==counts[kind]&&
        (kind==2||!std::memcmp(b+p+0x10,tags[kind],std::strlen(tags[kind])+1))&&
        TrialGuestReadable(Word(b,p+0x3c),counts[kind]*64)&&
        TrialGuestReadable(Word(b,p+0x40),counts[kind]*160);
}
bool Current(const uint8_t* b,const Ready& r,unsigned kind){
    if(!r.owner||!Owner(b,r.owner)||Word(b,r.owner+0x54)!=r.id||
       Word(b,0x51c4dc)!=r.world||!TrialGuestReadable(r.world,0x20)||
       !TrialGuestReadable(r.entity,kind?0x104:0x890)||Word(b,r.entity+0x54)!=r.entity_id||
       (Word(b,r.entity+0x5c)&1)||Word(b,r.entity+0x74)!=r.wrapper||
       !TrialGuestReadable(r.wrapper,4)||Word(b,r.wrapper)!=r.render||
       !TrialGuestReadable(r.render,0x118)||Word(b,r.render)!=0x457880||
       Word(b,r.render+0x68)!=r.resource||!Resource(b,r.resource,kind))return false;
    if(kind){const uint32_t body=TrialCoopBodyForPlayer(b,r.owner);
        if(!TrialGuestReadable(body,0x60)||Word(b,body+0x54)!=Word(b,r.entity+0x58))return false;
    }
    return true;
}
bool Loaded(const uint8_t* b,unsigned kind){
    if(kind==3)return Resource(b,resources[3],3);
    return Resource(b,resources[0],0)&&Resource(b,resources[1],1)&&Resource(b,resources[2],2);
}
bool Load(X86Context& c,uint8_t* b,uint32_t storage,unsigned kind=0){
    if(loading||!TrialGuestReadable(c.fs_base,4))return false;
    if(Loaded(b,kind))return true;
    const auto path=kernel::ResolvePath("D:\\corpseshield.lst",0);
    std::error_code error;const auto size=std::filesystem::file_size(path,error);
    if(error||size<0xc8||size>2*1024*1024)return false;
    const uint32_t stack=mem::AllocStack(0x10000);if(!stack)return false;
    struct Restore {
        uint32_t stack;uint8_t* b;uint32_t fs,seh;unsigned csr=_mm_getcsr();
        ~Restore(){Put(b,fs,seh);loading=false;observed={};_mm_setcsr(csr);mem::FreeStack(stack);}
    } restore{stack,b,c.fs_base,Word(b,c.fs_base)};
    loading=true;observed={};_mm_setcsr(0x1f80);
    std::memcpy(b+storage+16,"corpseshield",sizeof("corpseshield"));
    X86Context call=c;call.esp=stack-16;call.df=0;
    kernel::CallGuest(call,0x54b70,{storage+16});
    // A cached archive need not enumerate its resources again. In particular,
    // an older three-alias pack must keep working for an already held shield.
    for(unsigned i=0;i<resources.size();++i){
        if(Resource(b,observed[i],i))resources[i]=observed[i];
        else if(!Resource(b,resources[i],i))resources[i]=0;
    }
    return Loaded(b,kind);
}
Matrix Product(const Matrix& a,const Matrix& b){
    Matrix out{};
    for(unsigned row=0;row<4;++row)for(unsigned col=0;col<4;++col)
        out[row*4+col]=(a[row*4]*b[col]+a[row*4+1]*b[4+col])+
            (a[row*4+2]*b[8+col]+a[row*4+3]*b[12+col]);
    return out;
}
bool Anchor(const uint8_t* b,const Ready& r,unsigned kind,Matrix& out){
    const uint32_t locals=Word(b,r.render+0x6c),records=Word(b,r.resource+0x40);
    if(!TrialGuestReadable(locals,counts[kind]*64))return false;
    unsigned joint=first_joint[kind]-1;
    std::memcpy(out.data(),b+locals+joint*64,64);
    for(;;){
        for(float value:out)if(!std::isfinite(value)||std::abs(value)>1.e7f)return false;
        if(!joint)return true;
        uint16_t parent;std::memcpy(&parent,b+records+joint*160+0x16,2);
        if(parent>=joint)return false;
        Matrix local;std::memcpy(local.data(),b+locals+parent*64,64);
        out=Product(out,local);joint=parent;
    }
}
bool MatrixAt(const uint8_t* b,uint32_t p,Matrix& out){
    if(!TrialGuestReadable(p,64))return false;
    std::memcpy(out.data(),b+p,64);
    for(float v:out)if(!std::isfinite(v)||std::abs(v)>1.e7f)return false;
    return true;
}
bool Inverse(const Matrix& m,Matrix& out){
    const float det=m[0]*(m[5]*m[10]-m[6]*m[9])-m[1]*(m[4]*m[10]-m[6]*m[8])+m[2]*(m[4]*m[9]-m[5]*m[8]);
    if(!std::isfinite(det)||std::abs(det)<1.e-8f)return false;
    out={};out[15]=1;
    out[0]=(m[5]*m[10]-m[6]*m[9])/det;out[1]=(m[2]*m[9]-m[1]*m[10])/det;out[2]=(m[1]*m[6]-m[2]*m[5])/det;
    out[4]=(m[6]*m[8]-m[4]*m[10])/det;out[5]=(m[0]*m[10]-m[2]*m[8])/det;out[6]=(m[2]*m[4]-m[0]*m[6])/det;
    out[8]=(m[4]*m[9]-m[5]*m[8])/det;out[9]=(m[1]*m[8]-m[0]*m[9])/det;out[10]=(m[0]*m[5]-m[1]*m[4])/det;
    for(unsigned c=0;c<3;++c)out[12+c]=-(m[12]*out[c]+m[13]*out[4+c]+m[14]*out[8+c]);
    return true;
}
unsigned Parent(const uint8_t* b,uint32_t resource,unsigned joint){
    uint16_t p;std::memcpy(&p,b+Word(b,resource+0x40)+joint*160+0x16,2);return p;
}
bool ComposeRest(const uint8_t* b,uint32_t resource,std::array<Matrix,73>& out){
    const uint32_t locals=Word(b,resource+0x3c);
    for(unsigned i=0;i<73;++i){if(!MatrixAt(b,locals+i*64,out[i]))return false;
        if(i){const unsigned parent=Parent(b,resource,i);if(parent>=i)return false;out[i]=Product(out[i],out[parent]);}
    }
    return true;
}
bool Mapping(const uint8_t* b,const Ready& r,unsigned kind,const Held& h,Matrix& map,std::array<Matrix,15>& rest){
    const uint32_t authored=Word(b,r.resource+0x3c),records=Word(b,r.resource+0x40);
    for(unsigned i=0;i<15;++i){
        const unsigned joint=first_joint[kind]+i,parent=Parent(b,r.resource,joint);
        if(!MatrixAt(b,authored+joint*64,rest[i]))return false;
        if(parent>=first_joint[kind]){if(parent>=joint)return false;rest[i]=Product(rest[i],rest[parent-first_joint[kind]]);}
        else if(parent!=first_joint[kind]-1)return false;
    }
    // Compact binds bake the rest shield anchor. Undo that rotation; the
    // displayed body and native physics both retain original scale.
    if(!MatrixAt(b,records+first_joint[kind]*160+96,map))return false;
    map[3]=map[7]=map[11]=0;map[15]=1;
    for(unsigned c=0;c<3;++c)map[12+c]=rest[2][12+c]-(h.rest[5][12]*map[c]+h.rest[5][13]*map[4+c]+h.rest[5][14]*map[8+c]);
    return true;
}
bool Target(const uint8_t* b,const Ready& r,const Held& h,Matrix& world){
    // CE670 updates owner+6F0 before the shared-world step; C0180 copies
    // it to render+20 only later during drawing.
    Matrix map,anchor,render;std::array<Matrix,15> rest;
    if(!Mapping(b,r,0,h,map,rest)||!Anchor(b,r,0,anchor)||!MatrixAt(b,r.owner+0x6f0,render))return false;
    const Matrix display=Product(Product(map,anchor),render);
    world=display;
    for(unsigned row=0;row<3;++row){const unsigned i=row*4;
        const float scale=std::sqrt(display[i]*display[i]+display[i+1]*display[i+1]+display[i+2]*display[i+2]);
        if(!std::isfinite(scale)||scale<.01f||scale>10.f)return false;
        for(unsigned c=0;c<3;++c)world[i+c]/=scale;
    }
    // Pin native chest and displayed chest at the same held point, without
    // scaling Havok shapes/constraints by passing a scaled actor transform.
    for(unsigned c=0;c<3;++c)world[12+c]=display[12+c]+h.rest[5][12]*(display[c]-world[c])+h.rest[5][13]*(display[4+c]-world[4+c])+h.rest[5][14]*(display[8+c]-world[8+c]);
    return true;
}
struct NativeCall {
    X86Context seed;uint8_t* base;uint32_t stack=0,seh=0;FP fp;
    NativeCall(const X86Context& c,uint8_t* b):seed(c),base(b){
        if(TrialGuestReadable(c.fs_base,4)){seh=Word(b,c.fs_base);stack=mem::AllocStack(0x10000);}
        _mm_setcsr(0x1f80);
    }
    ~NativeCall(){if(stack){Put(base,seed.fs_base,seh);mem::FreeStack(stack);}}
    uint32_t operator()(uint32_t address,uint32_t self,std::initializer_list<uint32_t> args){
        X86Context c=seed;c.esp=stack-16;c.ecx=self;c.df=0;return kernel::CallGuest(c,address,args);
    }
};
void Destroy(const X86Context& c,uint8_t* b,Held& h){
    TrialCorpseShieldPhysicsDestroy(c,b,h.physics);
    if(h.physics.ragdoll)return;
    if(h.wrapper){NativeCall call(c,b);if(!call.stack){X86Context failure=c;x86_trap(failure,0x208d90,"Corpse shield model teardown stack unavailable");}
        if(TrialGuestReadable(h.wrapper,4)&&TrialGuestReadable(Word(b,h.wrapper),0x118))call(0x21b1a0,h.wrapper,{});
        call(0x2d5ccd,0,{h.wrapper});
    }
    h={};
}
void RestoreShield(const X86Context& c,uint8_t* b,unsigned slot){
    NativeCall call(c,b);
    if(!call.stack){X86Context failure=c;x86_trap(failure,0x20bc40,"Corpse shield fallback stack unavailable");}
    struct Restore {bool value=restoring;Restore(){restoring=true;}~Restore(){restoring=value;}} restore;
    // The native loader expects a pointer to a name pointer. Reserve it far
    // below its call frames rather than retaining any original stack address.
    const uint32_t scratch=call.stack-0xff00;
    for(unsigned kind=0;kind<2;++kind){auto& r=ready[slot*2+kind];
        if(Current(b,r,kind)&&r.fallback){
            std::memcpy(b+scratch+16,r.fallback,std::strlen(r.fallback)+1);Put(b,scratch,scratch+16);
            call(kind?0xfe170:0xbe050,r.entity,{scratch,0});
        }
        r={};
    }
}
bool Create(const X86Context& c,uint8_t* b,const Ready& r,Held& h){
    if(!Resource(b,resources[2],2)||!ComposeRest(b,resources[2],h.rest)||!Target(b,r,h,h.world))return false;
    NativeCall call(c,b);if(!call.stack)return false;
    const uint32_t scratch=mem::AllocSystem(256);if(!scratch)return false;
    struct Free {uint32_t p;~Free(){mem::FreeSystem(p);}} free{scratch};
    if(!TrialGuestReadable(scratch,256))return false;
    std::memcpy(b+scratch,alias[2],std::strlen(alias[2])+1);std::memcpy(b+scratch+128,h.world.data(),64);
    h.wrapper=call(0x2d57fb,0,{0x50});if(!h.wrapper)return false;
    if(!TrialGuestReadable(h.wrapper,0x50))return false;
    std::memset(b+h.wrapper,0,0x50);
    call(0x21d0e0,h.wrapper,{scratch,scratch+128});
    if(!TrialGuestReadable(h.wrapper,4))return false;
    h.render=Word(b,h.wrapper);
    if(!TrialGuestReadable(h.render,0x118)||Word(b,h.render)!=0x457880||Word(b,h.render+0x68)!=resources[2])return false;
    // This wrapper is not an entity and is never animated/drawn. Initialize
    // both native skeleton arrays, which Havok reads and writes directly.
    const uint32_t local=Word(b,h.render+0x6c),composed=Word(b,h.render+0x70);
    if(!TrialGuestReadable(local,73*64)||!TrialGuestReadable(composed,73*64))return false;
    std::memcpy(b+local,b+Word(b,resources[2]+0x3c),73*64);std::memcpy(b+composed,h.rest.data(),73*64);
    h.owner=r.owner;h.id=r.id;
    return TrialCorpseShieldPhysicsCreate(c,b,h.physics,h.wrapper,h.world,r.owner);
}
void Pose(uint8_t* b,const Ready& r,unsigned kind){
    const auto& h=held[Word(b,r.owner+0x368)];
    if(h.owner!=r.owner||h.id!=r.id||!h.posed||!TrialCorpseShieldPhysicsValid(b,h.physics))return;
    const uint32_t locals=Word(b,r.render+0x6c);
    if(!TrialGuestReadable(locals,counts[kind]*64)||locals==Word(b,r.resource+0x3c))return;
    Matrix map,inverse_map,anchor;std::array<Matrix,15> rest,world,result;
    if(!Mapping(b,r,kind,h,map,rest)||!Inverse(map,inverse_map)||!Anchor(b,r,kind,anchor))return;
    for(unsigned i=0;i<15;++i){
        Matrix inverse_rest;if(!Inverse(h.rest[joints[i]],inverse_rest))return;
        world[i]=Product(Product(Product(Product(Product(rest[i],inverse_map),inverse_rest),h.pose[joints[i]]),map),anchor);
        const unsigned parent=Parent(b,r.resource,first_joint[kind]+i);
        Matrix inverse_parent;
        if(!Inverse(parent>=first_joint[kind]?world[parent-first_joint[kind]]:anchor,inverse_parent))return;
        result[i]=Product(world[i],inverse_parent);
        for(float v:result[i])if(!std::isfinite(v)||std::abs(v)>1.e7f)return;
    }
    for(unsigned i=0;i<15;++i)std::memcpy(b+locals+(first_joint[kind]+i)*64,result[i].data(),64);
}
}

void TrialCorpseShieldReset(){using namespace corpse_shield;if(!loading){ready={};resources={};}}
void TrialCorpseShieldResourceLoaded(const X86Context& c,const uint8_t* b){
    using namespace corpse_shield;
    if(!loading||!b||!TrialGuestReadable(c.esp,0x18))return;
    const uint32_t name=Word(b,c.esp+0x10);
    for(unsigned kind=0;kind<observed.size();++kind)if(Name(b,name,alias[kind])&&Resource(b,c.eax,kind))observed[kind]=c.eax;
}
bool TrialCorpseShieldActive(const uint8_t* b,uint32_t owner){
    using namespace corpse_shield;
    if(!trial_cheats::CorpseShieldEnabled()||!Owner(b,owner)||!TrialGuestReadable(0x51c4dc,4))return false;
    const auto& r=ready[Word(b,owner+0x368)*2];
    const auto& h=held[Word(b,owner+0x368)];
    return r.owner==owner&&Current(b,r,0)&&h.owner==owner&&h.id==r.id&&h.posed&&TrialCorpseShieldPhysicsValid(b,h.physics);
}
bool TrialCorpseShieldIgnoreQuery(const uint8_t* b,uint32_t collidable,uint32_t query_filter){
    using namespace corpse_shield;
    if(!b||(!held[0].owner&&!held[1].owner)||!trial_cheats::CorpseShieldEnabled()||
       !TrialGuestReadable(collidable,0x20))return false;
    // Native 2A8860 resolves the rigid body from this self-relative pointer.
    // Its embedded collidable is at +14; never admit another interior pointer.
    const uint32_t body=collidable+Word(b,collidable+0x10);
    if(!TrialGuestReadable(body,0x54)||uint64_t(body)+0x14!=collidable)return false;
    for(unsigned slot=0;slot<held.size();++slot){const auto& h=held[slot];
        if(!Owner(b,h.owner)||Word(b,h.owner+0x368)!=slot||
           (query_filter&0xfffff)!=(Word(b,h.owner+0x94)&0xffff)||
           !TrialCorpseShieldActive(b,h.owner)||!TrialGuestReadable(h.physics.ragdoll,0x78))continue;
        const uint32_t bodies=Word(b,h.physics.ragdoll+0x70);
        if(!TrialGuestReadable(bodies,48)||Word(b,h.physics.ragdoll+0x74)!=12)continue;
        for(unsigned i=0;i<12;++i)if(Word(b,bodies+i*4)==body)return true;
    }
    return false;
}
void TrialCorpseShieldBeforePhysics(const X86Context& c,uint8_t* b,float dt){
    using namespace corpse_shield;
    if(!b||!TrialGuestReadable(0x51c4dc,4)||!std::isfinite(dt)||dt<=0)return;
    FP fp;_mm_setcsr(0x1f80);
    for(unsigned slot=0;slot<2;++slot){const auto& r=ready[slot*2];auto& h=held[slot];
        if(!Current(b,r,0)){Destroy(c,b,h);continue;}
        if(!trial_cheats::CorpseShieldEnabled()){Destroy(c,b,h);RestoreShield(c,b,slot);continue;}
        if(h.owner!=r.owner||h.id!=r.id||!TrialCorpseShieldPhysicsValid(b,h.physics)){
            Destroy(c,b,h);if(h.wrapper)continue;
            if(!Create(c,b,r,h)){Destroy(c,b,h);RestoreShield(c,b,slot);continue;}
        }
        Matrix target;
        if(!Target(b,r,h,target)||!TrialCorpseShieldPhysicsBefore(c,b,h.physics,target,dt)){Destroy(c,b,h);RestoreShield(c,b,slot);continue;}
        h.world=target;
    }
}
void TrialCorpseShieldAfterPhysics(const X86Context& c,uint8_t* b,float dt){
    using namespace corpse_shield;
    if(!b||!std::isfinite(dt)||dt<=0)return;
    FP fp;_mm_setcsr(0x1f80);
    for(unsigned slot=0;slot<2;++slot){auto& h=held[slot];
        if(!h.wrapper)continue;
        h.posed=TrialCorpseShieldPhysicsValid(b,h.physics)&&TrialCorpseShieldPhysicsAfter(c,b,h.physics,h.world,dt);
        if(h.posed){const uint32_t composed=Word(b,h.render+0x70);
            for(unsigned i=0;i<73;++i)if(!MatrixAt(b,composed+i*64,h.pose[i])){h.posed=false;break;}
        }
        if(!h.posed){Destroy(c,b,h);RestoreShield(c,b,slot);}
    }
}
void TrialCorpseShieldRelease(const X86Context& c,uint8_t* b){
    using namespace corpse_shield;
    for(auto& h:held)Destroy(c,b,h);
    ready={};resources={};
}
void TrialCorpseShieldPose(uint8_t* b,uint32_t render){
    using namespace corpse_shield;
    if(!b||!TrialGuestReadable(0x51c4dc,4))return;
    for(unsigned index=0;index<ready.size();++index){auto& r=ready[index];
        if(r.render==render&&Current(b,r,index%2)){FP fp;_mm_setcsr(0x1f80);Pose(b,r,index%2);return;}
    }
}
TrialCorpseShieldScope::TrialCorpseShieldScope(X86Context& c,uint8_t* b):exceptions_(std::uncaught_exceptions()){
    using namespace corpse_shield;
    if(restoring||!b||!TrialGuestReadable(0x51c4dc,4))return;
    const uint32_t world=Word(b,0x51c4dc);if(!TrialGuestReadable(world,0x20))return;
    if(!TrialGuestReadable(c.esp,4))return;
    const uint32_t caller=Word(b,c.esp);
    if(caller==0x20a1f1){
        if(!TrialGuestReadable(c.esp,8))return;
        const uint32_t cell=Word(b,c.esp+4),name=TrialGuestReadable(cell,4)?Word(b,cell):0;
        if(!Pickup(b,c.ecx,world,name))return;
        context_=&c;base_=b;owner_=entity_=c.ecx;id_=Word(b,c.ecx+0x54);world_=world;kind_=3;
        return;
    }
    uint32_t owner=c.ecx;kind_=caller==0xbe075?0:1;
    if(!kind_){if(!Owner(b,owner))return;}
    else{
        if(caller!=0xfe184||
           !TrialGuestReadable(c.ecx,0x104)||Word(b,c.ecx)!=0x462658||Word(b,c.ecx+0x54)==UINT32_MAX)return;
        owner=0;
        for(unsigned slot=0;slot<2;++slot){const uint32_t player=TrialCoopPlayer(slot),body=TrialCoopBodyForPlayer(b,player);
            if(Owner(b,player)&&TrialGuestReadable(body,0x60)&&Word(b,body+0x54)==Word(b,c.ecx+0x58)){owner=player;break;}
        }
        if(!owner)return;
    }
    context_=&c;base_=b;owner_=owner;entity_=c.ecx;id_=Word(b,owner+0x54);world_=world;slot_=Word(b,owner+0x368);
    // 20BC40 replaces the entity's current model, including switching away
    // from the shield. The independent flashlight lives at +660.
    ready[slot_*2+kind_]={};
}
void TrialCorpseShieldScope::Select(X86Context& c){
    using namespace corpse_shield;
    if(!base_||selected_||!trial_cheats::CorpseShieldEnabled()||c.esi!=entity_||
       !TrialGuestReadable(owner_,0x58)||Word(base_,owner_+0x54)!=id_||Word(base_,0x51c4dc)!=world_)return;
    const uint32_t name=TrialGuestReadable(c.ebp,4)?Word(base_,c.ebp):0;
    if(kind_==3){if(!Pickup(base_,entity_,world_,name))return;fallback_=pickup_original;}
    else{
        if(!Owner(base_,owner_))return;
        fallback_=Name(base_,name,original[kind_])?original[kind_]:nullptr;
    }
    if(!kind_)for(const char* upgrade:upgrades)if(Name(base_,name,upgrade))fallback_=upgrade;
    if(!fallback_)return;
    storage_=mem::AllocSystem(256);if(!storage_||!TrialGuestReadable(storage_,256)||!Load(c,base_,storage_,kind_))return;
    std::memcpy(base_+storage_+64,alias[kind_],std::strlen(alias[kind_])+1);Put(base_,storage_,storage_+64);
    c.ebp=storage_;
    selected_=true;
}
TrialCorpseShieldScope::~TrialCorpseShieldScope(){
    using namespace corpse_shield;
    if(kind_<2&&selected_&&exceptions_==std::uncaught_exceptions()&&uint8_t(context_->eax)&&
       Owner(base_,owner_)&&Word(base_,owner_+0x54)==id_&&Word(base_,0x51c4dc)==world_){
        const uint32_t wrapper=Word(base_,entity_+0x74);
        if(TrialGuestReadable(wrapper,4)){
            const uint32_t render=Word(base_,wrapper);
            if(TrialGuestReadable(render,0x118)&&Word(base_,render)==0x457880){
                const uint32_t resource=Word(base_,render+0x68);
                if(Resource(base_,resource,kind_))ready[slot_*2+kind_]={world_,owner_,id_,entity_,Word(base_,entity_+0x54),wrapper,render,resource,fallback_};
            }
        }
    }
    if(storage_)mem::FreeSystem(storage_);
}

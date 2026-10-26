#include "xbe_coop_turret_view.h"
#include "xbe_coop_turrets.h"
#include "xbe_coop_turret_mount.h"
#include "xbe_coop_guest.h"
#include "xbe_coop.h"
#include "cpu/x86_ops.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <array>
#include <cmath>
#include <cstring>
#include <xmmintrin.h>

namespace {
uint32_t Word(const uint8_t* b,uint32_t p){uint32_t v;std::memcpy(&v,b+p,4);return v;}
void Put(uint8_t* b,uint32_t p,uint32_t v){std::memcpy(b+p,&v,4);}
bool Actor(const uint8_t* b,uint32_t p,unsigned slot){
    return b&&trial_coop::Active()&&slot<2&&p==TrialCoopPlayer(slot)&&
        TrialGuestReadable(p,0x890)&&Word(b,p)==0x46fbe8&&Word(b,p+0x368)==slot&&
        Word(b,p+0x54)!=0xffffffffu&&!(Word(b,p+0x5c)&1);
}
uint32_t Gun(const uint8_t* b,uint32_t p){
    if(!TrialGuestReadable(p,0x890)||!TrialCoopTurretMount(b,p)||Word(b,p+0x4fc)!=25)return 0;
    const uint32_t wrapper=Word(b,p+0x74);
    if(!TrialGuestReadable(wrapper,4))return 0;
    const uint32_t render=Word(b,wrapper);
    return TrialGuestReadable(render,0x118)&&Word(b,render)==0x457880?render:0;
}
bool View(const X86Context& c,const uint8_t* b,unsigned slot){
    const uint32_t owner=TrialCoopPlayer(slot);
    if(!Actor(b,owner,slot)||c.edi!=owner||c.ebx!=Word(b,0x51c4dc)||
       !TrialGuestReadable(c.ebx,8)||!TrialGuestReadable(c.esi,0x230)||
       Word(b,c.esi)!=0x47c780||Word(b,c.esi+0xd0)!=0x478d20||
       (Word(b,c.esi+0x5c)&1)||Word(b,c.esi+0x54)!=Word(b,owner+0x308)||
       Word(b,c.esi+0x58)!=Word(b,owner+0x54)||Word(b,c.esi+0x22c)!=Word(b,owner+0x54)||
       Word(b,c.esi+0x210)!=2)return false;
    const uint32_t state=Word(b,c.ebx+4);
    if(!TrialGuestReadable(state,12))return false;
    const uint32_t first=Word(b,state+4),end=Word(b,state+8);
    if(end<first||end-first!=8||!TrialGuestReadable(first,8))return false;
    const uint32_t layer=Word(b,first+slot*4);
    if(!TrialGuestReadable(layer,0x28)||Word(b,layer+4)!=owner||Word(b,layer+0x24)!=slot)return false;
    const uint32_t stack=Word(b,layer+8);
    if(!TrialGuestReadable(stack,12)||!Word(b,stack+8))return false;
    const uint32_t head=Word(b,stack+4);
    if(!TrialGuestReadable(head,8))return false;
    const uint32_t node=Word(b,head+4);
    return TrialGuestReadable(node,12)&&Word(b,node+8)==c.esi;
}
}

namespace {
using Matrix=std::array<float,16>;
Matrix Product(const Matrix& a,const Matrix& b){
    Matrix out{};
    for(unsigned row=0;row<4;++row)for(unsigned axis=0;axis<4;++axis)
        // Same addition grouping as retail B8B60, including its translation.
        out[row*4+axis]=(a[row*4]*b[axis]+a[row*4+1]*b[4+axis])+
                       (a[row*4+2]*b[8+axis]+a[row*4+3]*b[12+axis]);
    return out;
}
bool ReadMatrix(const uint8_t* b,uint32_t address,Matrix& out){
    if(!TrialGuestReadable(address,64))return false;
    std::memcpy(out.data(),b+address,64);
    for(float value:out)if(!std::isfinite(value))return false;
    return out[3]==0.f&&out[7]==0.f&&out[11]==0.f&&out[15]==1.f;
}
struct Pose {
    uint32_t render=0,records=0,locals=0,worlds=0,count=0;
    bool Read(const uint8_t* b,uint32_t entity){
        if(!TrialGuestReadable(entity,0x78))return false;
        const uint32_t wrapper=Word(b,entity+0x74);
        if(!TrialGuestReadable(wrapper,4))return false;
        render=Word(b,wrapper);
        if(!TrialGuestReadable(render,0x118)||Word(b,render)!=0x457880)return false;
        const uint32_t model=Word(b,render+0x68);
        if(!TrialGuestReadable(model,0x44))return false;
        count=Word(b,model+0xc);records=Word(b,model+0x40);
        locals=Word(b,render+0x6c);worlds=Word(b,render+0x70);
        if(!count||count>256||!TrialGuestReadable(records,count*0xa0)||
           !TrialGuestReadable(locals,count*64)||!TrialGuestReadable(worlds,count*64))return false;
        // B8B60 requires parents to precede children. Validate before invoking
        // it, and never let stale/non-finite animation data affect the gun.
        for(unsigned i=0;i<count;++i){
            uint16_t parent;std::memcpy(&parent,b+records+i*0xa0+0x16,2);
            Matrix local;if((i&&parent>=i)||!ReadMatrix(b,locals+i*64,local))return false;
        }
        return true;
    }
    bool Bone(const uint8_t* b,const char* name,Matrix& out)const{
        // B5630 compares the 16-byte name at model bone+4. Use that same
        // bounded, case-insensitive identity, not a hardcoded bone ordinal.
        const size_t size=std::strlen(name);if(size>=16)return false;
        for(unsigned i=0;i<count;++i){
            const uint32_t at=records+i*0xa0+4;bool same=true;
            for(unsigned c=0;c<=size;++c){
                auto lower=[](unsigned char v){return v>='A'&&v<='Z'?v+('a'-'A'):v;};
                if(lower(b[at+c])!=lower(static_cast<unsigned char>(name[c]))){same=false;break;}
            }
            if(!same)continue;
            std::array<unsigned,256> chain{};unsigned depth=0;
            while(i){chain[depth++]=i;uint16_t parent;std::memcpy(&parent,b+records+i*0xa0+0x16,2);i=parent;}
            ReadMatrix(b,locals,out);
            while(depth){Matrix local;ReadMatrix(b,locals+chain[--depth]*64,local);out=Product(local,out);}
            return true;
        }
        return false;
    }
};
}
void TrialCoopTurretWeaponMatrix(X86Context& c,uint8_t* b){
    if(!b||!TrialGuestReadable(c.esi,0x890))return;
    const unsigned slot=Word(b,c.esi+0x368);
    if(!Actor(b,c.esi,slot)||c.edi!=c.esi+0x6f0||!Gun(b,c.esi))return;
    const uint32_t helicopter=TrialCoopTurretMount(b,c.esi);
    Pose gun,cabin;if(!gun.Read(b,c.esi)||!cabin.Read(b,helicopter))return;
    const uint32_t matrix_address=c.edi;
    Matrix matrix,gun_bone,cabin_bone,cabin_world;
    if(!ReadMatrix(b,matrix_address,matrix)||!ReadMatrix(b,helicopter+0x10,cabin_world))return;
    const uint32_t scratch=(c.esp-64)&~15u;
    if(c.esp<0x18000||!TrialGuestReadable(scratch-0x6000,0x6040))return;
    struct Restore {X86Context& c;X86Context saved;unsigned csr;uint8_t* scratch;
        std::array<uint8_t,0x140> bytes;
        Restore(X86Context& value,uint8_t* memory):c(value),saved(value),csr(_mm_getcsr()),scratch(memory){
            std::memcpy(bytes.data(),scratch,bytes.size());}
        ~Restore(){std::memcpy(scratch,bytes.data(),bytes.size());c=saved;_mm_setcsr(csr);}
    } restore(c,b+scratch-0x100); // B8B60 has no calls and a bounded 0x70-byte native frame.
    _mm_setcsr(0x1f80);
    if(!gun.Bone(b,"MachinePistol",gun_bone)||!cabin.Bone(b,"Chopper-Body",cabin_bone))return;
    cabin_world=Product(cabin_bone,cabin_world);
    const auto& pivot=coop_turret_mount::kGunPivot[slot];
    for(unsigned row=0;row<3;++row)for(unsigned axis=0;axis<3;++axis)
        matrix[row*4+axis]*=coop_turret_mount::kGunScale;
    for(unsigned axis=0;axis<3;++axis){
        const float target=cabin_world[12+axis]+pivot[0]*cabin_world[axis]+
            pivot[1]*cabin_world[4+axis]+pivot[2]*cabin_world[8+axis];
        matrix[12+axis]=target-(gun_bone[12]*matrix[axis]+
            gun_bone[13]*matrix[4+axis]+gun_bone[14]*matrix[8+axis]);
    }
    for(float value:matrix)if(!std::isfinite(value))return;
    // CE670 rebuilds this matrix every tick. Anchor the actual posed gun,
    // retain its original aiming/animation, and refresh the native world-bone
    // hierarchy before the following ammo/muzzle/effect queries use it.
    std::memcpy(b+matrix_address,matrix.data(),64);
    std::memcpy(b+gun.render+0x20,matrix.data(),64); // original B5C90 contract
    c.esp=scratch;c.ecx=gun.render;c.df=0;
    kernel::CallGuest(c,0xb8b60,{gun.worlds,matrix_address});
}

TrialCoopTurretView::~TrialCoopTurretView(){Clear();}
void TrialCoopTurretView::Clear(){
    if(base_&&TrialGuestReadable(owner_,0x78)&&Word(base_,owner_+0x54)==id_&&
       Word(base_,owner_+0x74)==wrapper_&&TrialGuestReadable(wrapper_,4)&&Word(base_,wrapper_)==render_&&
       TrialGuestReadable(render_,0x118)&&Word(base_,render_)==0x457880){
        Put(base_,render_+0x60,(Word(base_,render_+0x60)&~0x1b0u)|(flags_&0x1b0u));
        base_[render_+0x66]=visible_;
    }
    base_=nullptr;owner_=id_=wrapper_=render_=flags_=0;visible_=0;
}
void TrialCoopTurretView::Select(const X86Context& c,uint8_t* b){
    Clear();if(!b||!trial_coop::Active())return;
    unsigned slot=0;for(;slot<2;++slot)if(c.edi==TrialCoopPlayer(slot))break;
    if(slot==2||!View(c,b,slot))return;
    const uint32_t partner=TrialCoopPlayer(slot^1);
    if(!Actor(b,partner,slot^1))return;
    const uint32_t render=Gun(b,partner);if(!render)return;
    base_=b;owner_=partner;id_=Word(b,partner+0x54);wrapper_=Word(b,partner+0x74);render_=render;
    flags_=Word(b,render+0x60);visible_=b[render+0x66];
    // BAC10 uses these bits for first-person depth and alternate projection.
    // The partner sees the very same animated gun in ordinary world space.
    Put(b,render+0x60,flags_&~0x1b0u);b[render+0x66]=1;
}

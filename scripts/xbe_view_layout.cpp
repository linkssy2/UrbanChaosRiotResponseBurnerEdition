#include "xbe_view_layout.h"
#include "xbe_graphics_settings.h"
#include "cpu/x86_ops.h"
#include "kernel/memory.h"
#include "kernel/kernel.h"
#include "xbe_input_memory.h"
#include <cstring>
#include <atomic>
#include <xmmintrin.h>

namespace {
std::atomic<uint32_t> player{0};
thread_local TrialHudLayoutScope* hud=nullptr;
thread_local TrialReticleScope* reticle=nullptr;
uint32_t Word(const uint8_t* b,uint32_t a){uint32_t v;std::memcpy(&v,b+a,4);return v;}
float Real(const uint8_t* b,uint32_t a){float v;std::memcpy(&v,b+a,4);return v;}
void Put(uint8_t* b,uint32_t a,float v){std::memcpy(b+a,&v,4);}
struct Arithmetic {unsigned csr=_mm_getcsr();Arithmetic(){_mm_setcsr(0x1f80);}~Arithmetic(){_mm_setcsr(csr);}};
}

void TrialPlayerCameraCreated(const X86Context& c,const uint8_t* b){
    player=TrialGuestReadable(c.esi,0x30c)&&c.eax!=0xffffffffu?c.esi:0;
}
float TrialCameraTarget(const X86Context& c,const uint8_t* b){
    if(!TrialGuestReadable(c.ecx,0x230))return 0;
    const float target=Real(b,c.ecx+0x228);
    const unsigned fov=trial_graphics::FieldOfView();
    const uint32_t owner=player.load();
    if(fov==53 || !TrialGuestReadable(owner,0x30c))return target;
    // Match both live camera and parent IDs; cinematic/other follow cameras are excluded.
    if(Word(b,c.ecx+0x210)!=2 || Word(b,owner+0x308)!=Word(b,c.ecx+0x54) ||
       Word(b,owner+0x54)!=Word(b,c.ecx+0x58) || Word(b,c.ecx+0x22c)!=Word(b,owner+0x54))return target;
    Arithmetic arithmetic;
    // Change only this update's input, never the authored target or its zoom events.
    return trial_view::Zoom(target,fov);
}
void TrialWeaponProjection(const X86Context& c,uint8_t* b){
    const unsigned percent=trial_graphics::WeaponFovScale();
    if(percent==100||percent<50||percent>150||!b||
       !TrialGuestReadable(c.esi,0x118)||Word(b,c.esi)!=0x457880u||
       (Word(b,c.esi+0x60)&0x180u)!=0x180u||
       Word(b,c.esi+0x110)!=0x3dcccccdu||Word(b,c.esi+0x114)!=0x3e19999au||
       !TrialGuestReadable(c.edi,0x194)||Word(b,c.edi)!=0x456d88u||
       c.ecx!=c.edi||!TrialGuestReadable(c.esp,0x94))return;
    const uint32_t matrix=Word(b,c.esp);
    if(matrix!=c.esp+0x54)return;
    // C8930 assigns this alternate view/depth range to gun, arms and flashlight.
    // Change only BAC10's local copy; its original restore still owns world state.
    Arithmetic arithmetic;
    for(unsigned i=0;i<16;++i)if(!std::isfinite(Real(b,matrix+i*4)))return;
    const float x=Real(b,matrix),y=Real(b,matrix+20);
    if(x<=0||y<=0)return;
    const float scale=100.f/percent,scaled_x=x*scale,scaled_y=y*scale;
    if(!std::isfinite(scaled_x)||!std::isfinite(scaled_y))return;
    Put(b,matrix,scaled_x);Put(b,matrix+20,scaled_y);
}
TrialHudLayoutScope::TrialHudLayoutScope(const X86Context&,const uint8_t*):previous_(hud){hud=this;}
TrialHudLayoutScope::~TrialHudLayoutScope(){hud=previous_;}
void TrialScopeBorders(X86Context& c,uint8_t* b){
    if(!hud||hud->scale_==1||!TrialGuestReadable(c.esi,12)||Word(b,c.esi)!=0x4530bcu||
       !TrialGuestReadable(c.edi,16)||!TrialGuestReadable(c.esp,32)||c.esp<0x10200)return;
    Arithmetic arithmetic;
    const float width=Real(b,c.edi+8),height=Real(b,c.edi+12),alpha=Real(b,c.esp+28);
    if(Real(b,c.edi)!=0||Real(b,c.edi+4)!=0||std::abs(width-640/hud->scale_)>.01f||height!=480||
       !std::isfinite(alpha)||alpha<=0||alpha>1)return;
    const uint32_t scratch=(c.esp-128)&~15u;
    if(!TrialGuestReadable(scratch-32,160))return;
    const X86Context saved=c;const float edge=(width-640)*.5f;
    // Extend only the opaque outside of the circular retail mask, with its fade.
    for(unsigned side=0;side<2;++side){
        Put(b,scratch,side?width-edge:0);Put(b,scratch+4,0);
        Put(b,scratch+8,side?width:edge);Put(b,scratch+12,height);
        const uint32_t color=uint32_t(alpha*255.f+.5f)<<24;
        std::memcpy(b+scratch+16,&color,4);c.esp=scratch;
        kernel::CallGuest(c,0x656d0,{scratch,scratch+8,scratch+16});
    }
    c=saved;
}
void TrialHudLayout(const X86Context& c,uint8_t* b){
    if(!hud||!trial_graphics::Widescreen()||c.esp>0xffffffabu||!TrialGuestReadable(c.esp+0x10,0x44))return;
    const uint32_t d=c.esp+0x10;
    // Only the full retail 640x480 player HUD. Split views keep their original layout.
    if(Real(b,d)!=0 || Real(b,d+4)!=0 || Real(b,d+8)!=640 || Real(b,d+12)!=480)return;
    Arithmetic arithmetic;hud->scale_=.75f;Put(b,d+8,640/.75f);
}
uint32_t TrialHudCenterBits(){
    Arithmetic arithmetic;float center=320/(hud?hud->scale_:1);uint32_t bits;
    std::memcpy(&bits,&center,4);return bits;
}
TrialProjectedHudScope::TrialProjectedHudScope(const X86Context& c,const uint8_t* b):previous_(hud){
    // 73700 builds the OPITEMS bracket owner; 75B90 replaces its descriptor
    // bounds with 47610/470A0 camera projections before drawing the four corners.
    // These are already screen positions, not positions on our wider HUD canvas.
    if(hud&&hud->scale_!=1&&TrialGuestReadable(c.ecx,0x58)&&Word(b,c.ecx)==0x45312cu)
        hud=nullptr;
}
TrialProjectedHudScope::~TrialProjectedHudScope(){hud=previous_;}
TrialReticleScope::TrialReticleScope(const X86Context& c,const uint8_t* b):previous_(reticle){
    reticle=nullptr;
    const unsigned percent=trial_graphics::ReticleScale();
    // 0x73790 builds this class from weapon crosshair shaders. The sniper
    // mask/brackets use different classes and never enter its 0x73aa0 draw.
    if(!hud||percent==100||percent<50||percent>200||
       !TrialGuestReadable(c.ecx,0x94)||Word(b,c.ecx)!=0x453160u||
       !TrialGuestReadable(Word(b,c.ecx+0x90),4))return;
    Arithmetic arithmetic;scale_=float(percent)*.01f;reticle=this;
}
TrialReticleScope::~TrialReticleScope(){reticle=previous_;}
TrialHudQuadScope::TrialHudQuadScope(const X86Context& c,uint8_t* b):base_(b){
    if(!hud||(hud->scale_==1&&!reticle)||!TrialGuestReadable(c.esp,12))return;
    const uint32_t a=Word(b,c.esp+4),z=Word(b,c.esp+8);
    if(!TrialGuestReadable(a,8)||!TrialGuestReadable(z,8)||
       !std::isfinite(Real(b,a))||!std::isfinite(Real(b,z)))return;
    Arithmetic arithmetic;
    // Only the final sprite quad emitted by 0x7afe0 inside the reticle tree.
    // Resize about its authored center after all weapon placement/fade work;
    // leave UVs, spread/position inputs, visibility and scope masks untouched.
    if(reticle&&Word(b,c.esp)==0x7b149u&&TrialGuestReadable(c.esi,0x20)&&
       Word(b,c.esi)==0x453b50u&&(uint64_t(a)+8<=z||uint64_t(z)+8<=a)&&
       std::isfinite(Real(b,a+4))&&std::isfinite(Real(b,z+4))){
        float x0=Real(b,a),x1=Real(b,z),y0=Real(b,a+4),y1=Real(b,z+4);
        const float half_change=(reticle->scale_-1)*.5f;
        const float dx=(x1-x0)*half_change,dy=(y1-y0)*half_change;
        x0-=dx;x1+=dx;y0-=dy;y1+=dy;
        if(hud->scale_!=1){x0=trial_view::HudX(x0,hud->scale_);x1=trial_view::HudX(x1,hud->scale_);}
        if(std::isfinite(x0)&&std::isfinite(x1)&&std::isfinite(y0)&&std::isfinite(y1)){
            const uint32_t addresses[]={a,a+4,z,z+4};const float values[]={x0,y0,x1,y1};
            for(unsigned i=0;i<4;++i){address_[i]=addresses[i];original_[i]=Word(b,addresses[i]);Put(b,addresses[i],values[i]);}
            return;
        }
    }
    if(hud->scale_==1)return;
    address_[0]=a;original_[0]=Word(b,a);Put(b,a,trial_view::HudX(Real(b,a),hud->scale_));
    if(z!=a){address_[1]=z;original_[1]=Word(b,z);Put(b,z,trial_view::HudX(Real(b,z),hud->scale_));}
}
TrialHudQuadScope::~TrialHudQuadScope(){
    for(unsigned i=0;i<4;++i)if(address_[i])std::memcpy(base_+address_[i],&original_[i],4);
}

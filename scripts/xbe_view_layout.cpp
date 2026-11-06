#include "xbe_view_layout.h"
#include "xbe_graphics_settings.h"
#include "cpu/x86_ops.h"
#include "kernel/memory.h"
#include "kernel/kernel.h"
#include "xbe_input_memory.h"
#include "xbe_coop.h"
#include "xbe_coop_guest.h"
#include "xbe_coop_cinematics.h"
#include <cstring>
#include <atomic>
#include <xmmintrin.h>

namespace {
struct PlayerView {
    std::atomic<uint32_t> owner{0},id{0},camera{0},ladder_model{0};
    std::atomic<uint64_t> weapon_effect_scale{0};
};
PlayerView players[4];
thread_local uint32_t view_player=0;
thread_local unsigned menu_depth=0;
thread_local float menu_aspect=16.f/9.f;
thread_local TrialHudLayoutScope* hud=nullptr;
thread_local TrialReticleScope* reticle=nullptr;
uint32_t Word(const uint8_t* b,uint32_t a){uint32_t v;std::memcpy(&v,b+a,4);return v;}
float Real(const uint8_t* b,uint32_t a){float v;std::memcpy(&v,b+a,4);return v;}
void Put(uint8_t* b,uint32_t a,float v){std::memcpy(b+a,&v,4);}
struct Arithmetic {unsigned csr=_mm_getcsr();Arithmetic(){_mm_setcsr(0x1f80);}~Arithmetic(){_mm_setcsr(csr);}};
PlayerView* LivePlayer(const uint8_t* b,uint32_t owner){
    if(!b||!TrialGuestReadable(owner,0x36c))return nullptr;
    const uint32_t port=Word(b,owner+0x368);
    if(port>=4)return nullptr;
    auto& p=players[port];
    return p.owner.load()==owner&&Word(b,owner+0x54)==p.id.load()&&
        Word(b,owner+0x308)==p.camera.load()?&p:nullptr;
}
unsigned SettingsPlayer(const uint8_t* b,uint32_t owner){
    return trial_coop::Active()&&LivePlayer(b,owner)&&Word(b,owner)==0x46fbe8&&
        !(Word(b,owner+0x5c)&1)&&Word(b,owner+0x368)==1&&TrialCoopPlayer(1)==owner?1u:0u;
}
float ReticleAimU(const uint8_t* b,const X86Context& c,uint32_t root){
    if(!root||!TrialGuestReadable(c.esp,28))return 0;
    const uint32_t uv0=Word(b,c.esp+20),uv1=Word(b,c.esp+24);
    if(!TrialGuestReadable(uv0,8)||!TrialGuestReadable(uv1,8)||
       Real(b,uv0)!=0||Real(b,uv0+4)!=0||Real(b,uv1)!=1||Real(b,uv1+4)!=1||
       Real(b,c.esi+0x10)!=0||Real(b,c.esi+0x14)!=0||
       Real(b,c.esi+0x18)!=1||Real(b,c.esi+0x1c)!=1)return 0;
    for(uint32_t point:{Word(b,c.esp+4),Word(b,c.esp+8)})
        for(uint32_t uv:{uv0,uv1})if(uint64_t(point)+8>uv&&uint64_t(uv)+8>point)return 0;
    // 73790 retains each weapon's fade owner and the separate sniper fade.
    // Original and reconstructed HD art share these normalized aiming texels.
    for(unsigned i=0;i<28;++i){
        if(i==0||i==6||i==9||i==25||i==26)continue; // Empty slots or diffuse soft-dot artwork.
        const uint32_t fade=Word(b,root+0x20+i*4);
        if(!TrialGuestReadable(fade,8)||Word(b,fade)!=0x453d38u||Word(b,fade+4)!=c.esi)continue;
        const float extent=i==27?128.f:64.f;
        if(Real(b,c.esi+8)!=extent||Real(b,c.esi+12)!=extent)return 0;
        return (extent*.5f+.5f)/extent;
    }
    return 0;
}
}

float TrialRootAspect(const X86Context& c,const uint8_t* b){
    if(!b||!TrialGuestReadable(c.esp,28))return 4.f/3.f;
    const float original=Real(b,c.esp+24);
    // Only the display root. Derived split/cinematic viewports inherit its pixel aspect.
    if(Word(b,c.esp)!=0x8bf4au||Word(b,c.esp+4)||Real(b,c.esp+8)!=0||
       Real(b,c.esp+12)!=0||Real(b,c.esp+16)!=480||Real(b,c.esp+20)!=640)return original;
    return trial_graphics::AspectRatio();
}
uint32_t TrialDisplayAspectBits(){
    const float aspect=trial_graphics::AspectRatio();
    uint32_t bits;std::memcpy(&bits,&aspect,4);return bits;
}
TrialMenuLayoutScope::TrialMenuLayoutScope(float aspect):previous_(menu_aspect){++menu_depth;menu_aspect=aspect;}
TrialMenuLayoutScope::~TrialMenuLayoutScope(){menu_aspect=previous_;--menu_depth;}
void TrialPlayerCameraCreated(const X86Context& c,const uint8_t* b){
    if(!b||!TrialGuestReadable(c.esi,0x36c))return;
    const uint32_t port=Word(b,c.esi+0x368),id=Word(b,c.esi+0x54);
    if(port>=4)return;
    auto& p=players[port];p.owner=0;p.ladder_model=0;p.weapon_effect_scale=0;
    if(c.eax==0xffffffffu||id==0xffffffffu)return;
    p.id=id;p.camera=c.eax;p.owner=c.esi;
}
TrialPlayerViewScope::TrialPlayerViewScope():previous_(view_player){view_player=0;}
TrialPlayerViewScope::~TrialPlayerViewScope(){view_player=previous_;}
void TrialPlayerViewScope::Select(const X86Context& c,const uint8_t* b){view_player=LivePlayer(b,c.ecx)?c.ecx:0;}
void TrialPlayerViewScope::Clear(){view_player=0;}
float TrialCameraTarget(const X86Context& c,const uint8_t* b){
    if(!TrialGuestReadable(c.ecx,0x230))return 0;
    const float target=Real(b,c.ecx+0x228);
    if(Word(b,c.ecx+0x210)!=2)return target;
    // Match both live camera and parent IDs; cinematic/other follow cameras are excluded.
    for(auto& p:players){
        const uint32_t owner=p.owner.load();
        if(!LivePlayer(b,owner)||Word(b,owner+0x308)!=Word(b,c.ecx+0x54)||
           Word(b,owner+0x54)!=Word(b,c.ecx+0x58)||Word(b,c.ecx+0x22c)!=Word(b,owner+0x54))continue;
        const unsigned fov=trial_graphics::FieldOfView(SettingsPlayer(b,owner));
        if(fov==53)return target;
        Arithmetic arithmetic;
        // Change only this update's input, never the authored target or its zoom events.
        return trial_view::Zoom(target,fov);
    }
    return target;
}
void TrialLadderHands(const X86Context& c,const uint8_t* b){
    // CDCA8 receives the ladder's arm model just before setting its depth to 0..1.
    auto* p=LivePlayer(b,c.esi);if(!p)return;
    p->ladder_model=Word(b,c.esi+0x170)==11&&
        TrialGuestReadable(c.eax,0x118)&&Word(b,c.eax)==0x457880u?
        c.eax:0;
}
void TrialWeaponProjection(const X86Context& c,uint8_t* b){
    if(!b||
       !TrialGuestReadable(c.esi,0x118)||Word(b,c.esi)!=0x457880u||
       (Word(b,c.esi+0x60)&0x180u)!=0x180u||
       !TrialGuestReadable(c.edi,0x194)||Word(b,c.edi)!=0x456d88u||
       c.ecx!=c.edi||!TrialGuestReadable(c.esp,0x94))return;
    const uint32_t matrix=Word(b,c.esp);
    if(matrix!=c.esp+0x54)return;
    // C8930 assigns this alternate view/depth range to gun, arms and flashlight.
    // Change only BAC10's local copy; its original restore still owns world state.
    Arithmetic arithmetic;
    for(unsigned i=0;i<16;++i)if(!std::isfinite(Real(b,matrix+i*4)))return;
    auto* p=LivePlayer(b,view_player);
    if(p&&c.esi==p->ladder_model.load()&&Word(b,view_player+0x170)==11&&
       Word(b,c.esi+0x110)==0&&Word(b,c.esi+0x114)==0x3f800000u){
        // Hands touch world geometry: use the live world framing, not the gun's independent FOV.
        const float x=Real(b,c.edi+0xd0),y=Real(b,c.edi+0xe4);
        if(std::isfinite(x)&&std::isfinite(y)&&x>0&&y>0){Put(b,matrix,x);Put(b,matrix+20,y);}
        return;
    }
    const unsigned percent=trial_graphics::WeaponFovScale(SettingsPlayer(b,view_player));
    if(percent<50||percent>150||
       Word(b,c.esi+0x110)!=0x3dcccccdu||Word(b,c.esi+0x114)!=0x3e19999au)return;
    const float x=Real(b,matrix),y=Real(b,matrix+20);
    if(x<=0||y<=0)return;
    const float scale=100.f/percent,scaled_x=x*scale,scaled_y=y*scale;
    if(!std::isfinite(scaled_x)||!std::isfinite(scaled_y))return;
    Put(b,matrix,scaled_x);Put(b,matrix+20,scaled_y);
    const float world_x=Real(b,c.edi+0xd0),world_y=Real(b,c.edi+0xe4);
    if(p&&std::isfinite(world_x)&&std::isfinite(world_y)&&world_x>0&&world_y>0){
        const float ratios[]={scaled_x/world_x,scaled_y/world_y};uint64_t bits;
        std::memcpy(&bits,ratios,sizeof(bits));p->weapon_effect_scale=bits;
    }
}
TrialTaserVertices::TrialTaserVertices(const X86Context& c,uint8_t* b):base_(b){
    Arithmetic arithmetic;
    const uint32_t owner=view_player;auto* p=LivePlayer(b,owner);
    if(!b||c.ecx<0x80||!TrialGuestReadable(c.ecx-0x80,0x300)||
       Word(b,c.ecx-0x80)!=0x470058u||!p||!TrialGuestReadable(owner,0x820)||
       (Word(b,owner+0x4fc)!=1&&Word(b,owner+0x4fc)!=15)||!TrialGuestReadable(c.esp,28))return;
    const uint32_t id=Word(b,c.ecx-0x2c);
    if(id==0xffffffffu||(id!=Word(b,owner+0x814)&&id!=Word(b,owner+0x818)&&id!=Word(b,owner+0x81c)))return;
    const uint64_t bits=p->weapon_effect_scale.load();if(!bits)return;
    std::memcpy(scale_,&bits,sizeof(bits));
    for(float v:scale_)if(!std::isfinite(v)||v<=0||v>8)return;
    const uint32_t first=Word(b,c.esp+4),count=Word(b,c.esp+8),camera=Word(b,c.esp+12),vertices=Word(b,c.esp+16);
    if(first>=40||!count||count>40-first||!TrialGuestReadable(camera,64)||!TrialGuestReadable(vertices,count*36))return;
    std::memcpy(camera_,b+camera,64);
    for(float v:camera_)if(!std::isfinite(v))return;
    // The beam builder uses this camera-to-world matrix to face its ribbon.
    for(unsigned row:{0u,4u}){
        const float length=camera_[row]*camera_[row]+camera_[row+1]*camera_[row+1]+camera_[row+2]*camera_[row+2];
        if(std::abs(length-1.f)>.001f)return;
    }
    if(std::abs(camera_[0]*camera_[4]+camera_[1]*camera_[5]+camera_[2]*camera_[6])>.001f)return;
    vertices_=vertices;count_=count*3;
}
TrialTaserVertices::~TrialTaserVertices(){
    if(!vertices_||(scale_[0]==1&&scale_[1]==1))return;
    Arithmetic arithmetic;
    for(unsigned i=0;i<count_*3;++i)if(!std::isfinite(Real(base_,vertices_+i*4)))return;
    // Reframe only emitted ribbon vertices; authored endpoints and hit logic remain untouched.
    for(unsigned i=0;i<count_;++i){
        const uint32_t p=vertices_+i*12;
        const float delta[]={Real(base_,p)-camera_[12],Real(base_,p+4)-camera_[13],Real(base_,p+8)-camera_[14]};
        float right=0,up=0;
        for(unsigned j=0;j<3;++j){right+=delta[j]*camera_[j];up+=delta[j]*camera_[4+j];}
        right*=scale_[0]-1;up*=scale_[1]-1;
        for(unsigned j=0;j<3;++j)Put(base_,p+j*4,Real(base_,p+j*4)+right*camera_[j]+up*camera_[4+j]);
    }
}
TrialHudLayoutScope::TrialHudLayoutScope(const X86Context& c,const uint8_t* b):previous_(hud){
    hud=this;
    const bool coop=trial_coop::Active();
    if(!b||(coop&&TrialCoopPresentation()!=1)||
       !TrialGuestReadable(c.ecx,0x18)||Word(b,c.ecx)!=0x4531a0)return;
    const uint32_t owner=Word(b,c.ecx+4);
    if(!LivePlayer(b,owner)||Word(b,owner)!=0x46fbe8||
       (Word(b,owner+0x5c)&1)||Word(b,owner+0x300)!=c.ecx)return;
    const uint32_t slot=Word(b,owner+0x368);
    if(slot>1||(coop?TrialCoopPlayer(slot)!=owner:slot!=0))return;
    root_=c.ecx;slot_=slot;
}
TrialHudLayoutScope::~TrialHudLayoutScope(){hud=previous_;}
void TrialScopeBorders(X86Context& c,uint8_t* b){
    if(!hud||hud->scale_==1||!TrialGuestReadable(c.esi,12)||Word(b,c.esi)!=0x4530bcu||
       !TrialGuestReadable(c.edi,32)||!TrialGuestReadable(c.esp,32)||c.esp<0x10200)return;
    Arithmetic arithmetic;
    // The child mask uses the descriptor; the stack color only dims the scene behind it.
    const float width=640/hud->scale_,height=480,alpha=Real(b,c.edi+28);
    const float left=Real(b,c.edi),top=Real(b,c.edi+4),right=Real(b,c.edi+8),bottom=Real(b,c.edi+12);
    // Retail safe-area containers inset the descriptor; the centered mask still needs screen-edge coverage.
    if(!std::isfinite(left)||!std::isfinite(top)||!std::isfinite(right)||!std::isfinite(bottom)||
       left<0||top<0||right<=left||bottom<=top||right>width||bottom>height||
       std::abs(left+right-width)>.01f||std::abs(top+bottom-height)>.01f||
       !std::isfinite(alpha)||alpha<=0||alpha>1)return;
    const uint32_t scratch=(c.esp-128)&~15u;
    if(!TrialGuestReadable(scratch-32,160))return;
    const X86Context saved=c;const float edge=(width-640)*.5f;
    // Extend only the opaque outside of the circular retail mask, with its fade.
    for(unsigned side=0;side<2;++side){
        Put(b,scratch,side?width-edge:0);Put(b,scratch+4,0);
        Put(b,scratch+8,side?std::ceil(width):edge);Put(b,scratch+12,height);
        const uint32_t color=uint32_t(alpha*255.f+.5f)<<24;
        std::memcpy(b+scratch+16,&color,4);c.esp=scratch;
        kernel::CallGuest(c,0x656d0,{scratch,scratch+8,scratch+16});
    }
    c=saved;
}
void TrialHudLayout(const X86Context& c,uint8_t* b){
    if(!hud||c.esp>0xffffffabu||!TrialGuestReadable(c.esp+0x10,0x44))return;
    const uint32_t d=c.esp+0x10;
    const float top=float(hud->slot_)*240;
    hud->split_=trial_coop::Active()&&hud->root_&&Real(b,d)==0&&Real(b,d+4)==top&&
        Real(b,d+8)==640&&Real(b,d+12)==top+240;
    if(!hud->split_&&(Real(b,d)!=0||Real(b,d+4)!=0||Real(b,d+8)!=640||Real(b,d+12)!=480)){hud->root_=0;return;}
    Arithmetic arithmetic;hud->scale_=trial_graphics::HudScale();Put(b,d+8,640/hud->scale_);
}
TrialHudInsetScope::TrialHudInsetScope(const X86Context& c,uint8_t* b):base_(b),layout_(hud),notification_(hud?hud->notification_:false){
    if(!hud||!hud->split_||!TrialGuestReadable(c.ecx,0x1c)||Word(b,c.ecx)!=0x453cd0)return;
    const uint32_t notices=Word(b,hud->root_+0x10),main=Word(b,hud->root_+8);
    uint32_t corner=0;
    // 70D90: main = pair(pair(inset(four HUD children),aligned timer),six).
    if(TrialGuestReadable(main,12)&&Word(b,main)==0x453bd0){
        const uint32_t pair=Word(b,main+4);
        if(TrialGuestReadable(pair,12)&&Word(b,pair)==0x453bd0)corner=Word(b,pair+4);
    }
    if(c.ecx==notices){
        const uint32_t pair=Word(b,c.ecx+8);
        if(!TrialGuestReadable(pair,12)||Word(b,pair)!=0x453bd0)return;
        const uint32_t list=Word(b,pair+4),rewards=Word(b,pair+8);
        if(!TrialGuestReadable(list,4)||Word(b,list)!=0x453018||
           !TrialGuestReadable(rewards,4)||Word(b,rewards)!=0x45356c)return;
        notification_=hud->notification_;hud->notification_=true;return;
    }
    if(c.ecx!=corner||Real(b,c.ecx+12)!=48||Real(b,c.ecx+16)!=48||
       Real(b,c.ecx+20)!=36||Real(b,c.ecx+24)!=36)return;
    margins_=c.ecx+12;
    std::memcpy(saved_,b+margins_,sizeof saved_);
    Put(b,margins_,16);Put(b,margins_+4,16);Put(b,margins_+8,12);Put(b,margins_+12,12);
}
TrialHudInsetScope::~TrialHudInsetScope(){
    if(margins_)std::memcpy(base_+margins_,saved_,sizeof saved_);
    if(layout_)layout_->notification_=notification_;
}
TrialHudHelpScope::TrialHudHelpScope(const X86Context& c,const uint8_t* b):layout_(hud),previous_(hud?hud->help_:false){
    if(!hud)return;
    hud->help_=false;
    if(!b||!hud->root_||hud->notification_||c.esp>0xfffffffbu||!TrialGuestReadable(c.esp+4,4))return;
    // 70D90 attaches exactly one help queue to the six-child branch of this
    // actor's HUD. An unrelated queue must not inherit the enclosing owner.
    const uint32_t main=Word(b,hud->root_+8);
    if(!TrialGuestReadable(main,12)||Word(b,main)!=0x453bd0)return;
    const uint32_t six=Word(b,main+8);
    if(!TrialGuestReadable(six,8)||Word(b,six)!=0x453c20||Word(b,six+4)!=c.ecx||
       !TrialGuestReadable(c.ecx,0x14)||Word(b,c.ecx)!=0x453580)return;
    const uint32_t d=Word(b,c.esp+4);
    if(!TrialGuestReadable(d,16))return;
    Arithmetic arithmetic;
    const float top=Real(b,d+4),bottom=Real(b,d+12),origin=hud->split_?float(hud->slot_)*240:0;
    const float height=hud->split_?240.f:480.f;
    if(!std::isfinite(top)||!std::isfinite(bottom)||top<origin||bottom>origin+height||bottom<=top)return;
    hud->help_=true;
}
TrialHudHelpScope::~TrialHudHelpScope(){if(layout_)layout_->help_=previous_;}
uint32_t TrialHudCenterBits(){
    Arithmetic arithmetic;float center=320/(hud?hud->scale_:1);uint32_t bits;
    std::memcpy(&bits,&center,4);return bits;
}
TrialProjectedHudScope::TrialProjectedHudScope(const X86Context& c,uint8_t* b):previous_(hud),base_(b){
    // 73700 builds the OPITEMS bracket owner; 75B90 replaces its descriptor
    // bounds with 47610/470A0 camera projections before drawing the four corners.
    // These are already screen positions, not positions on our wider HUD canvas.
    const bool scope_tint=TrialGuestReadable(c.esp,4)&&Word(b,c.esp)==0x73256u&&
        TrialGuestReadable(c.esi,12)&&Word(b,c.esi)==0x4530bcu;
    if(hud&&hud->split_&&TrialGuestReadable(c.ecx,12)&&Word(b,c.ecx)==0x4530bcu){
        // 731E0's centered mask predates the wider HUD canvas. Keep its
        // original split-view descriptor, then restore the caller's canvas.
        // Disabling HudX alone would leave the mask centered at X426.67.
        if(!b||c.esp>0xfffffffbu||!TrialGuestReadable(c.esp+4,4)||
           Word(b,c.ecx+4)!=Word(b,hud->root_+4))return;
        const uint32_t d=Word(b,c.esp+4);
        if(!TrialGuestReadable(d,16))return;
        Arithmetic arithmetic;
        const float left=Real(b,d),right=Real(b,d+8),top=Real(b,d+4),bottom=Real(b,d+12);
        const float width=640/hud->scale_,origin=float(hud->slot_)*240;
        if(!std::isfinite(left)||!std::isfinite(right)||!std::isfinite(top)||!std::isfinite(bottom)||left<0||right>width||
           right<=left||std::abs(left+right-width)>.01f||
           top<origin||bottom>origin+240||bottom<=top||right-(width-640)<=left)return;
        descriptor_right_=d+8;saved_right_=Word(b,descriptor_right_);
        Put(b,descriptor_right_,right-(width-640));hud=nullptr;return;
    }
    // The scope's fullscreen tint also uses viewport coordinates, not the wider HUD canvas.
    if(hud&&hud->scale_!=1&&(scope_tint||
       (TrialGuestReadable(c.ecx,0x58)&&Word(b,c.ecx)==0x45312cu)))
        hud=nullptr;
}
TrialProjectedHudScope::~TrialProjectedHudScope(){
    if(descriptor_right_)std::memcpy(base_+descriptor_right_,&saved_right_,4);
    hud=previous_;
}
void TrialReticleColor(const X86Context& c,uint8_t* b){
    const uint32_t root=c.esi;
    if(!b||!TrialGuestReadable(root,0x20)||Word(b,root)!=0x453160u)return;
    const uint32_t owner=Word(b,root+4);
    if(!LivePlayer(b,owner)||Word(b,owner)!=0x46fbe8u||(Word(b,owner+0x5c)&1))return;
    const uint32_t r=Word(b,root+0x10),g=Word(b,root+0x14),blue=Word(b,root+0x18),one=0x3f800000;
    // The native update has already chosen target/spin priority for this owner.
    const unsigned state=r==one&&g==one&&blue==one?0:r==one&&!g&&!blue?1:
        !r&&g==one&&!blue?2:!r&&g==one&&blue==one?3:4;
    const auto choice=trial_graphics::ReticleColor(SettingsPlayer(b,owner),state);
    if(!choice||choice>=std::size(trial_graphics::ReticlePalette))return;
    Arithmetic arithmetic;const uint32_t rgb=trial_graphics::ReticlePalette[choice];
    Put(b,root+0x10,float((rgb>>16)&255)/255);
    Put(b,root+0x14,float((rgb>>8)&255)/255);
    Put(b,root+0x18,float(rgb&255)/255);
}
TrialReticleScope::TrialReticleScope(const X86Context& c,const uint8_t* b):previous_(reticle){
    reticle=nullptr;
    const unsigned percent=trial_graphics::ReticleScale();
    // 0x73790 builds this class from weapon crosshair shaders. The sniper
    // mask/brackets use different classes and never enter its 0x73aa0 draw.
    if(!hud||percent<50||percent>200||
       !TrialGuestReadable(c.ecx,0x94)||Word(b,c.ecx)!=0x453160u||
       !TrialGuestReadable(Word(b,c.ecx+0x90),4))return;
    if(hud->root_&&Word(b,c.ecx+4)==Word(b,hud->root_+4))root_=c.ecx;
    if(percent==100&&!root_)return;
    Arithmetic arithmetic;scale_=float(percent)*.01f;reticle=this;
}
TrialReticleScope::~TrialReticleScope(){reticle=previous_;}
TrialHudQuadScope::TrialHudQuadScope(const X86Context& c,uint8_t* b):base_(b){
    if(!hud&&menu_depth&&TrialGuestReadable(c.esp,12)){
        Arithmetic arithmetic;
        const float scale=std::min(1.f,menu_aspect/trial_graphics::AspectRatio());
        if(scale==1)return;
        const uint32_t a=Word(b,c.esp+4),z=Word(b,c.esp+8);
        if(!TrialGuestReadable(a,8)||!TrialGuestReadable(z,8)||
           !std::isfinite(Real(b,a))||!std::isfinite(Real(b,z)))return;
        address_[0]=a;original_[0]=Word(b,a);Put(b,a,319.5f+(Real(b,a)-319.5f)*scale);
        if(z!=a){address_[1]=z;original_[1]=Word(b,z);Put(b,z,319.5f+(Real(b,z)-319.5f)*scale);}
        return;
    }
    if(!hud||(hud->scale_==1&&!reticle&&!hud->notification_&&!hud->help_)||!TrialGuestReadable(c.esp,12))return;
    const uint32_t a=Word(b,c.esp+4),z=Word(b,c.esp+8);
    if(!TrialGuestReadable(a,8)||!TrialGuestReadable(z,8)||
       !std::isfinite(Real(b,a))||!std::isfinite(Real(b,z)))return;
    Arithmetic arithmetic;
    if(hud->help_){
        if((uint64_t(a)+8>z&&uint64_t(z)+8>a)||
           !std::isfinite(Real(b,a+4))||!std::isfinite(Real(b,z+4)))return;
        // 77570 discards the descriptor: retain each help line's offset from
        // the view center, as the descriptor-positioned order icons already do.
        const float dx=320/hud->scale_-320,dy=hud->split_?float(hud->slot_)*240-120:0;
        const uint32_t addresses[]={a,a+4,z,z+4};
        const float values[]={trial_view::HudX(Real(b,a)+dx,hud->scale_),Real(b,a+4)+dy,
            trial_view::HudX(Real(b,z)+dx,hud->scale_),Real(b,z+4)+dy};
        for(unsigned i=0;i<4;++i){address_[i]=addresses[i];original_[i]=Word(b,addresses[i]);Put(b,addresses[i],values[i]);}
        return;
    }
    if(hud->notification_){
        if((uint64_t(a)+8>z&&uint64_t(z)+8>a)||
           !std::isfinite(Real(b,a+4))||!std::isfinite(Real(b,z+4)))return;
        // Retail reward panels occupy X=210..510 and start at Y=83.
        // Translate the whole animated branch, retaining relative text/icon
        // positions and fades. Unlike descriptor-positioned HUD children,
        // 77450/72200 discard the descriptor; their tweens use absolute local
        // coordinates, so the lower owner's origin must be added here.
        const float dx=320/hud->scale_-360,dy=float(hud->slot_)*240+42-83;
        const uint32_t addresses[]={a,a+4,z,z+4};
        const float values[]={trial_view::HudX(Real(b,a)+dx,hud->scale_),Real(b,a+4)+dy,
            trial_view::HudX(Real(b,z)+dx,hud->scale_),Real(b,z+4)+dy};
        for(unsigned i=0;i<4;++i){address_[i]=addresses[i];original_[i]=Word(b,addresses[i]);Put(b,addresses[i],values[i]);}
        return;
    }
    // Only the final sprite quad emitted by 0x7afe0 inside the reticle tree.
    // Resize about its authored center after all weapon placement/fade work;
    // leave UVs, spread/position inputs, visibility and scope masks untouched.
    if(reticle&&Word(b,c.esp)==0x7b149u&&TrialGuestReadable(c.esi,0x20)&&
       Word(b,c.esi)==0x453b50u&&(uint64_t(a)+8<=z||uint64_t(z)+8<=a)&&
       std::isfinite(Real(b,a+4))&&std::isfinite(Real(b,z+4))){
        float x0=Real(b,a),x1=Real(b,z),y0=Real(b,a+4),y1=Real(b,z+4);
        if(reticle->scale_!=1){
            const float half_change=(reticle->scale_-1)*.5f;
            const float dx=(x1-x0)*half_change,dy=(y1-y0)*half_change;
            x0-=dx;x1+=dx;y0-=dy;y1+=dy;
        }
        if(hud->scale_!=1){x0=trial_view::HudX(x0,hud->scale_);x1=trial_view::HudX(x1,hud->scale_);}
        const float aim_u=ReticleAimU(b,c,reticle->root_);
        if(aim_u&&x1>x0&&y1>y0){
            // Each validated player viewport is 640 wide. Center the visible
            // aiming texel, not the quad midpoint; retain native vertical aim.
            const float dx=320-(x0+(x1-x0)*aim_u);x0+=dx;x1+=dx;
        }
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

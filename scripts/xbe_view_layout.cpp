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
std::atomic<uint64_t> ladder_hands{0};
std::atomic<uint64_t> weapon_effect_scale{0};
thread_local TrialHudLayoutScope* hud=nullptr;
thread_local TrialReticleScope* reticle=nullptr;
uint32_t Word(const uint8_t* b,uint32_t a){uint32_t v;std::memcpy(&v,b+a,4);return v;}
float Real(const uint8_t* b,uint32_t a){float v;std::memcpy(&v,b+a,4);return v;}
void Put(uint8_t* b,uint32_t a,float v){std::memcpy(b+a,&v,4);}
struct Arithmetic {unsigned csr=_mm_getcsr();Arithmetic(){_mm_setcsr(0x1f80);}~Arithmetic(){_mm_setcsr(csr);}};
}

void TrialPlayerCameraCreated(const X86Context& c,const uint8_t* b){
    ladder_hands=0;
    weapon_effect_scale=0;
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
void TrialLadderHands(const X86Context& c,const uint8_t* b){
    // CDCA8 receives the ladder's arm model just before setting its depth to 0..1.
    ladder_hands=b&&TrialGuestReadable(c.esi,0x178)&&Word(b,c.esi+0x170)==11&&
        TrialGuestReadable(c.eax,0x118)&&Word(b,c.eax)==0x457880u?
        (uint64_t(c.esi)<<32)|c.eax:0;
}
void TrialWeaponProjection(const X86Context& c,uint8_t* b){
    const unsigned percent=trial_graphics::WeaponFovScale();
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
    const uint64_t hands=ladder_hands.load();
    const uint32_t controller=uint32_t(hands>>32);
    if(c.esi==uint32_t(hands)&&TrialGuestReadable(controller,0x178)&&
       Word(b,controller+0x170)==11&&Word(b,c.esi+0x110)==0&&Word(b,c.esi+0x114)==0x3f800000u){
        // Hands touch world geometry: use the live world framing, not the gun's independent FOV.
        const float x=Real(b,c.edi+0xd0),y=Real(b,c.edi+0xe4);
        if(std::isfinite(x)&&std::isfinite(y)&&x>0&&y>0){Put(b,matrix,x);Put(b,matrix+20,y);}
        return;
    }
    if(percent<50||percent>150||
       Word(b,c.esi+0x110)!=0x3dcccccdu||Word(b,c.esi+0x114)!=0x3e19999au)return;
    const float x=Real(b,matrix),y=Real(b,matrix+20);
    if(x<=0||y<=0)return;
    const float scale=100.f/percent,scaled_x=x*scale,scaled_y=y*scale;
    if(!std::isfinite(scaled_x)||!std::isfinite(scaled_y))return;
    Put(b,matrix,scaled_x);Put(b,matrix+20,scaled_y);
    const float world_x=Real(b,c.edi+0xd0),world_y=Real(b,c.edi+0xe4);
    if(std::isfinite(world_x)&&std::isfinite(world_y)&&world_x>0&&world_y>0){
        const float ratios[]={scaled_x/world_x,scaled_y/world_y};uint64_t bits;
        std::memcpy(&bits,ratios,sizeof(bits));weapon_effect_scale=bits;
    }
}
TrialTaserVertices::TrialTaserVertices(const X86Context& c,uint8_t* b):base_(b){
    Arithmetic arithmetic;
    const uint32_t owner=player.load();
    if(!b||c.ecx<0x80||!TrialGuestReadable(c.ecx-0x80,0x300)||
       Word(b,c.ecx-0x80)!=0x470058u||!TrialGuestReadable(owner,0x820)||
       (Word(b,owner+0x4fc)!=1&&Word(b,owner+0x4fc)!=15)||!TrialGuestReadable(c.esp,28))return;
    const uint32_t id=Word(b,c.ecx-0x2c);
    if(id==0xffffffffu||(id!=Word(b,owner+0x814)&&id!=Word(b,owner+0x818)&&id!=Word(b,owner+0x81c)))return;
    const uint64_t bits=weapon_effect_scale.load();if(!bits)return;
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
TrialHudLayoutScope::TrialHudLayoutScope(const X86Context&,const uint8_t*):previous_(hud){hud=this;}
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
    const bool scope_tint=TrialGuestReadable(c.esp,4)&&Word(b,c.esp)==0x73256u&&
        TrialGuestReadable(c.esi,12)&&Word(b,c.esi)==0x4530bcu;
    // The scope's fullscreen tint also uses viewport coordinates, not the wider HUD canvas.
    if(hud&&hud->scale_!=1&&(scope_tint||
       (TrialGuestReadable(c.ecx,0x58)&&Word(b,c.ecx)==0x45312cu)))
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

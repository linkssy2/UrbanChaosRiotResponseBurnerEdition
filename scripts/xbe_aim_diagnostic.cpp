#include "xbe_aim_diagnostic.h"
#ifndef UC_BURNER_RELEASE
#include "xbe_coop.h"
#include "xbe_coop_guest.h"
#include "cpu/x86_ops.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <xmmintrin.h>

namespace aim_diagnostic {
constexpr unsigned kShotLimit=64,kFrameLimit=4,kQuadLimit=32;
struct Player {
    uint32_t owner=0,id=0,shot=0,frames=0,quads=0;
    uint64_t render=0;
    uint32_t camera=0,viewport=0;
    uint32_t admitted[2]{},layers[2]{},layer_owner[2]{},layer_tag[2]{},layer_viewport[2]{},camera_id=0,camera_owner_id=0;
    std::array<float,64> draw{};
    std::array<float,4> bounds{};
    bool viewport_valid=false;
};
std::array<Player,4> players;
struct Bullet {uint32_t object=0,id=0,owner=0,owner_id=0,shot=0;};
std::array<Bullet,kShotLimit> bullets;
thread_local uint32_t shot_owner=0,reticle_owner=0;
thread_local TrialAimFireScope* fire=nullptr;
unsigned fire_calls=0,rejected=0;
unsigned shots=0,lines=0;
uint64_t render_sequence=0;
struct Arithmetic {unsigned csr=_mm_getcsr();~Arithmetic(){_mm_setcsr(csr);}};
uint32_t Word(const uint8_t* b,uint32_t p){uint32_t v;std::memcpy(&v,b+p,4);return v;}
float Real(const uint8_t* b,uint32_t p){float v;std::memcpy(&v,b+p,4);return v;}
bool Enabled(){static const bool enabled=[](){const char* p=std::getenv("BURNER_AIM_LOG");return p&&*p;}();return enabled&&trial_coop::Available();}
FILE* File(){
    static FILE* f=[](){const char* p=std::getenv("BURNER_AIM_LOG");return p&&*p?std::fopen(p,"wb"):nullptr;}();
    return f;
}
FILE* Begin(const char* kind){
    if(!Enabled()||lines>=10000)return nullptr;
    FILE* f=File();if(!f)return nullptr;++lines;
    const auto now=std::chrono::steady_clock::now().time_since_epoch();
    std::fprintf(f,"%s tick_ms=%llu",kind,(unsigned long long)std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
    return f;
}
void End(FILE* f){std::fputc('\n',f);std::fflush(f);}
uint32_t SafeWord(const uint8_t* b,uint32_t p){return b&&TrialGuestReadable(p,4)?Word(b,p):0xffffffffu;}
uint32_t Field(const uint8_t* b,uint32_t p,uint32_t offset){
    return p&&p<=0xffffffffu-offset?SafeWord(b,p+offset):0xffffffffu;
}
void Identity(FILE* f,const uint8_t* b,uint32_t owner){
    std::fprintf(f," owner=%08X vtable=%08X id=%u slot=%u flags=%08X camera_id=%u admitted0=%08X admitted1=%08X",
        owner,SafeWord(b,owner),Field(b,owner,0x54),Field(b,owner,0x368),Field(b,owner,0x5c),
        Field(b,owner,0x308),TrialCoopPlayer(0),TrialCoopPlayer(1));
}
void Bytes(FILE* f,const uint8_t* b,const char* name,uint32_t p,uint32_t count){
    std::fprintf(f," %s=",name);
    if(!b||!TrialGuestReadable(p,count)){std::fputs("unreadable",f);return;}
    for(uint32_t i=0;i<count;++i)std::fprintf(f,"%02X",b[p+i]);
}
void Inputs(FILE* f,const uint8_t* b){
    const uint32_t mapping=SafeWord(b,0x5096e0),world=SafeWord(b,0x51c4dc),states=Field(b,world,0x20);
    std::fprintf(f," input_map=%08X input_states=%08X preset0=%u preset1=%u",
        mapping,states,Field(b,world,0x948),Field(b,world,0x94c));
    Bytes(f,b,"input_map0c18",mapping<=0xffffffe7u?mapping+0xc:0,13);
    Bytes(f,b,"input_state0",states,0xd0);
    Bytes(f,b,"input_state1",states<=0xffffff2fu?states+0xd0:0,0xd0);
    const uint32_t devices=Field(b,world,0x24);
    for(unsigned i=0;i<2;++i){
        const uint32_t device=Field(b,devices,4*i);
        std::fprintf(f," device%u=%08X device%u_world=%08X device%u_port=%u",
            i,device,i,Field(b,device,8),i,Field(b,device,12));
        Bytes(f,b,i?"device1_trigger":"device0_trigger",device<=0xffffff8bu?device+0x74:0,12);
        const uint32_t binding=Field(b,device,4),source=Field(b,binding,0x14);
        const uint32_t values=Field(b,binding,0x40),flags=Field(b,binding,0x44);
        std::fprintf(f," binding%u=%08X binding%u_selector=%u binding%u_source=%08X binding%u_count=%u binding%u_fire_bits=%08X",
            i,binding,i,Field(b,binding,4),i,source,i,Field(b,binding,0x18),i,Field(b,values,11*4));
        Bytes(f,b,i?"binding1_fire_flag":"binding0_fire_flag",flags<=0xfffffff4u?flags+11:0,1);
        // Follow only the original XInput source type, not a generic mapper node.
        const bool pad=b&&TrialGuestReadable(source,0x78)&&Word(b,source)==0x481768u&&Word(b,source+0x10)==0x4fd5e0u;
        std::fprintf(f," source%u_pad=%u",i,unsigned(pad));
        if(pad){
            const uint32_t state=Word(b,source+0x74);
            std::fprintf(f," source%u_port=%u source%u_index=%u source%u_trigger_bits=%08X source%u_handle=%08X source%u_packet=%u",
                i,Word(b,source+8),i,Word(b,source+12),i,Word(b,source+0x48),i,Field(b,state,0),i,Field(b,state,4));
        }
    }
}
void Rejected(const X86Context& c,const uint8_t* b,const char* reason,uint32_t request=0){
    if(rejected++>=64)return;
    Arithmetic arithmetic;
    if(FILE* f=Begin("shot-rejected")){
        std::fprintf(f," reason=%s fire=%u request=%08X argument_id=%u packet_id=%u",
            reason,fire?fire->serial_:0,request,Field(b,c.esp,4),Field(b,request,12));
        Identity(f,b,c.esi);End(f);
    }
}
void Vector(FILE* f,const char* name,const uint8_t* b,uint32_t p,unsigned count){
    std::fprintf(f," %s=",name);
    if(!TrialGuestReadable(p,count*4)){std::fprintf(f,"unreadable");return;}
    for(unsigned i=0;i<count;++i)std::fprintf(f,"%s%.9g",i?",":"",double(Real(b,p+i*4)));
}
Player* Owner(const uint8_t* b,uint32_t owner){
    if(!b||!TrialGuestReadable(owner,0x704)||Word(b,owner)!=0x46fbe8u||
       Word(b,owner+0x54)==0xffffffffu||(Word(b,owner+0x5c)&1))return nullptr;
    const uint32_t slot=Word(b,owner+0x368);if(slot>=players.size())return nullptr;
    auto& p=players[slot];const uint32_t id=Word(b,owner+0x54);
    if(p.owner!=owner||p.id!=id)p=Player{owner,id};
    return &p;
}
void Draw(FILE* f,const Player& p){
    std::fprintf(f," shot=%u owner=%08X id=%u render=%llu camera=%08X viewport=%08X",
        p.shot,p.owner,p.id,(unsigned long long)p.render,p.camera,p.viewport);
    if(p.viewport_valid)std::fprintf(f," viewport_tlbr=%.9g,%.9g,%.9g,%.9g",
        double(p.bounds[0]),double(p.bounds[1]),double(p.bounds[2]),double(p.bounds[3]));
    std::fprintf(f," camera_id=%u camera_owner_id=%u",p.camera_id,p.camera_owner_id);
    for(unsigned i=0;i<2;++i)std::fprintf(f," admitted%u=%08X layer%u=%08X layer%u_owner=%08X layer%u_tag=%u layer%u_viewport=%08X",
        i,p.admitted[i],i,p.layers[i],i,p.layer_owner[i],i,p.layer_tag[i],i,p.layer_viewport[i]);
    constexpr const char* names[]={"camera_world","camera_inverse","projection","alternate_projection"};
    for(unsigned n=0;n<4;++n){
        std::fprintf(f," %s=",names[n]);
        for(unsigned i=0;i<16;++i)std::fprintf(f,"%s%.9g",i?",":"",double(p.draw[n*16+i]));
    }
}
}

TrialAimFireScope::TrialAimFireScope(const X86Context& c,const uint8_t* b,bool resumed){
    using namespace aim_diagnostic;
    if(!Enabled())return;
    Arithmetic arithmetic;
    previous_=fire;fire=this;
    if(fire_calls>=128)return;
    serial_=++fire_calls;owner_=resumed?c.esi:c.ecx;
    if(FILE* f=Begin("fire-enter")){
        std::fprintf(f," fire=%u entry=%s item=%u return=%08X",serial_,resumed?"CAF25":"CAB20",
            resumed?c.edi:Field(b,c.esp,4),resumed?0:SafeWord(b,c.esp));
        Identity(f,b,owner_);Inputs(f,b);End(f);
    }
}
TrialAimFireScope::~TrialAimFireScope(){
    using namespace aim_diagnostic;
    if(fire!=this)return;
    Arithmetic arithmetic;
    fire=previous_;
    if(serial_)if(FILE* f=Begin("fire-exit")){
        std::fprintf(f," fire=%u owner=%08X ordinary_bullets=%u recorded_bullets=%u",serial_,owner_,ordinary_,accepted_);End(f);
    }
}
TrialAimShotScope::TrialAimShotScope(const X86Context& c,const uint8_t* b):previous_(aim_diagnostic::shot_owner){
    using namespace aim_diagnostic;shot_owner=0;
    if(!Enabled())return;
    if(fire)++fire->ordinary_;
    if(shots>=kShotLimit)return;
    if(!TrialGuestReadable(c.esp,8)){Rejected(c,b,"stack");return;}
    auto* p=Owner(b,c.esi);const uint32_t request=Word(b,c.esp);
    if(!p){Rejected(c,b,"owner",request);return;}
    if(!TrialGuestReadable(request,60)){Rejected(c,b,"packet",request);return;}
    if(Word(b,c.esp+4)!=p->id){Rejected(c,b,"argument-owner",request);return;}
    if(Word(b,request+12)!=p->id){Rejected(c,b,"packet-owner",request);return;}
    Arithmetic arithmetic;shot_owner=p->owner;p->shot=++shots;p->frames=kFrameLimit;p->quads=0;
    if(fire)++fire->accepted_;
    if(FILE* f=Begin("shot")){
        std::fprintf(f," shot=%u owner=%08X id=%u slot=%u item=%u selected=%08X last_render=%llu fire=%u admitted0=%08X admitted1=%08X",
            p->shot,p->owner,p->id,Word(b,p->owner+0x368),Word(b,request),Word(b,p->owner+0x388),
            (unsigned long long)p->render,fire?fire->serial_:0,TrialCoopPlayer(0),TrialCoopPlayer(1));
        Vector(f,"origin",b,request+48,3);Vector(f,"spread_direction",b,request+16,3);
        // CB3C8 has pushed two arguments; +54 at the original aligned frame
        // is the unchanged target-minus-eye before 43590 perturbs each pellet.
        Vector(f,"base_direction",b,c.esp+8+0x54,3);
        Vector(f,"eye",b,p->owner+0x6b0,16);
        Vector(f,"target",b,p->owner+0x454,3);Vector(f,"previous_target",b,p->owner+0x460,3);
        End(f);
    }
    if(p->render)if(FILE* f=Begin("previous-render")){Draw(f,*p);End(f);}
}
TrialAimShotScope::~TrialAimShotScope(){aim_diagnostic::shot_owner=previous_;}

void TrialAimBullet(const X86Context& c,const uint8_t* b){
    using namespace aim_diagnostic;
    if(!Enabled()||!TrialGuestReadable(c.esi,0x228))return;
    for(auto& old:bullets)if(old.object==c.esi)old={};
    if(!shot_owner)return;
    auto* p=Owner(b,shot_owner);
    if(!p||!p->shot||Word(b,c.esi+0xe0)!=p->id)return;
    Arithmetic arithmetic;
    auto& bullet=bullets[(p->shot-1)%bullets.size()];
    bullet={c.esi,Word(b,c.esi+0x54),p->owner,p->id,p->shot};
    if(FILE* f=Begin("bullet")){
        std::fprintf(f," shot=%u owner=%08X id=%u bullet=%08X bullet_id=%u native_frame=%u",
            bullet.shot,bullet.owner,bullet.owner_id,bullet.object,bullet.id,Word(b,c.esi+0x224));
        Vector(f,"origin",b,c.esi+0x218,3);Vector(f,"direction",b,c.esi+0x1d0,3);End(f);
    }
}
void TrialAimImpact(const X86Context& c,const uint8_t* b){
    using namespace aim_diagnostic;
    if(!Enabled()||!TrialGuestReadable(c.esp,8)||!TrialGuestReadable(c.ecx,0x228))return;
    const uint32_t packet=Word(b,c.esp+4);if(!TrialGuestReadable(packet,0x28))return;
    const Bullet* found=nullptr;
    // Some projectiles survive their initial EBA90 update. Bind the original
    // initialized entity, not only the synchronous caller's current owner.
    for(const auto& bullet:bullets)if(bullet.object==c.ecx&&bullet.id==Word(b,c.ecx+0x54)&&
        bullet.owner_id==Word(b,c.ecx+0xe0)){
        if(!found||bullet.shot>found->shot)found=&bullet;
    }
    if(!found)return;
    auto* p=Owner(b,found->owner);if(!p||p->id!=found->owner_id)return;
    Arithmetic arithmetic;
    if(FILE* f=Begin("static-hit")){
        std::fprintf(f," shot=%u owner=%08X id=%u bullet=%08X bullet_id=%u material=%u",
            found->shot,p->owner,p->id,c.ecx,Word(b,c.ecx+0x54),Word(b,packet+4));
        Vector(f,"point",b,packet+0x10,3);Vector(f,"normal",b,packet+0x1c,3);
        Vector(f,"bullet_direction",b,c.ecx+0x1d0,3);Vector(f,"bullet_origin",b,c.ecx+0x218,3);End(f);
    }
}

void TrialAimRender(const X86Context& c,const uint8_t* b){
    using namespace aim_diagnostic;if(!Enabled())return;
    auto* p=Owner(b,c.edi);const uint32_t draw=c.esp+0x80;
    if(!p||!TrialGuestReadable(draw,0x110)||!TrialGuestReadable(c.esi,0x104))return;
    Arithmetic arithmetic;p->camera=c.esi;p->viewport=Word(b,draw+4);p->render=++render_sequence;
    p->camera_id=SafeWord(b,c.esi+0x54);p->camera_owner_id=SafeWord(b,c.esi+0x58);
    const uint32_t world=SafeWord(b,0x51c4dc),state=Field(b,world,4);
    const uint32_t first=Field(b,state,4),end=Field(b,state,8);
    for(unsigned i=0;i<2;++i){
        p->admitted[i]=TrialCoopPlayer(i);p->layers[i]=p->layer_owner[i]=p->layer_tag[i]=p->layer_viewport[i]=0;
        if(first>0&&first<=0xfffffff7u&&end==first+8&&TrialGuestReadable(first,8)){
            const uint32_t layer=Word(b,first+4*i);p->layers[i]=layer;
            if(TrialGuestReadable(layer,0x28)){
                p->layer_owner[i]=Word(b,layer+4);p->layer_tag[i]=Word(b,layer+0x24);p->layer_viewport[i]=Word(b,layer+0x20);
            }
        }
    }
    std::memcpy(p->draw.data(),b+draw+0x10,p->draw.size()*4);p->quads=0;
    // Concrete A5C50 viewport: top,left,bottom,right at +8..+14.
    p->viewport_valid=TrialGuestReadable(p->viewport,0x1c)&&Word(b,p->viewport)==0x456f98u;
    if(p->viewport_valid)std::memcpy(p->bounds.data(),b+p->viewport+8,16);
    if(p->frames){--p->frames;if(FILE* f=Begin("render")){Draw(f,*p);End(f);}}
}

TrialAimReticleScope::TrialAimReticleScope(const X86Context& c,const uint8_t* b):previous_(aim_diagnostic::reticle_owner){
    using namespace aim_diagnostic;reticle_owner=0;
    if(!Enabled()||!TrialGuestReadable(c.ecx,8)||Word(b,c.ecx)!=0x453160u)return;
    if(auto* p=Owner(b,Word(b,c.ecx+4)))reticle_owner=p->owner;
}
TrialAimReticleScope::~TrialAimReticleScope(){aim_diagnostic::reticle_owner=previous_;}
void TrialAimQuad(const X86Context& c,const uint8_t* b){
    using namespace aim_diagnostic;
    if(!Enabled()||!reticle_owner||!TrialGuestReadable(c.esp,12)||Word(b,c.esp)!=0x7b149u)return;
    auto* p=Owner(b,reticle_owner);
    if(!p||!p->shot||!p->frames||p->quads++>=kQuadLimit)return;
    Arithmetic arithmetic;if(FILE* f=Begin("reticle-quad")){
        std::fprintf(f," shot=%u owner=%08X id=%u render=%llu sprite=%08X",
            p->shot,p->owner,p->id,(unsigned long long)p->render,c.esi);
        Vector(f,"xy0",b,Word(b,c.esp+4),2);Vector(f,"xy1",b,Word(b,c.esp+8),2);End(f);
    }
}
#endif

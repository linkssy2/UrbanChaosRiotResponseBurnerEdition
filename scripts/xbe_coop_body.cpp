#include "xbe_coop_body.h"
#include "xbe_coop_guest.h"
#include "xbe_coop.h"
#include "xbe_coop_turrets.h"
#include "cpu/x86_ops.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <cstring>
#include <xmmintrin.h>

namespace {
struct Body {uint32_t owner=0,owner_id=0,world=0,object=0,id=0;bool attempted=false;};
Body bodies[2];
Body retiring[2];
thread_local uint32_t animation_sound_body=0;
uint32_t Word(const uint8_t* b,uint32_t a){uint32_t v;std::memcpy(&v,b+a,4);return v;}
void Put(uint8_t* b,uint32_t a,uint32_t v){std::memcpy(b+a,&v,4);}
bool Owner(const uint8_t* b,const Body& p){
    return b&&p.owner&&TrialGuestReadable(p.owner,0x890)&&Word(b,p.owner)==0x46fbe8u&&
        Word(b,p.owner+0x54)==p.owner_id&&Word(b,0x51c4dc)==p.world;
}
bool Live(const uint8_t* b,const Body& p){
    return Owner(b,p)&&p.object&&TrialGuestReadable(p.object,0x270)&&
        Word(b,p.object)==0x45d040u&&Word(b,p.object+0x54)==p.id&&
        Word(b,p.object+0x58)==p.owner_id&&!(Word(b,p.object+0x5c)&1);
}
Body* Find(const uint8_t* b,uint32_t owner){
    for(auto& p:bodies)if(p.owner==owner&&Live(b,p))return &p;
    return nullptr;
}
bool AuthoredBodyCamera(const X86Context& c,const uint8_t* b,const Body& p){
    if(!trial_coop::Requested()||c.ebx!=p.world||!TrialGuestReadable(c.ebx,8)||
       !TrialGuestReadable(c.esi,0x104)||Word(b,c.esi+0x54)==0xffffffffu||
       (Word(b,c.esi+0x5c)&1)||Word(b,c.esi+0xd0)!=0x478d20u)return false;
    const uint32_t type=Word(b,c.esi);
    // These authored cameras show the cutscene's own actors. EnemyDeathCamera
    // and player-death cameras must continue showing the generated bodies.
    if(type!=0x46fd70u&&type!=0x477da8u)return false;
    const uint32_t slot=Word(b,p.owner+0x368),state=Word(b,c.ebx+4);
    if(slot>1||TrialCoopPlayer(slot)!=p.owner||(Word(b,p.owner+0x5c)&1)||
       !TrialGuestReadable(state,12))return false;
    const uint32_t first=Word(b,state+4),end=Word(b,state+8);
    if(end<first||(end-first!=4&&end-first!=8)||slot>=(end-first)/4||!TrialGuestReadable(first,end-first))return false;
    const uint32_t layer=Word(b,first+slot*4);
    if(!TrialGuestReadable(layer,0x28)||Word(b,layer+4)!=p.owner||Word(b,layer+0x24)!=slot)return false;
    const uint32_t stack=Word(b,layer+8);
    if(!TrialGuestReadable(stack,12)||!Word(b,stack+8))return false;
    const uint32_t head=Word(b,stack+4);
    if(!TrialGuestReadable(head,8))return false;
    const uint32_t node=Word(b,head+4);
    return TrialGuestReadable(node,12)&&Word(b,node+8)==c.esi;
}
uint32_t RenderVisibility(const uint8_t* b,uint32_t object){
    if(!TrialGuestReadable(object,0x78))return 0;
    const uint32_t model=Word(b,object+0x74);
    if(!TrialGuestReadable(model,4))return 0;
    const uint32_t render=Word(b,model);
    if(!TrialGuestReadable(render,4))return 0;
    const uint32_t table=Word(b,render);
    if(!TrialGuestReadable(table,0x18))return 0;
    const uint32_t setter=Word(b,table+0x10),getter=Word(b,table+0x14);
    // Retail static and animated render interfaces keep visibility at different offsets.
    const uint32_t offset=setter==0x8a080&&getter==0x8a090?0x50:
        setter==0xb5e80&&getter==0xb5e90?0x66:0;
    return offset&&TrialGuestReadable(render,offset+1)?render+offset:0;
}
uint32_t Call(X86Context& c,uint32_t sp,uint32_t self,uint32_t fn,std::initializer_list<uint32_t> args){
    c.esp=sp;c.ecx=self;return kernel::CallGuest(c,fn,args);
}
void Create(X86Context& c,uint8_t* b,Body& p,unsigned slot){
    if(c.esp<0x18000)return;
    const uint32_t state=Word(b,p.world+4),services=Word(b,0x51c398);
    const uint32_t scratch=(c.esp-0x500)&~15u;
    if(!TrialGuestReadable(scratch-0x6000,0x6500)||!TrialGuestReadable(state,0x2050)||
       !TrialGuestReadable(services,16)||!TrialGuestReadable(Word(b,services+4),8)||
       !TrialGuestReadable(Word(b,services+8),8))return;
    struct Restore {
        X86Context& c;X86Context saved;uint8_t* b;uint32_t state,source;std::array<uint8_t,12> creation;unsigned csr;
        ~Restore(){std::memcpy(b+state+0x2044,creation.data(),12);Put(b,0x51c3e0,source);c=saved;_mm_setcsr(csr);}
    } restore{c,c,b,state,Word(b,0x51c3e0),{},_mm_getcsr()};
    std::memcpy(restore.creation.data(),b+state+0x2044,12);
    p.attempted=true;_mm_setcsr(0x1f80);
    std::memset(b+scratch,0,0x340);
    Call(c,scratch,scratch,0x159c0,{0,0xffffffffu});
    const uint32_t value=scratch+0x300;
    const auto prop=[&](uint32_t key,uint32_t v){Put(b,value,v);Call(c,scratch,scratch,0x1f8de0,{1,key,value});};
    // D6880's original properties, using the accepted character rather than a network peer hash.
    const unsigned character=unsigned(trial_coop::Selected(slot));
    static constexpr uint32_t skins[]={0x45bcf8,0x45be70,0x45be10,0x45bdb0,0x45bd50};
    prop(0x45bcd8,0);prop(0x45bcc0,0);prop(0x45bca8,character==3?1:0);prop(0x45bc90,character==2?2:0);
    const uint32_t item=Call(c,scratch,p.owner+0x498,0x422c0,{});
    if(TrialGuestReadable(item,8)){
        const uint32_t weapon=Word(b,item+4);
        // Retail ID25 has no third-person metadata. Its real animated gun is
        // shared between views; the body constructor explicitly supports -1.
        prop(0x45bce8,weapon==25?0xffffffffu:weapon);
    }
    // 20A260 delegates to this original child factory with the parent's entity ID.
    Call(c,scratch,scratch,0x1f8e30,{1});
    Put(b,scratch+0x2b0,0x459a38);
    p.id=Call(c,scratch,p.world,0x207d20,{scratch+0x2b0,p.owner_id,scratch});
    Call(c,scratch,scratch,0x1f8ba0,{});
    p.object=Call(c,scratch,0,0x1f8fc0,{p.id});
    if(!Live(b,p)){p.object=0;return;}
    // Retail body creation uses its own animation-derived world matrix.
    Put(b,p.object+0x60,(Word(b,p.object+0x60)&~0x10u)|2u);
    Put(b,value,skins[character]);
    Call(c,scratch,p.object,0xe6110,{value,0});
    if(!Live(b,p))return;
    // The original visibility event also controls the attached weapon children.
    Put(b,value,1);Put(b,value+4,p.owner_id);
    Call(c,scratch,Word(b,services+8),0x1f90a0,{p.id,Word(b,0x4fa004),value,1,value+4});
}
}

void TrialCoopBodiesReset(){for(auto& p:bodies)p={};for(auto& p:retiring)p={};}
void TrialCoopBodiesEnsure(X86Context& c,uint8_t* b){
    if(!b||!trial_coop::Active()||!trial_coop::ResourcesReady())return;
    const uint32_t world=Word(b,0x51c4dc);
    if(!TrialGuestReadable(world,8))return;
    for(unsigned i=0;i<2;++i){
        const uint32_t owner=TrialCoopPlayer(i);
        if(!TrialGuestReadable(owner,0x890)||Word(b,owner)!=0x46fbe8u||Word(b,owner+0x368)!=i)return;
    }
    for(unsigned i=0;i<2;++i){
        const uint32_t owner=TrialCoopPlayer(i);auto& p=bodies[i];
        if(p.owner!=owner||!Owner(b,p))p={owner,Word(b,owner+0x54),world};
        if(!p.attempted)Create(c,b,p,i);
    }
}
void TrialCoopBodyResolve(X86Context& c,uint8_t* b){
    // Only the three CA620 body lookups are hooked. Other MainPlayer virtual
    // queries and its null network interface retain their original semantics.
    if(trial_coop::Active())if(auto* p=Find(b,c.esi))c.eax=p->object;
}
uint32_t TrialCoopBodyForPlayer(const uint8_t* b,uint32_t owner){
    if(trial_coop::Active())if(auto* p=Find(b,owner))return p->object;
    return 0;
}
bool TrialCoopBodyRecreate(X86Context& c,uint8_t* b,uint32_t owner){
    if(!trial_coop::Active()||!trial_coop::ResourcesReady()||c.esp<0x18000||
       !TrialGuestReadable(c.esp-0x7000,0x7000))return false;
    auto* old=Find(b,owner);
    if(!old||!b[old->object+0x198]||Word(b,owner+0x170)!=12)return false;
    const unsigned slot=Word(b,owner+0x368);
    if(slot>1||TrialCoopPlayer(slot)!=owner)return false;
    if(retiring[slot].object&&retiring[slot].world==old->world)return false;
    Body fresh{old->owner,old->owner_id,old->world};
    Create(c,b,fresh,slot);
    if(!Live(b,fresh)||fresh.object==old->object)return false;
    // Keep the parent registered until the world is between update passes.
    // Immediate retirement or flag-1 deletion can orphan a gun whose first
    // update already ran, or invalidate the world's cached next list node.
    retiring[slot]=*old;
    Put(b,old->object+0x60,Word(b,old->object+0x60)|0x40000u);
    *old=fresh;
    return true;
}
void TrialCoopBodiesRetire(X86Context& c,uint8_t* b){
    if(!b||c.ebp!=Word(b,0x51c4dc)||c.esp<0x18000||
       !TrialGuestReadable(c.esp-0x6000,0x6000))return;
    struct Restore {unsigned csr;~Restore(){_mm_setcsr(csr);}} restore{_mm_getcsr()};
    X86Context temp=c;
    const uint32_t sp=c.esp-0x100;
    for(auto& queued:retiring){
        const Body old=queued;queued={};
        // A native delete request may have marked the queued body meanwhile.
        // Its skip-update bit deliberately keeps it alive until this boundary.
        if(!old.object||old.world!=c.ebp||!TrialGuestReadable(old.object,0x270)||
           Word(b,old.object)!=0x45d040u||Word(b,old.object+0x54)!=old.id||
           Word(b,old.object+0x58)!=old.owner_id||
           Call(temp,sp,0,0x1f8fc0,{old.id})!=old.object)continue;
        const auto child=[&](uint32_t id,uint32_t parent){
            if(id==0xffffffffu)return 0u;
            const uint32_t object=Call(temp,sp,0,0x1f8fc0,{id});
            return object&&TrialGuestReadable(object,0x78)&&Word(b,object+0x54)==id&&
                Word(b,object+0x58)==parent?object:0u;
        };
        const auto retire=[&](uint32_t object){
            if(!object)return;
            const uint32_t table=Word(b,object);
            if(TrialGuestReadable(table,0x28))Call(temp,sp,object,Word(b,table+0x24),{});
        };
        for(uint32_t offset:{0x1acu,0x1b0u,0x22cu,0x230u}){
            const uint32_t object=child(Word(b,old.object+offset),old.id);
            // ThirdPersonGun owns a GunFlare as well as the body's direct
            // LightningBeam children. Retire descendants before their parent.
            if(object&&Word(b,object)==0x462658u&&TrialGuestReadable(object,0xb0))
                retire(child(Word(b,object+0xac),Word(b,object+0x54)));
            retire(object);
        }
        Call(temp,sp,old.object,0x218b30,{});
    }
}
void TrialCoopBodyWeapon(X86Context& c,uint8_t* b){
    if(!trial_coop::Active()||c.esp<0x18000)return;
    auto* p=Find(b,c.esi);if(!p||!Word(b,p->object+0x158)||!TrialGuestReadable(c.esp-0x6000,0x6000))return;
    X86Context temp=c;
    struct Restore {unsigned csr;~Restore(){_mm_setcsr(csr);}} restore{_mm_getcsr()};
    const uint32_t sp=c.esp-0x100;
    const uint32_t item=Call(temp,sp,p->owner+0x498,0x422c0,{});
    if(!TrialGuestReadable(item,8))return;
    uint32_t id=Word(b,item+4);
    // ID25 is a non-null definition with no +C0 animation metadata. Retain the
    // last valid body weapon while mounted; E55F0 supports a null definition,
    // but any non-null old/new definition must have its original metadata.
    if(id==25)return;
    // D6070's multiplayer presentation selects Shield (ID0), not the stored
    // firearm, while the native inventory shield gate and position allow it.
    // E55F0 supplies the original shield idle pose and weapon-child model swap;
    // releasing/lowering restores the firearm through that same transition.
    if(Call(temp,sp,p->owner+0x498,0x41f50,{})){
        float position;std::memcpy(&position,b+p->owner+0x24c,4);
        if(position<100.f)id=0;
    }
    const uint32_t weapon=b[p->owner+0x2ac]?0:Call(temp,sp,0,0x442d0,{id});
    if(weapon!=Word(b,p->object+0x154))Call(temp,sp,p->object,0xe55f0,{weapon});
}
void TrialCoopBodyAction(X86Context& c,uint8_t* b){
    // C1E74 has the accepted native action in EDI and its owner in ESI. The
    // campaign actor intentionally has no network body/C8 interface, so keep
    // its first-person path and add only the original body's animation path.
    if(!b||!trial_coop::Active()||c.edi>17||c.esp<0x18000||
       !TrialGuestReadable(c.esp-0x6000,0x6000))return;
    auto* p=Find(b,c.esi);
    if(!p||!Word(b,p->object+0x158)||!Word(b,p->object+0x1bc))return;
    struct Restore {
        unsigned csr;uint32_t previous;
        ~Restore(){animation_sound_body=previous;_mm_setcsr(csr);}
    } restore{_mm_getcsr(),animation_sound_body};
    animation_sound_body=p->object;
    X86Context temp=c;
    Call(temp,c.esp-0x100,p->object,0xe57b0,{c.edi});
}
bool TrialCoopBodyAnimationSoundSuppressed(const X86Context& c,const uint8_t* b){
    // The owner already plays this action's sound. Suppress only the redundant
    // body sound while its additive animation call is on this thread's stack.
    if(!b||!trial_coop::Active()||!animation_sound_body||c.ecx!=animation_sound_body)return false;
    for(const auto& p:bodies)if(p.object==c.ecx&&Live(b,p))return true;
    return false;
}
void TrialCoopBodyDestroyed(const X86Context& c,uint8_t*){
    for(auto& p:bodies)if(p.owner==c.ecx||p.object==c.ecx)p={};
}
bool TrialCoopSkipUnreadyView(const X86Context& c,const uint8_t* b){
    // A newly constructed Mainplayer has a layer before its startup coroutine
    // activates a camera. Retail renders a null camera as a fullscreen black
    // clear, which would erase the already rendered first player's view.
    if(!trial_coop::Requested()||!b||c.esi||c.edi>3||
       c.ebx!=Word(b,0x51c4dc)||!TrialGuestReadable(c.ebx,8))return false;
    const uint32_t owner=TrialCoopPlayer(1);
    if(!TrialGuestReadable(owner,0x36c)||Word(b,owner)!=0x46fbe8||
       Word(b,owner+0x368)!=1||Word(b,owner+0x5c)&1)return false;
    const uint32_t state=Word(b,c.ebx+4);
    if(!TrialGuestReadable(state,12))return false;
    const uint32_t first=Word(b,state+4),end=Word(b,state+8);
    if(end<first||end-first>16||c.edi>=(end-first)/4||
       !TrialGuestReadable(first,end-first))return false;
    const uint32_t layer=Word(b,first+c.edi*4);
    const bool unready=TrialGuestReadable(layer,0x28)&&Word(b,layer+4)==owner&&Word(b,layer+0x24)==1;
    if(unready)TrialCoopTraceStartup(c,b,owner,TrialCoopTraceStage::UnreadyView);
    return unready;
}
TrialCoopBodyView::~TrialCoopBodyView(){Clear();}
void TrialCoopBodyView::Clear(){
    if(base_)for(auto& s:saved_)if(s.object&&TrialGuestReadable(s.object,0x60)&&Word(base_,s.object+0x54)==s.id){
        Put(base_,s.object+0x5c,(Word(base_,s.object+0x5c)&~0x100u)|(s.flags&0x100u));
        if(s.visibility&&RenderVisibility(base_,s.object)==s.visibility)base_[s.visibility]=s.visible;
    }
    saved_={};base_=nullptr;
}
void TrialCoopBodyView::Select(const X86Context& c,uint8_t* b){
    Clear();if((!trial_coop::Active()&&!trial_coop::Requested())||c.esp<0x1000||!TrialGuestReadable(c.esp-0x1000,0x1000))return;
    auto* p=Find(b,c.edi);if(!p)return;
    const bool authored=AuthoredBodyCamera(c,b,*p);
    // 21525D still has the active original camera in ESI. Ordinary gameplay
    // hides only this view's own body; authored scenes hide both standing bodies.
    if(!authored&&(!trial_coop::Active()||!TrialGuestReadable(c.esi,0x230)||
       Word(b,c.esi+0x54)!=Word(b,p->owner+0x308)||Word(b,c.esi+0x58)!=p->owner_id||
       Word(b,c.esi+0x22c)!=p->owner_id||Word(b,c.esi+0x210)!=2))return;
    base_=b;
    // Original body weapon and spark children are independently visible entities.
    // Resolve them through the retail registry, retaining their previous hide state.
    X86Context temp=c;
    struct Restore {unsigned csr=_mm_getcsr();~Restore(){_mm_setcsr(csr);}} restore;
    const auto save=[&](const Body& body,unsigned first){
        saved_[first]={body.object,body.id,Word(b,body.object+0x5c)};
        const uint32_t offsets[]={0x1ac,0x1b0,0x22c,0x230};
        for(unsigned i=0;i<4;++i){
            const uint32_t id=Word(b,body.object+offsets[i]);if(id==0xffffffffu)continue;
            const uint32_t object=Call(temp,c.esp-0x100,0,0x1f8fc0,{id});
            if(object&&TrialGuestReadable(object,0x60)&&Word(b,object+0x54)==id&&Word(b,object+0x58)==body.id)
                saved_[first+i+1]={object,id,Word(b,object+0x5c)};
        }
    };
    if(authored){
        for(unsigned slot=0;slot<2;++slot){
            const auto& body=bodies[slot];
            if(Live(b,body)&&TrialCoopPlayer(slot)==body.owner&&Word(b,body.owner+0x368)==slot&&
               !(Word(b,body.owner+0x5c)&1)&&Word(b,body.owner+0x170)!=12&&!b[body.object+0x198])save(body,slot*5);
        }
    }else{
        save(*p,0);
        // A mounted partner's gun is their actual MainPlayer model. Hide only
        // stale handheld children here, leaving the partner's body visible.
        for(const auto& body:bodies)if(body.owner!=p->owner&&Live(b,body)&&TrialCoopTurretMount(b,body.owner)){
            for(unsigned i=0;i<2;++i){
                const uint32_t id=Word(b,body.object+0x1ac+i*4);if(id==0xffffffffu)continue;
                const uint32_t object=Call(temp,c.esp-0x100,0,0x1f8fc0,{id});
                if(object&&TrialGuestReadable(object,0x60)&&Word(b,object+0x54)==id&&Word(b,object+0x58)==body.id)
                    saved_[5+i]={object,id,Word(b,object+0x5c)};
            }
        }
    }
    for(auto& s:saved_)if(s.object){
        Put(b,s.object+0x5c,s.flags|0x100u);
        // 215106 updates render visibility only once, before the camera loop.
        // 968A0 admits draws using the render object's byte, not entity flags.
        // Scope that byte too, restoring its actual value for the next camera.
        s.visibility=RenderVisibility(b,s.object);
        if(s.visibility){s.visible=b[s.visibility];b[s.visibility]=0;}
    }
}

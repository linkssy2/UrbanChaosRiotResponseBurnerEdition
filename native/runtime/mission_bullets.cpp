#include "mission_bullets.h"
#include <vector>

struct NativeMissionBullets::Impl {
    struct Bullet {NativeMissionEntity entity;UCBulletState state{};Impl*owner;bool constructed=false,removed=false;};
    NativeMissionWorld&world;const uint8_t&progression;
    UCBulletScenery scenery;UCBulletTracerServices tracer;
    std::vector<std::unique_ptr<Bullet>> bullets;
    int32_t cached=-1;uint32_t counter=0;int failure=0;
    Impl(NativeMissionWorld&w,const uint8_t&p,const UCBulletScenery&s,const UCBulletTracerServices&t):world(w),progression(p),scenery(s),tracer(t){}
    static int empty_tick(void*p){auto&b=*(Bullet*)p;return b.constructed&&!b.removed?0:1;} //source11030
    static int close(void*p){auto&b=*(Bullet*)p;auto&e=b.entity;
        if(uc_entity_event_lookup(&b.owner->world.registry(),e.id)||e.model||e.helper||e.actor.handleBC||e.actor.audioC0||e.actor.scriptC8)return 1;
        for(auto*l:e.identity.listeners)if(l)return 1;
        if(e.aux&&uc_entity_fresh_aux_destroy(e.aux))return 1;e.aux=nullptr;b.constructed=false;return 0;
    }
    static int remove(void*p){auto&b=*(Bullet*)p;auto&s=*b.owner;auto&e=b.entity;
        if(!b.constructed||b.removed||e.model||e.helper||e.actor.handleBC||e.actor.audioC0||e.actor.scriptC8||!e.aux||e.source!=-1)return 1;
        for(auto*l:e.identity.listeners)if(l)return 1;
        --s.counter;s.cached=-1; //EAE10 before218B30; unsigned source wrap retained.
        if(uc_entity_fresh_aux_clear(e.aux))return 2;e.flags|=0x8100;uint8_t queued=0;
        if(uc_entity_retire(&s.world.retirement(),e.id,&queued)||!queued)return 2;e.flags|=0x80000;b.removed=true;return 0;
    }
    static int destroy(void*p){auto&b=*(Bullet*)p;auto&s=*b.owner;if(close(p))return 1;
        for(auto&entry:s.bullets)if(entry.get()==&b){entry.reset();return 0;}return 1;
    }
    static int step(Bullet&b){auto&s=*b.owner;UCBulletStep h{};h.context=&b;h.delta=s.world.script_services().delta;
        h.preflight=[](void*p,const UCEntityFreshView*v,const UCBulletState*state){auto&b=*(Bullet*)p;
            return int(b.constructed&&!b.removed&&v==&b.entity.view&&state==&b.state);};
        h.displacement=[](void*p,const UCEntityFreshView*v,UCBulletState*state,float*out){return uc_bullet_scenery(&((Bullet*)p)->owner->scenery,v,state,out);};
        h.tracer=[](void*p,const UCEntityFreshView*v,UCBulletState*state){return uc_bullet_tracer(&((Bullet*)p)->owner->tracer,v,state);};
        return uc_bullet_step(&h,&b.entity.view,&b.state);
    }
    int fire(const UCWeaponBulletRequest&r,uint32_t owner){
        if(failure)return failure;
        auto*t=uc_entity_event_lookup(&world.registry(),cached);Bullet*b=nullptr;
        if(t){for(auto&entry:bullets)if(entry && &entry->entity.identity==t){b=entry.get();break;}if(!b)return failure=2;}
        else{
            auto fresh=std::make_unique<Bullet>();fresh->owner=this;b=fresh.get();bullets.push_back(std::move(fresh));
            if(world.bind_dynamic({&b->entity.view,b,empty_tick,empty_tick,close,nullptr,remove,destroy}))return failure=2;
            auto c=world.context();const void*definition="Bullet";c.definition=&definition;
            *c.registration->creation={0,1,-1};
            if(uc_bullet_construct(&c,&b->entity.view,&b->state))return failure=2;
            cached=b->entity.id;b->constructed=true;
        }
        //The synchronous request cannot advance world+0C; this is its value
        //at EB4F0, not a second authoritative frame counter.
        uint32_t frame=world.frame();UCBulletInitialize h{};h.registry=&world.registry();h.group_counter=world.context().group_counter;
        h.frame=&frame;h.bullet_counter=&counter;h.progression181=&progression;h.context=b;
        h.resolve=[](void*p,UCEntityEventTarget*t){auto&w=((Bullet*)p)->owner->world;return w.context().resolve(w.context().owner,t);};
        h.preflight=[](void*p,const UCEntityFreshView*v,const UCBulletState*state,const UCWeaponBulletRequest*){auto&b=*(Bullet*)p;
            return int(b.constructed&&!b.removed&&v==&b.entity.view&&state==&b.state);};
        h.initial_update=[](void*p,const UCEntityFreshView*,UCBulletState*){return step(*(Bullet*)p);};
        const int result=uc_bullet_initialize(&h,&b->entity.view,&b->state,&r,owner);if(result)failure=result;return result;
    }
};
NativeMissionBullets::NativeMissionBullets(NativeMissionWorld&w,const uint8_t&p,const UCBulletScenery&s,const UCBulletTracerServices&t):p_(std::make_unique<Impl>(w,p,s,t)){}
NativeMissionBullets::~NativeMissionBullets()=default;
int NativeMissionBullets::fire(const UCWeaponBulletRequest&r,uint32_t owner){return p_->fire(r,owner);}
uint32_t NativeMissionBullets::shots()const{return p_->counter;}

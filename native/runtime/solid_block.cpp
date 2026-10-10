#include "solid_block.h"
#include "mission_world.h"
#include <cmath>
#include <cstring>
#include <new>
int NativeSolidBlock::base(void*p){auto&b=*(NativeSolidBlock*)p;return uc_entity_fresh_construct_authored(b.context_,&b.entity.view,b.active_);}
int NativeSolidBlock::prepare(void*p,UCSolidBlock*s){auto&b=*(NativeSolidBlock*)p;
    if(s!=&b.state_||b.body_)return 1;
    b.entity.flags|=0x10000000;
    auto body=std::unique_ptr<Body>(new(std::nothrow)Body);
    if(!body||uc_solid_block_geometry(b.entity.transform,*b.radius_,&body->geometry))return 1;
    b.body_=std::move(body);s->body=b.body_.get();return add(p,s->body);
}
int NativeSolidBlock::add(void*p,void*body){auto&b=*(NativeSolidBlock*)p;if(body!=b.body_.get()||!body||b.body_->active)return 1;b.body_->active=true;return 0;}
int NativeSolidBlock::remove(void*p,void*body){auto&b=*(NativeSolidBlock*)p;if(body!=b.body_.get()||!body||!b.body_->active)return 1;b.body_->active=false;return 0;}
int NativeSolidBlock::suffix(void*p){return ((NativeSolidBlock*)p)->entity.script.update();}
int NativeSolidBlock::construct(const UCEntityFreshContext&c,const UCEntityAuthored *const *active,
    const UCEntityAuthored&m,const NativeMissionServices&s,const float*radius,NativeMissionWorld*world){
    UCSolidBlockGeometry checked;
    if(context_||!active||*active!=&m||!m.definition||!m.definition->class_name||
       std::strcmp(m.definition->class_name,"SolidCollisionBlock")||m.asset_present||
       !radius||uc_solid_block_geometry(m.definition->transform,*radius,&checked)||
       !s.crt_seed||!s.delta)return 1;
    context_=&c;active_=active;radius_=radius;world_=world;
    if(uc_solid_block_construct(&state_,this,base)||entity.script.attach_actor(entity.view,m,s))return failure_=2;
    return 0;
}
int NativeSolidBlock::update(){
    if(!context_||failure_)return failure_?failure_:1;
    UCSolidBlockServices h{this,prepare,add,remove,suffix};int r=uc_solid_block_update(&state_,&entity.flags,&h);
    if(r)failure_=2;return r;
}
int NativeSolidBlock::late(){
    if(!context_||failure_)return failure_?failure_:1;
    //Actual inherited218C30/218400 and464DC8+7C=46980, zero-velocity getter.
    if(!(entity.actor.word90&1)){for(unsigned i=12;i<15;i++){if(!std::isfinite(entity.transform[i]))return failure_=2;entity.transform[i]+=0.0f;}entity.transform[15]=1.f;}return 0;
}
int NativeSolidBlock::retire(){
    if(!world_||failure_||!context_||entity.model||entity.helper||entity.actor.handleBC||!entity.aux)return 1;
    for(auto*l:entity.identity.listeners)if(l)return 1;
    if((entity.flags&0x1000)&&(entity.source<0||uint32_t(entity.source)>=world_->source_count()))return 1;
    //111120 releases the active collision body before218B30 retires the actor.
    if(body_&&state_.state==1&&remove(this,body_.get()))return failure_=2;
    body_.reset();state_.body=nullptr;
    uc_mission_command_destroy(nullptr,entity.actor.audioC0);entity.actor.audioC0=nullptr;
    if(uc_entity_fresh_aux_clear(entity.aux))return failure_=2;
    entity.flags|=0x8100;uint8_t queued=0;
    if(uc_entity_retire(&world_->retirement(),entity.id,&queued)||!queued)return failure_=2;
    if(entity.flags&0x1000){auto*f=world_->slot(entity.source).flags;*f=(*f&~15u)|16u;}
    entity.flags|=0x80000;return 0;
}
int NativeSolidBlock::close_detached(){
    if(!context_)return 0;
    if(entity.script.close_detached())return 1;
    body_.reset();state_.body=nullptr;
    if(entity.aux&&uc_entity_fresh_aux_destroy(entity.aux))return 1;
    entity.aux=nullptr;context_=nullptr;active_=nullptr;radius_=nullptr;return 0;
}

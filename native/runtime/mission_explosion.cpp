#include "mission_explosion.h"
#include <cmath>
#include <cstring>
#include <stdexcept>
extern "C" {
#include "../collision_asset_query.h"
}
int NativeMissionExplosion::construct(NativeMissionWorld&w,const UCRigidPropExplosion&r,NativePlayerAudio&a,
 const UCArchive*archive,UCArchiveLookup&lookup,const std::filesystem::path&stream,const NativeMissionExplosionServices&h){
 return initialize(w,&r,UINT32_MAX,a,archive,lookup,stream,h);
}
int NativeMissionExplosion::construct(NativeMissionWorld&w,uint32_t source,NativePlayerAudio&a,
 const UCArchive*archive,UCArchiveLookup&lookup,const std::filesystem::path&stream,const NativeMissionExplosionServices&h){
 return initialize(w,nullptr,source,a,archive,lookup,stream,h);
}
int NativeMissionExplosion::initialize(NativeMissionWorld&w,const UCRigidPropExplosion*r,uint32_t source,
 NativePlayerAudio&audio,const UCArchive*archive,UCArchiveLookup&lookup,const std::filesystem::path&stream,const NativeMissionExplosionServices&h){
 if(world_||!archive||lookup.archive!=archive||!h.field_step||!h.shake||!h.online||!h.broadcast||!h.area_effect||!h.effect||!h.debris)return 1;
 if(r){if(r->rotation[0]!=0||r->rotation[1]!=0||r->rotation[2]!=0||!r->sound||!std::isfinite(r->power)||r->power<=0||
   !uc_entity_event_lookup(&w.registry(),r->parent))return 1;
  for(unsigned k=0;k<3;k++)if(!std::isfinite(r->position[k])||std::fabs(r->position[k])>1e6f)return 1;
 }else{
  if(source>=w.source_count())return 1;const auto&m=w.metadata(source);UCExplosionParameters p{};
  if(*w.active_descriptor()!=&m||m.asset_present||m.condition_count||m.definition->script_count||
   uc_explosion_parameters(m.definition,&p)||p.power<=0)return 1;
 }
 world_=&w;services_=h;entity.view.actor=nullptr;entity.view.aux84=nullptr;
 if(r&&w.bind_dynamic({&entity.view,this,[](void*p){return ((NativeMissionExplosion*)p)->update();},[](void*){return 0;},
  [](void*p){return ((NativeMissionExplosion*)p)->close_detached();},nullptr,[](void*p){return ((NativeMissionExplosion*)p)->remove();},
  [](void*p){return ((NativeMissionExplosion*)p)->close_detached();}}))return failure_=2;
 struct Context{NativeMissionExplosion*self;const UCRigidPropExplosion*request;uint32_t source;NativePlayerAudio*audio;
  const UCArchive*archive;UCArchiveLookup*lookup;const std::filesystem::path*stream;};
 Context context{this,r,source,&audio,archive,&lookup,&stream};UCExplosionConstructorServices c{};c.owner=&context;
 c.base=[](void*p){auto&c=*(Context*)p;auto&s=*c.self;auto&w=*s.world_;
  if(!c.request)return uc_entity_base_construct_authored(&w.context(),&s.entity.view,w.active_descriptor());
  auto f=w.context();const void*definition="Explosion";f.definition=&definition;
  *f.registration->creation={0,1,c.request->parent};return uc_entity_base_construct_position(&f,&s.entity.view,c.request->position);};
 c.coroutine_initialize=[](void*p){auto&s=*((Context*)p)->self;if(s.initialized_||s.sequence_.live())return 1;s.initialized_=true;return 0;};
 c.property=[](void*p,unsigned type,const char*name,UCExplosionProperty*out,unsigned*found){auto&c=*(Context*)p;
  if(!c.request){int present=0;int result=uc_entity_stream_get(c.self->world_->metadata(c.source).definition,c.self->world_->property_cursor(c.source),type,out,&present);*found=unsigned(present);return result;}
  const auto&r=*c.request;*found=1;
  if(type==6&&!strcmp(name,"Sound"))out->string=r.sound;
  else if(type==2&&!strcmp(name,"Scale"))out->number=r.scale;
  else if(type==1&&!strcmp(name,"DamageType"))out->integer=r.damage_type;
  else if(type==2&&!strcmp(name,"Radius"))out->number=r.radius;
  else if(type==2&&!strcmp(name,"Power"))out->number=r.power;
  else if(type==1&&!strcmp(name,"OwnerID"))out->integer=r.owner;
  else if(type==3&&!strcmp(name,"AffectsDoors"))out->integer=r.affects_doors;
  else if(type==3&&!strcmp(name,"IgnoreCollision"))out->integer=r.ignore_collision;
  else if(type==1&&!strcmp(name,"ParticleType"))out->integer=r.particle_type;
  else if(type==3&&!strcmp(name,"Force kill"))out->integer=r.force_kill;
  else *found=0;return 0;};
 c.sound=[](void*p,const char*name,void**out){auto&c=*(Context*)p;auto&s=*c.self;*out=nullptr;
  if(!name||!*name)return 0;char bank[256];size_t index=SIZE_MAX;
  if(uc_resource_name(name,bank,sizeof bank)||uc_archive_lookup(c.lookup,bank,&index)||index==SIZE_MAX)return 1;
  s.bank_=bank;s.audio_=std::make_unique<NativePlayerAudio::MissionSource>(*c.audio,c.archive,index,*c.stream);*out=s.audio_.get();return 0;};
 c.coroutine_bind=[](void*p){auto&s=*((Context*)p)->self;return s.sequence_.create([](void*q,NativeCoroutine&){auto&s=*(NativeMissionExplosion*)q;
   s.fields_={s.entity.transform+12,&s.state_.parameters,s.entity.id,s.entity.parent,0};auto h=s.hooks();
   if(uc_explosion_sequence(&s.fields_,&h))s.failure_=2;},&s);};
 return uc_explosion_construct(&state_,&c)?failure_=2:0;
}
UCExplosionSequenceServices NativeMissionExplosion::hooks(){UCExplosionSequenceServices h{};h.owner=this;
 h.field_create=[](void*p,const float*point,float power){auto&s=*(NativeMissionExplosion*)p;
  s.field_=std::make_unique<UCExplosionField>();uc_collision_to_physics(point,s.field_->center);s.field_->power=power;s.field_->remaining=2;s.state_.field=s.field_.get();return 0;};
 h.shake=[](void*p,const float*v,float range,float strength,float seconds,uint32_t kind){auto&s=*(NativeMissionExplosion*)p;return s.services_.shake(s.services_.owner,v,range,strength,seconds,kind);};
 h.online=[](void*p){auto&s=*(NativeMissionExplosion*)p;return s.services_.online(s.services_.owner);};
 h.broadcast=[](void*p,const UCExplosionBlast*b,int32_t id){auto&s=*(NativeMissionExplosion*)p;return s.services_.broadcast(s.services_.owner,*b,id);};
 h.area_effect=[](void*p,const float*b,uint32_t type){auto&s=*(NativeMissionExplosion*)p;return s.services_.area_effect(s.services_.owner,b,type);};
 h.sound=[](void*p,const char*name){auto&s=*(NativeMissionExplosion*)p;if(s.audio_)s.audio_->play(name);return 0;};
 h.effect=[](void*p,uint32_t address){auto&s=*(NativeMissionExplosion*)p;return s.services_.effect(s.services_.owner,s,address);};
 h.debris=[](void*p,uint32_t n,float scale,float power,const char*asset){auto&s=*(NativeMissionExplosion*)p;return s.services_.debris(s.services_.owner,s,n,scale,power,asset);};
 h.field_done=[](void*p){auto&s=*(NativeMissionExplosion*)p;return s.field_?int(s.field_->done):-1;};
 h.field_step=[](void*p,float dt){auto&s=*(NativeMissionExplosion*)p;return s.field_?s.services_.field_step(s.services_.owner,*s.field_,dt):1;};
 h.delta=[](void*p){return double(*((NativeMissionExplosion*)p)->world_->script_services().delta);};
 h.sound_active=[](void*p){auto&s=*(NativeMissionExplosion*)p;return s.audio_?int(s.audio_->busy()):0;};
 h.yield=[](void*p){return ((NativeMissionExplosion*)p)->sequence_.yield();};
 h.remove=[](void*p){return ((NativeMissionExplosion*)p)->remove();};return h;
}
int NativeMissionExplosion::update(){
 if(!world_||!initialized_||failure_||removed_)return failure_?failure_:1;
 if(audio_)audio_->step(*world_->script_services().delta);
 if(sequence_.resume()<0)return failure_=2;return failure_;
}
int NativeMissionExplosion::remove(){
 if(!world_||removed_||entity.model||entity.helper||entity.aux)return 1;
 field_.reset();state_.field=nullptr; //E93F0 before20A620 retirement.
 entity.flags|=0x8100;uint8_t queued=0;if(uc_entity_retire(&world_->retirement(),entity.id,&queued)||!queued)return failure_=2;
 if(entity.identity.listeners[0])return failure_=2;removed_=true;return 0;
}
int NativeMissionExplosion::close_detached(){
 if(!world_)return 0;if(uc_entity_event_lookup(&world_->registry(),entity.id)||sequence_.close())return 1;
 audio_.reset();field_.reset();state_.audio=state_.field=nullptr;world_=nullptr;return 0;
}

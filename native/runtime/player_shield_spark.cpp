#include "player_shield_spark.h"
#include "npc_thrown_particles.h"
#include <cstring>
int NativePlayerShieldSpark::construct(NativeMissionWorld&w,int32_t parent,const float*position,const float*rotation,NativeNpcThrownParticles&p,const NativePlayerShieldSparkServices&h){
 if(world_||!position||!rotation||!h.feedback||!uc_entity_event_lookup(&w.registry(),parent))return 1;
 world_=&w;particles_=&p;services_=h;
 if(w.bind_dynamic({&entity.view,this,[](void*p){return ((NativePlayerShieldSpark*)p)->step();},[](void*p){return ((NativePlayerShieldSpark*)p)->late();},[](void*p){return ((NativePlayerShieldSpark*)p)->close_detached();},nullptr,[](void*p){return ((NativePlayerShieldSpark*)p)->remove();},[](void*p){return ((NativePlayerShieldSpark*)p)->close_detached();}}))return failure_=2;
 float pose[6];memcpy(pose,position,12);memcpy(pose+3,rotation,12);auto c=w.context();const void*definition="FXShieldBulletSpark";c.definition=&definition;*c.registration->creation={0,1,parent};
 if(uc_entity_fresh_construct_pose(&c,&entity.view,pose)||w.set_priority(entity.id,3))return failure_=2;
 //Source descriptor's Scale=1 and texture=0 are retained constructor fields;
 //18EB40 uses the authored assets directly and does not consume either value.
 return coroutine_.create([](void*q,NativeCoroutine&){auto&s=*(NativePlayerShieldSpark*)q;UCShieldSparkServices h{};h.owner=q;h.world=s.entity.transform;
  h.feedback=[](void*p,const float*v,float range,float power,float duration,unsigned kind){auto&s=*(NativePlayerShieldSpark*)p;return s.services_.feedback(s.services_.owner,v,range,power,duration,kind);};
  h.particles=[](void*p,const char*n,const float*m){return ((NativePlayerShieldSpark*)p)->particles_->append_shield(n,m);};
  h.yield=[](void*p){return ((NativePlayerShieldSpark*)p)->coroutine_.yield();};h.delta=[](void*p){return double(*((NativePlayerShieldSpark*)p)->world_->script_services().delta);};
  h.remove=[](void*p){return ((NativePlayerShieldSpark*)p)->remove();};if(uc_player_shield_spark_sequence(&h))s.failure_=2;
 },this)?failure_=2:0;
}
int NativePlayerShieldSpark::step(){if(!world_||removed_||failure_)return failure_?failure_:1;if(coroutine_.resume()<0)return failure_=2;
 if(failure_)return failure_;return uc_entity_fresh_aux_clear(entity.aux)?failure_=2:0;}
int NativePlayerShieldSpark::late(){if(!world_||failure_)return 1;if(!(entity.actor.word90&1)){for(unsigned i=12;i<15;i++)entity.transform[i]+=0.f;entity.transform[15]=1;}return 0;}
int NativePlayerShieldSpark::remove(){if(!world_||removed_||failure_||entity.helper||entity.actor.handleBC||entity.actor.audioC0||entity.identity.listeners[0])return 1;
 if(uc_entity_fresh_aux_clear(entity.aux))return failure_=2;entity.flags|=0x8100;uint8_t queued=0;if(uc_entity_retire(&world_->retirement(),entity.id,&queued)||!queued)return failure_=2;entity.flags|=0x80000;removed_=true;return 0;}
int NativePlayerShieldSpark::close_detached(){if(!world_)return 0;if(uc_entity_event_lookup(&world_->registry(),entity.id)||coroutine_.close())return 1;if(uc_entity_fresh_aux_destroy(entity.aux))return 1;entity.aux=nullptr;world_=nullptr;return 0;}

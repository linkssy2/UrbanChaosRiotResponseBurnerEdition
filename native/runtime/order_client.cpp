#include "order_client.h"
#include <cstring>
NativeOrderClient::~NativeOrderClient(){uc_entity_subscriptions_close(subscriptions_);}
UCOrderClientServices NativeOrderClient::services(){UCOrderClientServices h{};h.owner=this;
 h.alive=[](void*p,int32_t id,uint8_t*out){auto&s=*(NativeOrderClient*)p;*out=uc_entity_event_lookup(&s.world_->registry(),id)!=nullptr;return 0;};
 h.position=[](void*p,int32_t id,float*out,uint8_t*found){auto&s=*(NativeOrderClient*)p;return s.callbacks_.manager_position?s.callbacks_.manager_position(s.callbacks_.owner,id,out,found):1;};
 h.accept=[](void*p,int32_t id,const float*position,uint8_t*out){auto&s=*(NativeOrderClient*)p;return s.callbacks_.accept?s.callbacks_.accept(s.callbacks_.owner,id,position,out):1;};
 h.attached=[](void*p){auto&s=*(NativeOrderClient*)p;return s.callbacks_.attached?s.callbacks_.attached(s.callbacks_.owner):1;};
 h.choose=[](void*p,int32_t v){auto&s=*(NativeOrderClient*)p;return s.callbacks_.choose?s.callbacks_.choose(s.callbacks_.owner,v):1;};
 h.completed=[](void*p){auto&s=*(NativeOrderClient*)p;return s.callbacks_.completed?s.callbacks_.completed(s.callbacks_.owner):1;};
 h.send=[](void*p,int32_t target,uint32_t event,const void*payload,uint32_t size,int32_t sender){auto&s=*(NativeOrderClient*)p;uint8_t handled;uint32_t from=(uint32_t)sender;return uc_entity_event_send(&s.world_->registry(),target,event,payload,size,&from,&handled);};
 h.broadcast=[](void*p,uint32_t event,const void*payload,uint32_t size,int32_t sender){auto&s=*(NativeOrderClient*)p;uint32_t from=(uint32_t)sender;return uc_entity_listener_broadcast(s.world_->listeners(),&s.world_->registry(),event,payload,size,&from);};
 h.remove=[](void*p){return ((NativeOrderClient*)p)->remove();};
 h.retire=[](void*p){return ((NativeOrderClient*)p)->retire();};return h;
}
int NativeOrderClient::create(NativeMissionWorld&w,int32_t parent,const char*name,const NativeOrderClientCallbacks&callbacks,NativeOrderClient**out){
 if(!out||!name||!*name||!callbacks.accept||!callbacks.attached||!callbacks.choose||!callbacks.completed||
  !uc_entity_event_lookup(&w.registry(),parent))return 1;
 auto*s=new NativeOrderClient;s->world_=&w;s->callbacks_=callbacks;
 if(w.bind_dynamic({&s->entity.view,s,step,[](void*){return 0;}, //source virtual34=11030
  [](void*p){auto*s=(NativeOrderClient*)p;int status=s->close_detached();if(!status)delete s;return status;},nullptr,
  [](void*p){return ((NativeOrderClient*)p)->remove();},
  [](void*p){auto*s=(NativeOrderClient*)p;int status=s->close_detached();if(!status)delete s;return status;}})){delete s;return 1;}
 *out=s;auto c=w.context();const void*definition=name;const void*source_context=nullptr;c.definition=&definition;
 *c.registration->creation={0,1,parent};
 if(uc_entity_base_construct_parent(&c,&s->entity.view,&source_context))return s->failure_=2;
 uc_order_client_init(&s->state_);
 if(uc_entity_subscriptions_create(s,&s->subscriptions_))return s->failure_=2;
 int inserted=0;if(uc_entity_listener_register(&w.registry(),s->entity.id,s->entity.id,
  uc_entity_subscriptions_listener(s->subscriptions_),w.listeners(),&inserted)||!inserted)return s->failure_=2;
 for(uint32_t event:{127u,130u,126u,136u}){
  UCEntityEventSubscription row{event,nullptr};
  switch(event){
   case 127:row.receive=[](void*p,const void*v,uint32_t*from){auto&s=*(NativeOrderClient*)p;auto h=s.services();return !from?1:uc_order_client_event(&s.state_,&h,127,v,(int32_t)*from);};break;
   case 130:row.receive=[](void*p,const void*v,uint32_t*from){auto&s=*(NativeOrderClient*)p;auto h=s.services();return !from?1:uc_order_client_event(&s.state_,&h,130,v,(int32_t)*from);};break;
   case 126:row.receive=[](void*p,const void*v,uint32_t*from){auto&s=*(NativeOrderClient*)p;auto h=s.services();return uc_order_client_event(&s.state_,&h,126,v,from?(int32_t)*from:-1);};break;
   case 136:row.receive=[](void*p,const void*v,uint32_t*from){auto&s=*(NativeOrderClient*)p;auto h=s.services();return uc_order_client_event(&s.state_,&h,136,v,from?(int32_t)*from:-1);};break;
  }
  if(uc_entity_subscriptions_append(s->subscriptions_,&row))return s->failure_=2;
 }
 return 0;
}
int NativeOrderClient::step(void*p){return ((NativeOrderClient*)p)->update();}
NativeOrderClient*NativeOrderClient::find(NativeMissionWorld&w,int32_t id){
 auto*t=uc_entity_event_lookup(&w.registry(),id);if(!t)return nullptr;
 for(uint32_t i=w.source_count();i<w.source_count()+1024;++i){auto*b=w.binding(i);if(!b)continue;
  if(b->view->target.identity==t)return b->step==step?(NativeOrderClient*)b->owner:nullptr;
 }return nullptr;
}
int NativeOrderClient::update(){if(!ready())return failure_?failure_:0;auto h=services();return uc_order_client_update(&state_,&h,entity.id,entity.parent)?failure_=2:0;}
int NativeOrderClient::complete(int32_t command){if(!ready())return 1;auto h=services();return uc_order_client_complete(&state_,&h,entity.id,command)?failure_=2:0;}
int NativeOrderClient::advertise(const NativeOrderAction&action){if(!ready()||!action.name||!action.icon)return 1;auto h=services();return h.send(this,state_.manager,124,&action,sizeof action,entity.id)?failure_=2:0;}
int NativeOrderClient::integer(const char*name,int32_t*out,uint8_t*found)const{
 if(!name||!out||!found||!ready())return 1;if(!callbacks_.integer){*found=0;return 0;}
 return callbacks_.integer(callbacks_.owner,name,out,found);
}
int NativeOrderClient::vector(const char*name,float*out,uint8_t*found)const{
 if(!name||!out||!found||!ready())return 1;if(!callbacks_.vector){*found=0;return 0;}
 return callbacks_.vector(callbacks_.owner,name,out,found);
}
int NativeOrderClient::remove(){if(removed_)return 0;if(failure_)return failure_;
 if(callbacks_.removing&&callbacks_.removing(callbacks_.owner))return failure_=2;
 auto h=services();return uc_order_client_remove(&state_,&h,entity.id)?failure_=2:0;}
int NativeOrderClient::retire(){if(removed_)return 0;
 if(entity.helper||entity.aux||entity.actor.handleBC||entity.actor.audioC0||entity.source!=-1)return 1;
 entity.flags|=0x8100;uint8_t queued=0;if(uc_entity_retire(&world_->retirement(),entity.id,&queued)||!queued)return 2;
 if(entity.identity.listeners[0]){if(uc_entity_listener_list_remove_first(world_->listeners(),entity.id))return 2;for(auto&v:entity.identity.listeners)v=nullptr;}
 entity.flags|=0x80000;removed_=true;return 0;
}
int NativeOrderClient::close_detached(){if(uc_entity_event_lookup(&world_->registry(),entity.id)==&entity.identity)return 1;
 uc_entity_subscriptions_close(subscriptions_);subscriptions_=nullptr;
 auto closed=callbacks_.closed;callbacks_.closed=nullptr;if(closed)closed(callbacks_.owner);return 0;}

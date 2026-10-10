#include "order_manager.h"
#include <algorithm>
#include <cstring>
#include <list>
struct NativeOrderManager::Impl {
 NativeOrderManager *self;NativeMissionWorld *world=nullptr;NativeOrderManagerServices services{};
 UCOrderManager selection{};NativeOrderLook look{};UCEntitySubscriptions *subscriptions=nullptr;
 struct Available{int32_t id;uint8_t accepted;};struct Client{int32_t id;std::vector<UCOrderManagerAction> actions;};
 std::list<Available> available;std::list<Client> clients;std::list<int32_t> vetoes;
 std::vector<NativeOrderSprite> sprites;std::vector<UCOrderManagerAction> actions;std::vector<uint32_t> eligible;
 uint32_t next_hud=2,selected=0,player=0;float selected_time=0;int failure=0;bool removed=false;
 explicit Impl(NativeOrderManager*p):self(p){}~Impl(){uc_entity_subscriptions_close(subscriptions);}
 auto client(int32_t id){return std::find_if(clients.begin(),clients.end(),[=](const Client&v){return v.id==id;});}
 int send(int32_t target,uint32_t event,const void*payload,uint32_t bytes){uint8_t handled;uint32_t from=(uint32_t)self->entity.id;return uc_entity_event_send(&world->registry(),target,event,payload,bytes,&from,&handled);}
 int property(int32_t id,const char*name,uint8_t fallback,uint8_t*out){
  if(auto*c=NativeOrderClient::find(*world,id)){int32_t value=0;uint8_t found=0;if(c->integer(name,&value,&found))return 1;*out=found?value!=0:fallback;return 0;}
  return services.property?services.property(services.owner,id,name,fallback,out):1;
 }
 int hud(uint32_t id,uint8_t mode){auto it=std::find_if(sprites.begin(),sprites.end(),[=](const auto&v){return v.id==id;});
  if(mode==2){selected=id;for(auto&s:sprites)if(s.id==id){s.color[0]=120.f/255.f;s.color[1]=200.f/255.f;s.color[2]=1;s.alpha=1;selected_time=1;}else s.alpha=0;return 0;}
  if(it==sprites.end())return 0;
  if(mode){for(auto&v:it->color)v=1;it->alpha=1;if(selected==id)selected=0;}
  else if(selected==id&&!(selected_time<0))selected_time=(float)((double)selected_time-*world->script_services().delta);
  else it->alpha=0;return 0;
 }
 int event(uint32_t event,const void*payload,int32_t from){auto c=client(from);
  if(event==125){if(c==clients.end()&&std::none_of(available.begin(),available.end(),[=](const auto&v){return v.id==from;})){if(available.size()==1024)return 1;available.push_back({from,0});}return 0;}
  if(event==129){if(!payload)return 1;const uint8_t accepted=*(const uint8_t*)payload;
   auto a=std::find_if(available.begin(),available.end(),[=](const auto&v){return v.id==from;});if(a!=available.end())a->accepted=accepted;
   if(accepted){if(c==clients.end()){if(clients.size()==1024)return 1;clients.push_back({from,{}});}const uint32_t zero=0;if(send(from,130,&zero,4))return 2;
    a=std::find_if(available.begin(),available.end(),[=](const auto&v){return v.id==from;});if(a!=available.end())available.erase(a);}
   return 0;
  }
  if(event==132){if(c!=clients.end()){for(const auto&a:c->actions)sprites.erase(std::remove_if(sprites.begin(),sprites.end(),[&](const auto&v){return v.id==a.hud;}),sprites.end());clients.erase(c);}return 0;}
  if(event==124){if(c==clients.end())return 0;if(!payload)return 1;const auto&a=*(const NativeOrderAction*)payload;
   if(!a.name||!a.icon||uint32_t(a.slot)>3)return 1;
   if(std::any_of(c->actions.begin(),c->actions.end(),[&](const auto&v){return v.command==a.id;}))return 0;
   if(sprites.size()==8193||c->actions.size()==8192||next_hud==UINT32_MAX)return 1;
   const uint32_t ax[]={2,2,1,0},ay[]={0,1,2,2};const uint32_t id=next_hud++;
   sprites.push_back({id,from,a.id,a.slot,a.icon,ax[a.slot],ay[a.slot],{1,1,1},0});c->actions.push_back({from,a.id,a.name,(uint32_t)a.slot,id,a.contextual});return 0;
  }
  if(event==133){if(c==clients.end())return 0;if(!payload)return 1;int32_t command;std::memcpy(&command,payload,4);
   auto a=std::find_if(c->actions.begin(),c->actions.end(),[=](const auto&v){return v.command==command;});if(a!=c->actions.end()){
    uint32_t id=a->hud;sprites.erase(std::remove_if(sprites.begin(),sprites.end(),[=](const auto&v){return v.id==id;}),sprites.end());c->actions.erase(a);}return 0;
  }
  if(event==123){if(vetoes.size()==65536)return 1;vetoes.push_back(from);return 0;}
  if(event==131){auto v=std::find(vetoes.begin(),vetoes.end(),from);if(v!=vetoes.end())vetoes.erase(v);return 0;}
  if(event==136){if(c==clients.end())return 0;if(!payload)return 1;uint8_t show=0;if(property(from,"show order on hud",1,&show))return 1;if(!show)return 0;
   int32_t command;std::memcpy(&command,payload,4);auto a=std::find_if(c->actions.begin(),c->actions.end(),[=](const auto&v){return v.command==command;});
   //157450->70D70 publishes the actual texture name as the action feedback;
   //the root presentation owner consumes the same source ordinal via prompt.
   if(a!=c->actions.end()){if(!services.action_feedback)return 1;auto s=std::find_if(sprites.begin(),sprites.end(),[&](const auto&v){return v.id==a->hud;});
    if(s!=sprites.end())return services.action_feedback(services.owner,s->material);}return 0;
  }return 1;
 }
 UCOrderManagerServices core(){UCOrderManagerServices h{};h.owner=this;
  h.look=[](void*p){auto&s=*(Impl*)p;s.look.type=0;s.look.object=-1;if(s.clients.empty())return 0;return s.services.look?s.services.look(s.services.owner,s.self->entity.parent,s.look):1;};
  h.property=[](void*p,int32_t id,const char*n,uint8_t d,uint8_t*out){return ((Impl*)p)->property(id,n,d,out);};
  h.veto=[](void*p,const char*n,uint8_t*out){auto&s=*(Impl*)p;*out=1;for(int32_t id:s.vetoes){uint8_t handled=0,accepted=1;
   if(!s.services.veto||s.services.veto(s.services.owner,id,n,s.self->entity.id,&handled,&accepted))return 1;if(handled&&!accepted){*out=0;break;}}return 0;};
  h.control=[](void*p,uint8_t*out){auto&s=*(Impl*)p;return s.services.control?s.services.control(s.services.owner,out):1;};
  h.gameplay=[](void*p,uint8_t*out){auto&s=*(Impl*)p;return s.services.gameplay?s.services.gameplay(s.services.owner,out):1;};
  h.input=[](void*p,uint32_t q,uint32_t m,double*out){auto&s=*(Impl*)p;return s.services.input?s.services.input(s.services.owner,s.player,q,m,out):1;};
  h.delta=[](void*p,double*out){auto&s=*(Impl*)p;*out=*s.world->script_services().delta;return 0;};
  h.hud=[](void*p,uint32_t id,uint8_t mode){return ((Impl*)p)->hud(id,mode);};
  h.send=[](void*p,int32_t id,int32_t command){return ((Impl*)p)->send(id,126,&command,4);};
  h.sound=[](void*p){auto&s=*(Impl*)p;return s.services.sound?s.services.sound(s.services.owner):1;};
  h.forced=[](void*p){auto&s=*(Impl*)p;return s.services.forced?s.services.forced(s.services.owner):1;};
  h.prompt=[](void*p,uint32_t i){auto&s=*(Impl*)p;return s.services.prompt?s.services.prompt(s.services.owner,i):1;};return h;
 }
};
NativeOrderManager::NativeOrderManager():p_(std::make_unique<Impl>(this)){}
NativeOrderManager::~NativeOrderManager()=default;
int NativeOrderManager::construct(NativeMissionWorld&w,int32_t parent,uint32_t player,const NativeOrderManagerServices&h){auto&s=*p_;
 if(s.world||!uc_entity_event_lookup(&w.registry(),parent))return 1;s.world=&w;s.services=h;s.player=player;
 if(w.bind_dynamic({&entity.view,this,step,[](void*p){return ((NativeOrderManager*)p)->late();},
  [](void*p){return ((NativeOrderManager*)p)->close_detached();},nullptr,[](void*p){return ((NativeOrderManager*)p)->remove();},[](void*p){return ((NativeOrderManager*)p)->close_detached();}}))return s.failure=2;
 auto c=w.context();const void*definition="OrderManager",*source_context=nullptr;c.definition=&definition;*c.registration->creation={0,1,parent};
 if(uc_entity_base_construct_parent(&c,&entity.view,&source_context))return s.failure=2;
 uc_order_manager_init(&s.selection);if(uc_entity_subscriptions_create(&s,&s.subscriptions))return s.failure=2;
 int inserted=0;if(uc_entity_listener_register(&w.registry(),entity.id,entity.id,uc_entity_subscriptions_listener(s.subscriptions),w.listeners(),&inserted)||!inserted)return s.failure=2;
 const UCEntityEventSubscription rows[]={
  {125,[](void*p,const void*v,uint32_t*f){return f?((Impl*)p)->event(125,v,(int32_t)*f):1;}},
  {129,[](void*p,const void*v,uint32_t*f){return f?((Impl*)p)->event(129,v,(int32_t)*f):1;}},
  {132,[](void*p,const void*v,uint32_t*f){return f?((Impl*)p)->event(132,v,(int32_t)*f):1;}},
  {124,[](void*p,const void*v,uint32_t*f){return f?((Impl*)p)->event(124,v,(int32_t)*f):1;}},
  {133,[](void*p,const void*v,uint32_t*f){return f?((Impl*)p)->event(133,v,(int32_t)*f):1;}},
  {123,[](void*p,const void*v,uint32_t*f){return f?((Impl*)p)->event(123,v,(int32_t)*f):1;}},
  {131,[](void*p,const void*v,uint32_t*f){return f?((Impl*)p)->event(131,v,(int32_t)*f):1;}},
  {136,[](void*p,const void*v,uint32_t*f){return f?((Impl*)p)->event(136,v,(int32_t)*f):1;}}};
 for(const auto&r:rows)if(uc_entity_subscriptions_append(s.subscriptions,&r))return s.failure=2;
 s.sprites.push_back({1,-1,0,-1,"Hud/HUD_Orders_Pad_XBOX.shader",2,1,{1,1,1},1});return 0;
}
int NativeOrderManager::step(void*p){return ((NativeOrderManager*)p)->update();}
int NativeOrderManager::update(){auto&s=*p_;if(s.failure||s.removed)return s.failure;
 const uint32_t zero=0;
 for(auto a=s.available.begin();a!=s.available.end();){int32_t id=a->id;
  if(uc_entity_event_lookup(&s.world->registry(),id)){++a;if(s.send(id,127,&zero,4))return s.failure=2;}
  else a=s.available.erase(a);
 }
 s.actions.clear();for(const auto&c:s.clients)s.actions.insert(s.actions.end(),c.actions.begin(),c.actions.end());s.eligible.resize(s.actions.size());auto h=s.core();
 if(uc_order_manager_select(&s.selection,&h,s.actions.data(),(uint32_t)s.actions.size(),(uint32_t)s.clients.size(),entity.parent,1,s.eligible.data(),(uint32_t)s.eligible.size()))return s.failure=2;return 0;
}
int NativeOrderManager::late(){auto&s=*p_;if(s.failure||s.removed)return s.failure;auto*t=uc_entity_event_lookup(&s.world->registry(),entity.parent);
 if(t){const auto&c=s.world->context();auto*v=c.resolve(c.owner,t);if(!v)return s.failure=2;std::memcpy(entity.transform,v->transform,64);}return 0;
}
int NativeOrderManager::remove(){auto&s=*p_;if(s.removed)return 0;if(s.failure)return s.failure;if(entity.helper||entity.aux||entity.source!=-1)return 1;
 entity.flags|=0x8100;uint8_t queued=0;if(uc_entity_retire(&s.world->retirement(),entity.id,&queued)||!queued)return s.failure=2;
 if(entity.identity.listeners[0]){if(uc_entity_listener_list_remove_first(s.world->listeners(),entity.id))return s.failure=2;for(auto&v:entity.identity.listeners)v=nullptr;}
 entity.flags|=0x80000;s.removed=true;return 0;
}
int NativeOrderManager::close_detached(){auto&s=*p_;if(uc_entity_event_lookup(&s.world->registry(),entity.id)==&entity.identity)return 1;
 uc_entity_subscriptions_close(s.subscriptions);s.subscriptions=nullptr;s.removed=true;return 0;
}
NativeOrderManager*NativeOrderManager::find(NativeMissionWorld&w,int32_t id){auto*t=uc_entity_event_lookup(&w.registry(),id);if(!t)return nullptr;
 for(uint32_t i=w.source_count();i<w.source_count()+1024;++i){auto*b=w.binding(i);if(b&&b->view->target.identity==t)return b->step==step?(NativeOrderManager*)b->owner:nullptr;}return nullptr;
}
int NativeOrderManager::integer(const char*n,int32_t*out,uint8_t*found)const{if(!n||!out||!found||!p_->world||p_->removed)return 1;auto&s=*p_;*found=1;
 if(!std::strcmp(n,"Look Object ID"))*out=s.look.object;else if(!std::strcmp(n,"Look Object Type"))*out=s.look.type;
 else if(!std::strcmp(n,"orders available"))*out=s.selection.available;
 else if(!std::strcmp(n,"forced orders available"))*out=s.selection.available&&s.selection.forced_client!=-1;
 else if(!std::strcmp(n,"orders menu shown"))*out=s.selection.forcing;
 else if(!std::strcmp(n,"player holding order button"))*out=s.selection.held;else *found=0;return 0;
}
int NativeOrderManager::vector(const char*n,float*out,uint8_t*found)const{if(!n||!out||!found||!p_->world||p_->removed)return 1;*found=1;
 if(!std::strcmp(n,"Position"))std::memcpy(out,entity.transform+12,12);else if(!std::strcmp(n,"Player Look Point"))std::memcpy(out,p_->look.point,12);else *found=0;return 0;
}
const std::vector<NativeOrderSprite>&NativeOrderManager::sprites()const{return p_->sprites;}
const UCOrderManager&NativeOrderManager::selection()const{return p_->selection;}

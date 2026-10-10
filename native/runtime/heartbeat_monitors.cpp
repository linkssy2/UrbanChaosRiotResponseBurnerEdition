#include "heartbeat_monitors.h"
#include <stdexcept>
UCHudHeartbeatActor NativeHeartbeatMonitors::actor(int32_t id)const{UCHudHeartbeatActor a{};auto*t=uc_entity_event_lookup(&world_->registry(),id);if(!t)return a;
 const auto&c=world_->context();auto*v=c.resolve(c.owner,t);if(!v||!v->actor)throw std::runtime_error("Heartbeat actor health projection");
 a.present=1;a.health=v->actor->health9C;a.maximum=v->actor->maximumA0;return a;
}
UCHeartbeatMonitorServices NativeHeartbeatMonitors::services(){UCHeartbeatMonitorServices h{};h.owner=this;
 h.bind=[](void*p,uint32_t i,int32_t id,const char*key,const char*joined,const char*left){auto&s=*static_cast<NativeHeartbeatMonitors*>(p);
  //71200 retains left at+58; none of this widget's original virtual methods
  //reads it. Its join cue alone requests type15, after binding the health row.
  (void)left;int result=s.widgets_[i].bind(id,key,s.actor(id).health);
  return result?result:joined?(s.message_?s.message_(s.message_owner_,joined):1):0;};
 h.clear=[](void*p,uint32_t i){static_cast<NativeHeartbeatMonitors*>(p)->widgets_[i].clear();return 0;};
 h.active=[](void*p,uint32_t i){return int(static_cast<NativeHeartbeatMonitors*>(p)->widgets_[i].active());};
 h.exists=[](void*p,int32_t id){return int(uc_entity_event_lookup(&static_cast<NativeHeartbeatMonitors*>(p)->world_->registry(),id)!=nullptr);};
 h.step=[](void*p,uint32_t i,float dt){auto&s=*static_cast<NativeHeartbeatMonitors*>(p);s.widgets_[i].step(s.actor(s.widgets_[i].actor()),dt,*s.random_,s.sound_,s.sound_owner_);return 0;};return h;
}
void NativeHeartbeatMonitors::load(const std::filesystem::path&game,NativeRenderer&r,NativeDialogueText&text,const UCArchive*archive,NativeMissionWorld&w,UCGameRandom&random,void(*sound)(void*),void*owner){
 if(world_||!archive||!sound)throw std::runtime_error("Heartbeat monitor lifetime");for(auto&v:widgets_)v.load(game,r,text,archive);
 uc_heartbeat_monitors_init(&state_);world_=&w;random_=&random;sound_=sound;sound_owner_=owner;
}
int NativeHeartbeatMonitors::show(int32_t id,const char*text){if(!world_)return 1;auto h=services();return uc_heartbeat_monitors_show(&state_,id,text,&h);}
int NativeHeartbeatMonitors::join(int32_t id,const char*text,const char*joined,const char*left){if(!world_)return 1;auto h=services();return uc_heartbeat_monitors_join(&state_,id,text,joined,left,&h);}
int NativeHeartbeatMonitors::remove(int32_t id){if(!world_)return 1;auto h=services();return uc_heartbeat_monitors_remove(&state_,id,&h);}
void NativeHeartbeatMonitors::step(float dt){if(!world_)throw std::runtime_error("Heartbeat monitor is not loaded");auto h=services();if(uc_heartbeat_monitors_step(&state_,dt,&h))throw std::runtime_error("Heartbeat monitor step71460");}
bool NativeHeartbeatMonitors::order_available(int32_t id)const{
 if(id==-1)return false;if(!order_)throw std::runtime_error("Heartbeat order property provider missing");
 int32_t order=-1;uint8_t found=0;if(order_(order_owner_,id,&order,&found))throw std::runtime_error("Heartbeat order property query");return found&&order!=-1;
}
int NativeHeartbeatMonitors::action_feedback(ID3D11ShaderResourceView*image){
 if(!world_||!image)return 1;for(const auto&s:state_.slots)if(order_available(s.actor))widgets_[s.widget].action_feedback(image);return 0;
}
void NativeHeartbeatMonitors::draw(NativeRenderer&r,float x,float y,const float color[4]){if(!world_)return;y+=30.f;y=(float)(uc_heartbeat_monitors_ease(state_.slide)*30.+y);
 for(const auto&slot:state_.slots){auto&w=widgets_[slot.widget];w.draw(r,x,y,color,w.has_feedback()&&order_available(w.actor()));y+=30.f;}}

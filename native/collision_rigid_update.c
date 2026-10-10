#include "collision_rigid_update.h"
#include <stdlib.h>
#include <string.h>
UCCollisionStatus uc_collision_rigid_update_init(UCCollisionRigidUpdate *out,const void *world,
 const UCCollisionPhantomGates *g,const UCCollisionRigidUpdateHooks *h,void *context,uint32_t pairs,uint32_t agents) {
 if(!out||out->world||!world||!g||!g->busy||!g->pending_requests||!g->drain_blocked||!h||
  !h->preflight||!h->member||!h->selected||!h->query||!h->other||!h->filter||!h->phantom_refresh||
  !h->phantom_child||!h->agents||!h->agent_pair||!h->reconcile||!h->enabled||!h->category||!h->matrix||
  !h->remove_agent||!h->dirty_island||!h->update_agent||!h->add_pairs||!h->enqueue||!h->drain||pairs>32768||agents>32768)return UC_COLLISION_ARGUMENT;
 UCCollisionRigidUpdate u={0};u.queried.data=pairs?calloc(pairs,sizeof(*u.queried.data)):NULL;
 u.existing.data=agents?calloc(agents,sizeof(*u.existing.data)):NULL;
 if((pairs&&!u.queried.data)||(agents&&!u.existing.data)){free(u.queried.data);free(u.existing.data);return UC_COLLISION_MEMORY;}
 u.world=world;u.context=context;u.gates=*g;u.hooks=*h;u.queried.capacity=pairs;u.existing.capacity=agents;*out=u;return UC_COLLISION_OK;
}
UCCollisionStatus uc_collision_rigid_update_request(UCCollisionRigidUpdate *u,void *body,uint32_t mode,uint32_t child,int *applied) {
 if(!u||!u->world||!body||!applied)return UC_COLLISION_ARGUMENT;
 if(*u->gates.busy){UCCollisionStatus s=u->hooks.enqueue(u->context,body,(uint8_t)mode,(uint8_t)child);if(s)return s;++*u->gates.pending_requests;*applied=0;return UC_COLLISION_OK;}
 if(u->active)return UC_COLLISION_UNSUPPORTED;
 UCCollisionRigidUpdateMember member=u->hooks.member(u->context,body);
 const UCCollisionPhantomPairBody *selected=u->hooks.selected(u->context,body);
 if(member.world!=u->world||!member.handle||member.handle->index==UC_COLLISION_SAP_DETACHED||
  !selected||!selected->identity||selected->type!=1||selected->owner!=body)return UC_COLLISION_ARGUMENT;
 UCCollisionStatus s=u->hooks.preflight(u->context,body,mode,child);if(s)return s;
 u->active=1;++*u->gates.busy;u->queried.count=u->existing.count=0;
 if(!mode){
  u->hooks.query(u->context,body,&u->queried);
  for(uint32_t i=0;i<u->queried.count;){
   const UCCollisionPhantomPairBody *other=u->hooks.other(u->context,u->queried.data[i].second);
   if(other->type==2&&other->owner){
    const void *filter=u->hooks.filter(u->context);u->hooks.phantom_refresh(u->context,filter,other,selected);
    if(child)u->hooks.phantom_child(u->context,other->owner);
    u->queried.data[i]=u->queried.data[--u->queried.count];
   }else ++i;
  }
  UCCollisionRigidUpdateAgents list=u->hooks.agents(u->context,body);
  for(uint32_t i=0;i<list.count;i++){
   u->existing.data[u->existing.count++]=u->hooks.agent_pair(u->context,list.data[i].agent);
   list=u->hooks.agents(u->context,body);
  }
  u->hooks.reconcile(u->context,&u->existing,&u->queried);
 }
 uint16_t selected_category=u->hooks.category(u->context,selected->identity);
 UCCollisionRigidUpdateAgents list=u->hooks.agents(u->context,body);
 for(uint32_t i=0;i<list.count;){
  UCCollisionPairAgent entry=list.data[i];const void *filter=u->hooks.filter(u->context);
  if(!u->hooks.enabled(u->context,filter,selected->identity,entry.other_collidable)||
   !u->hooks.matrix(u->context,selected_category,u->hooks.category(u->context,entry.other_collidable))){
   u->hooks.remove_agent(u->context,entry.agent);u->hooks.dirty_island(u->context,body);
  }else {if(child==1)u->hooks.update_agent(u->context,entry.agent);++i;}
  list=u->hooks.agents(u->context,body);
 }
 if(u->queried.count){const void *filter=u->hooks.filter(u->context);u->hooks.add_pairs(u->context,filter,&u->queried);}
 --*u->gates.busy;u->active=0;*applied=1;
 if(!*u->gates.busy&&*u->gates.pending_requests&&!*u->gates.drain_blocked)return u->hooks.drain(u->context);
 return UC_COLLISION_OK;
}
UCCollisionStatus uc_collision_rigid_update_dispose(UCCollisionRigidUpdate *u) {
 if(!u||!u->world)return UC_COLLISION_ARGUMENT;if(u->active)return UC_COLLISION_UNSUPPORTED;
 free(u->queried.data);free(u->existing.data);memset(u,0,sizeof(*u));return UC_COLLISION_OK;
}

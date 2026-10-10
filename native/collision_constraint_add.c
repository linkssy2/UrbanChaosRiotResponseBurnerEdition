#include "collision_constraint_add.h"
static int endpoint_valid(const UCCollisionConstraintEndpoint *b) {
 return b&&b->storage&&b->reference&&b->reference->identity&&b->reference->allocation_marker&&
  b->reference->references&&b->reference->allocation_marker!=b->reference->references;
}
UCCollisionStatus uc_collision_constraint_resolve_preview(const UCCollisionConstraintDefinition *d,
 const UCCollisionConstraintResolveHooks *h,void *ctx,UCCollisionConstraintEndpoint *resolved[2]) {
 if(!d||!d->constraint||!h||!h->current_world||!h->fixed_endpoint||!resolved)return UC_COLLISION_ARGUMENT;
 UCCollisionConstraintEndpoint *body[2]={d->body[0],d->body[1]};
 for(unsigned i=0;i<2;i++)if(body[i]&&!endpoint_valid(body[i]))return UC_COLLISION_ARGUMENT;
 for(unsigned i=0;i<2;i++)if(!body[i]&&body[1-i]) {
  const void *world=h->current_world(ctx,body[1-i]);
  if(world){body[i]=h->fixed_endpoint(ctx,world);if(!endpoint_valid(body[i]))return UC_COLLISION_ARGUMENT;}
 }
 if(body[0]&&body[1]) {
  UCCollisionContactReference *a=body[0]->reference,*b=body[1]->reference;
  if(a->identity==b->identity&&(a->allocation_marker!=b->allocation_marker||a->references!=b->references))return UC_COLLISION_ARGUMENT;
 }
 resolved[0]=body[0];resolved[1]=body[1];return UC_COLLISION_OK;
}
static void publish(UCCollisionConstraintDefinition *d,UCCollisionConstraintEndpoint *const body[2]) {
 for(unsigned i=0;i<2;i++)if(!d->body[i]&&body[i]) {
  d->body[i]=body[i];UCCollisionContactReference *r=body[i]->reference;if(*r->allocation_marker)++*r->references;
 }
}
UCCollisionStatus uc_collision_constraint_resolve_bodies(UCCollisionConstraintDefinition *d,
 const UCCollisionConstraintResolveHooks *h,void *ctx) {
 UCCollisionConstraintEndpoint *body[2];UCCollisionStatus s=uc_collision_constraint_resolve_preview(d,h,ctx,body);
 if(!s)publish(d,body);return s;
}
static UCCollisionStatus admit(UCCollisionConstraintAddition *a,UCCollisionConstraintDefinition *d,uint32_t notify,UCCollisionConstraintEndpoint *body[2]) {
 if(!a||!a->world||!a->gates.busy||!a->gates.pending_requests||!a->gates.drain_blocked||
  !a->hooks.preflight||!a->hooks.install||!a->hooks.notify||!a->hooks.drain)return UC_COLLISION_ARGUMENT;
 if(a->active)return UC_COLLISION_UNSUPPORTED;
 UCCollisionStatus s=uc_collision_constraint_resolve_preview(d,&a->hooks.resolve,a->context,body);if(s)return s;
 return a->hooks.preflight(a->context,d,body,notify);
}
UCCollisionStatus uc_collision_constraint_addition_validate(UCCollisionConstraintAddition *a,UCCollisionConstraintDefinition *d,uint32_t notify) {
 UCCollisionConstraintEndpoint *body[2];return admit(a,d,notify,body);
}
UCCollisionStatus uc_collision_constraint_addition_request(UCCollisionConstraintAddition *a,UCCollisionConstraintDefinition *d,uint32_t notify,int *applied) {
 if(!applied)return UC_COLLISION_ARGUMENT;
 UCCollisionConstraintEndpoint *body[2];UCCollisionStatus s=admit(a,d,notify,body);if(s)return s;
 UCCollisionConstraint *constraint=d->constraint;*applied=1;a->active=1;publish(d,body);++*a->gates.busy;
 a->hooks.install(a->context,d);if(notify)a->hooks.notify(a->context,constraint);
 --*a->gates.busy;a->active=0;
 if(!*a->gates.busy&&*a->gates.pending_requests&&!*a->gates.drain_blocked)return a->hooks.drain(a->context);
 return UC_COLLISION_OK;
}

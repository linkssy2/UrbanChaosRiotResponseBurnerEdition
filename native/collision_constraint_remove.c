#include "collision_constraint_remove.h"
UCCollisionStatus uc_collision_constraint_removal_validate(UCCollisionConstraintRemoval *r,void *constraint,uint32_t notify) {
 if(!r||!r->world||!constraint||!r->gates.busy||!r->gates.pending_requests||!r->gates.drain_blocked||
  !r->hooks.preflight||!r->hooks.listener_count||!r->hooks.notify||!r->hooks.unlink||!r->hooks.drain)return UC_COLLISION_ARGUMENT;
 if(r->active)return UC_COLLISION_UNSUPPORTED;
 return r->hooks.preflight(r->context,constraint,notify);
}
UCCollisionStatus uc_collision_constraint_removal_request(UCCollisionConstraintRemoval *r,void *constraint,uint32_t notify,int *applied) {
 if(!applied)return UC_COLLISION_ARGUMENT;
 UCCollisionStatus s=uc_collision_constraint_removal_validate(r,constraint,notify);if(s)return s;
 *applied=1;r->active=1;++*r->gates.busy;
 if(notify&&r->hooks.listener_count(r->context))r->hooks.notify(r->context,constraint);
 r->hooks.unlink(r->context,constraint);
 --*r->gates.busy;r->active=0;
 if(!*r->gates.busy&&*r->gates.pending_requests&&!*r->gates.drain_blocked)return r->hooks.drain(r->context);
 return UC_COLLISION_OK;
}

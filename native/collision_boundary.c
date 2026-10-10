#include "collision_boundary.h"
#include <math.h>
#include <string.h>

static int complete(const UCCollisionBoundaryHooks *h) {
    return h && h->observer && h->create && h->listener && h->add && h->remove_six && h->release;
}
UCCollisionStatus uc_collision_boundary_init(UCCollisionBoundary *s,
    const void *world, const float lower[4], const float upper[4],
    uint32_t mode, unsigned managed, const UCCollisionBoundaryHooks *h, void *context) {
    if (!s || !world || !lower || !upper || !complete(h) || managed > 1 ||
        s->world || s->references || s->managed || s->mode) return UC_COLLISION_ARGUMENT;
    for (unsigned i=0;i<6;i++) if (s->phantoms[i]) return UC_COLLISION_ARGUMENT;
    for (unsigned i=0;i<3;i++) if (!isfinite(lower[i]) || !isfinite(upper[i]) || lower[i]>upper[i]) return UC_COLLISION_ARGUMENT;
    s->world=world; s->mode=mode; s->managed=(uint16_t)managed; s->references=(uint16_t)(1+managed);
    h->observer(context,world,s,1);
    for (unsigned i=0;i<6;i++) {
        float lo[4]={0},hi[4]={0};
        memcpy(lo,lower,3*sizeof(float)); memcpy(hi,upper,3*sizeof(float));
        lo[i/2]=hi[i/2]=(i&1)?lower[i/2]:upper[i/2];
        void *p=h->create(context,lo,hi,0);
        h->listener(context,p,s,1); s->phantoms[i]=p; h->add(context,world,p);
    }
    return UC_COLLISION_OK;
}
UCCollisionStatus uc_collision_boundary_detach(UCCollisionBoundary *s,
    const UCCollisionBoundaryHooks *h, void *context) {
    if (!s || !complete(h) || s->managed>1) return UC_COLLISION_ARGUMENT;
    if (!s->world) return UC_COLLISION_OK;
    if (s->references < 1+s->managed) return UC_COLLISION_ARGUMENT;
    for (unsigned i=0;i<6;i++) if (!s->phantoms[i]) return UC_COLLISION_ARGUMENT;
    h->remove_six(context,s->world,s->phantoms);
    for (unsigned i=0;i<6;i++) {
        h->listener(context,s->phantoms[i],s,0);
        h->release(context,s->phantoms[i]); s->phantoms[i]=NULL;
    }
    h->observer(context,s->world,s,0);
    if (s->managed) --s->references;
    s->world=NULL;
    return UC_COLLISION_OK;
}
UCCollisionStatus uc_collision_boundary_release_phantoms(UCCollisionBoundary *s,
    const UCCollisionBoundaryHooks *h, void *context) {
    if (!s || !h || !h->release || s->world) return UC_COLLISION_ARGUMENT;
    for (unsigned i=0;i<6;i++) if (s->phantoms[i]) {
        h->release(context,s->phantoms[i]); s->phantoms[i]=NULL;
    }
    return UC_COLLISION_OK;
}

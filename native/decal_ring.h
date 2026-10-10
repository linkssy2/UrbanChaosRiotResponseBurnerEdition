#ifndef UC_DECAL_RING_H
#define UC_DECAL_RING_H
#include <stdint.h>
typedef struct UCDecalEntry {
    void *material;
    float color[3],corners[4][3];
    uint32_t flags;
    float rate,progress;
} UCDecalEntry;
typedef struct UCDecalRing {UCDecalEntry *entries;uint32_t first,next,capacity;} UCDecalRing;
typedef struct UCDecalRingServices {
    void *context;
    int (*preflight)(void *,const char *shader,const float corners[4][3]);
    /* Actual22B280 reference lookup; NULL is an actual nullable result. */
    int (*material)(void *,const char *shader,void **material);
    /* Actual898A0 graphics+2C provider and +28 packed-BGRA point query. */
    int (*light)(void *,const float point[3],uint32_t *bgra);
} UCDecalRingServices;
/* Represented9BE80 ring initialization after actual successful backing storage
 * allocation. capacity is original byte budget/76,2..65536; entries untouched.
 * Registration, full renderable base construction and allocation stay upstream. */
int uc_decal_ring_init(UCDecalRing *,UCDecalEntry *,uint32_t capacity);
/* Full9BBB0 state/publication order with9BA60 reserve. Inputs and services are
 * stable/disjoint; callbacks complete without changing ring/input owners or
 * reentry. Their exact current material/light providers are required. Bounded
 * finite corners (abs<=1e6), nondegenerate face, finite fade(abs<=1e4), RN with
 * DAZ/FTZ off. All-protected full-ring original nontermination is unsupported.
 * Source reserve commits before material lookup, metadata before light query.
 * Return0 complete,1 pre-effect rejection,2 committed service failure.
 * This stores real marks; it does not itself issue GPU draws. */
int uc_decal_ring_insert(UCDecalRing *,const char *shader,const float corners[4][3],
    uint32_t flags,float fade,const UCDecalRingServices *);
/* Whole9BAC0 positive-delta progress and9B920 logical clear. Clear retains all
 * entries. Finite progress/rate/delta with finite intermediates; no callbacks. */
int uc_decal_ring_tick(UCDecalRing *,float delta);
int uc_decal_ring_clear(UCDecalRing *);
#endif

#ifndef UC_GLASS_PROXY_H
#define UC_GLASS_PROXY_H
#include "glass_model.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Borrowed ordered projection of the actual preloaded support map, not another
 * cache or loader. Values are the live embedded metadata identities. Keys are
 * stable ASCII strings <=1023 bytes, strictly ordered by actual3878F0; distinct
 * entries name distinct embedded metadata identities. Native guards bound keys/
 * ordering, not all ownership consistency. Count
 * <=65536. No map, key, payload or factory mutation during a call. */
typedef struct UCModelSupportEntry {const char *name;const void *value;} UCModelSupportEntry;
typedef struct UCModelSupportMap {const UCModelSupportEntry *entries;size_t count;} UCModelSupportMap;
int uc_model_support_find(const UCModelSupportMap *,const char *,const void **);
/* The represented21D0E0 fields. source-unwritten bytes and guest class/layout
 * are not modeled. Model owns its transform/name; no duplicated transform. */
typedef struct UCGlassProxy {
    UCGlassRegistry *model;
    const void *collision4,*support8;
    uint32_t state0C,state34;
    uint8_t visible14,enabled15,flag39;
    float phase18,tint20[4];
    void *state30,*state3C;
} UCGlassProxy;
/* Actual .glass_pane recipe only, with both actual metadata lookups missing.
 * Collision query .glass_panehk misses before .glass_panecol is considered;
 * entity query .ENTSupp also misses. Real preloaded maps must be supplied,
 * not fabricated empty maps. Pure admission verifies this before effects.
 * Original singleton/cache/graphics dispatch is stable; existing factory owns
 * the actual registry/group creation. The two map views, name and identity
 * stay live/disjoint through current-primary publication. Output storage is
 * fresh/unowned and exclusive.0complete,1rejected before effects,2partial
 * factory failure: p->model is the actual partial owner and must be cleaned,
 * never retry. Nonnull support parsing and generic proxy lifetime excluded.
 * Missing-map temporary zero support allocation has no observable callbacks
 * in this profile and is eliminated; the oracle still constructs/frees it.
 * Registry destruction is the existing real registry/group cleanup; caller
 * owns this outer proxy storage and must end all borrows before cleanup. */
int uc_glass_proxy_init(UCGlassProxy *,const UCGlassModelFactory *,const char *name,
    const UCModelSupportMap *collision,const UCModelSupportMap *entity);
#ifdef __cplusplus
}
#endif
#endif

#ifndef UC_GLASS_PANE_H
#define UC_GLASS_PANE_H
#include "glass_bullet.h"
#include "glass_store.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCGlassPane UCGlassPane;
typedef struct UCGlassPaneUV {float uv[4][2];uint32_t initialized;} UCGlassPaneUV;
/* Concrete50040 geometry and transferred450B3C/53520 sink. The initialized,
 * malloc-allocated airborne owner is transferred ONLY on success; its actual
 * ground owner is already transferred to it. The store must retain its actual completing ground destructor. Shared pools, initialized vector/game RNG and
 * UV table remain borrowed/live; no duplicate simulation/storage ownership.
 * The table tests bit0/preserves other bits, publishes00,10,11,01 once. Matrix
 * and quad are copied; two original world triangles012/023 receive alpha1.
 * Native outer allocations replace original caller allocations, not guest ABI.
 * Finite world/quad/UV and existing shard arithmetic domain. NULL means native
 * admission/allocation failure, with ownership and shared table unchanged. */
UCGlassPane *uc_glass_pane_create(UCGlassPaneUV *,const float world[16],const float quad[4][2],UCGlassStore *owned_air,UCVectorRandom *,UCGameRandom *);
/* Actual50640: until triggered, writes pointer andcount2; once triggered writes
 * ONLYcount0, preserving caller's pointer bytes. Returned span belongs to pane.
 * Actual source finish11030 has no effects and needs no invented callback. */
int uc_glass_pane_triangles(const UCGlassPane *,const UCGlassTriangle **,int32_t *);
/* Aliases the SAME current world/quad/trigger/sink slots for qualified520F0.
 * Returned view does not own/pin anything; pane must stay stable until done. */
UCGlassBullet uc_glass_pane_bullet(UCGlassPane *);
/* Actual50670/50690 ->506F0/50710 ->actual airborne/ground teardown, then sink
 * and pane release. All registry/observer borrows must first be removed by
 * their owners (outer51AD0 does ground,air,pane wrapper removal in that order).
 * No accesses/retries after success.0complete,1 invalid beforeeffects. */
int uc_glass_pane_destroy(UCGlassPane *);
#ifdef __cplusplus
}
#endif
#endif

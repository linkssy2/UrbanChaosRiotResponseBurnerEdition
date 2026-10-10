#ifndef UC_DECAL_RENDER_H
#define UC_DECAL_RENDER_H
#include "decal_ring.h"
typedef struct UCDecalBatch UCDecalBatch;
typedef struct UCDecalDrawHooks {
    void (*device)(void *); /* Current graphics+A4/provider+1C, before begin. */
    void (*begin)(void *); /* Quads; default attributes1/5/6=(0,0,0,1),4=(0,0). */
    float (*exposure)(void *); /* Current graphics+40/provider+10 per mark. */
    void (*quad)(void *,const UCDecalEntry *,const float corners[4][3],const float color[4]);
    void (*end)(void *);
} UCDecalDrawHooks;
typedef struct UCDecalRenderHooks {
    int (*preflight)(void *,const UCDecalRing *);
    uint8_t (*special_enabled)(void *); /* Current graphics+60/provider+74. */
    /* Real material+18, parameter0. Completing material service calls
     * uc_decal_draw_batch for each authored geometry pass, synchronously. */
    int (*material)(void *,void *material,const UCDecalBatch *);
} UCDecalRenderHooks;
/* Full9C4A0 grouping and9BF50 geometry policy. Mode!=2 returns without later
 * inputs. Ring/order/material identities remain immutable across all services.
 * Requires nonnull materials and finite bounded corners/colors/progress/rate,
 * RN/gradual underflow. The caller admits finite nonzero exposure with finite
 * reciprocal-scaled color before effects. Pure preflight covers all completing
 * material/backend work. No retained batch pointer or reentry. Source flags2
 * marks are suppressed only at outer selection: a later matching-material
 * batch still includes them, as in the original. Every material pass reemits
 * its matching marks, even when a prior pass marked their selection bits.
 * Scratch selection bits are private per call, not another authoritative ring.
 * Returns0 completed/gated,1 pre-effect rejection,2 committed service failure.
 * GPU material setup, rasterization and parent scene placement remain required.
 */
int uc_decal_render(const UCDecalRing *,uint32_t mode,const UCDecalRenderHooks *,void *);
int uc_decal_draw_batch(const UCDecalBatch *,const UCDecalDrawHooks *,void *);
#endif

#ifndef UC_GLASS_RENDER_H
#define UC_GLASS_RENDER_H
#include "glass_store.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCGlassRender {
 const void *source,*material;
 const UCGlassTriangle *triangles;
 int32_t count;
} UCGlassRender;
typedef struct UCGlassRenderHooks {
 void (*triangles)(void*,const void*,const UCGlassTriangle**,int32_t*);
 void (*finish)(void*,const void*);
 void *(*matrix_owner)(void*);
 const float *(*matrix)(void*,void*);
 void (*set_matrix)(void*,void*,const float[16]);
 void (*material)(void*,const void*,UCGlassRender*); /* actual material+18; invokes draw below */
 void (*cull)(void*,uint32_t);
 void (*begin)(void*,uint32_t);
 void (*attribute)(void*,uint32_t,unsigned,const float*);
 void (*end)(void*);
} UCGlassRenderHooks;
/* ActualAD930/AD870, with a borrowed source store/material and one published
 * triangle span. No duplicate shard storage. source/material/hooks/context and
 * enclosing wrapper remain live. Required services complete; no reentry except
 * material calling draw. Getter publishes any signed count<=256 and a stable
 * span for positive count; graph/count/triangle bytes then remain immutable
 * through finish. Identity is actual caller-owned51E110 matrix, not invented
 * here. Current matrix owner may change at each getter; returned matrix is live
 * through immediate copy. Services implement actual source matrix/material/SDK
 * effects. No implicit material binding or supplied-success draw.
 * Original triangle positions use immediate w0, normal(0,0,1,1), whiteRGB and
 * unchanged per-triangle alpha; no early color packing. Full source draw cull
 * bracket is0 then900 and is absent for nonpositive count. Parent snapshots
 * current matrix, sets identity, draws, restores via a fresh owner lookup,
 * finishes source then clears only triangles (count remains stale).
 * 0 complete;1 pre-effect admission failure;2 violated completing callback
 * contract after effects, no rollback/retry. No allocation/render API here. */
int uc_glass_render(UCGlassRender*,const float identity[16],const UCGlassRenderHooks*,void*);
int uc_glass_render_draw(const UCGlassRender*,const UCGlassRenderHooks*,void*);
#ifdef __cplusplus
}
#endif
#endif

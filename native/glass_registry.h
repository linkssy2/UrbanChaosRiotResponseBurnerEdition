#ifndef UC_GLASS_REGISTRY_H
#define UC_GLASS_REGISTRY_H
#include "glass_render.h"
#include "material_resources.h"
#include "scene_model_registration.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCGlassRegistry UCGlassRegistry;
/* Represented AD3D0 fields: owned unique source->wrapper storage, identity
 * transform and enabled1. No duplicate source, guest vtable/global context stamp
 * or generic graphics factory is manufactured. Capacity1..4096 is preadmitted.
 * The exact immutable source51E110 matrix is supplied by its real parent. */
int uc_glass_registry_create(uint32_t capacity,const float identity[16],UCGlassRegistry **);
/* AD800: actual SAME material resource/cache lookup precedes wrapper allocation.
 * Registry owns stable wrappers, borrows source/material until removal. Source
 * identity is the uintptr ordering key, never a truncated32-bit guest address.
 * Repeated keys are rejected BEFORE lookup: source duplicate insertion leaks its
 * fresh unreferenced wrapper and is outside this bounded unique-registration
 * domain. Count is not observed before AD930's triangle getter publishes it.
 * Archive remains open; missing materials remain NULL, not a fallback shader. */
int uc_glass_registry_add(UCGlassRegistry *,const void *source,UCMaterialResources *,const char *shader);
/* ADC50 deletes the matching wrapper before removing its entry; missing is quiet. */
int uc_glass_registry_remove(UCGlassRegistry *,const void *source);
/* Actual AD7C0 mode2 traverses ascending source identity and completes AD930
 * using the existing concrete render owner. Other modes/empty owner are quiet.
 * Preadmit every source/material/hook before drawing; immutable membership and
 * descriptors through callbacks, no reentry. Render effects may change external
 * state. Failure2 preserves earlier draw effects and is not retryable. */
int uc_glass_registry_draw(UCGlassRegistry *,uint32_t mode,const float identity[16],const UCGlassRenderHooks *,void *);
/* ADD40 wrapper lifetime -> CURRENT graphics+DC group removal -> map storage
 * release. The same existing primary group is used, no parallel registration.
 * Caller must arrange actual attachment externally. Only CURRENT group matches
 * are removed; other borrowed memberships must be retired before close.
 * The getter may change external world state but cannot mutate this registry.
 * On failure2 all local storage is still released; owner must not be reused. */
int uc_glass_registry_close(UCGlassRegistry *,UCSceneModelGroup *(*current_primary)(void *),void *);
uint32_t uc_glass_registry_count(const UCGlassRegistry *);
const UCGlassRender *uc_glass_registry_at(const UCGlassRegistry *,uint32_t index);
const float *uc_glass_registry_matrix(const UCGlassRegistry *);
uint8_t uc_glass_registry_enabled(const UCGlassRegistry *);
/* Actual interface AD500/AD530 and AD570/AD560. Matrix copy preserves all bits.
 * Name is exactly16 bytes: strncpy padding/truncation, NOT guaranteed terminated.
 * The name slot is source-unwritten at construction; native getter returnsNULL
 * until first name publication. Returned views are borrowed until close.
 * Stable disjoint input, exclusive call;1 rejects before effects. */
int uc_glass_registry_set_matrix(UCGlassRegistry *,const float matrix[16]);
int uc_glass_registry_set_name(UCGlassRegistry *,const char *);
const char *uc_glass_registry_name(const UCGlassRegistry *);
/* All inputs disjoint/live and calls exclusive. Return0 complete,1 pre-effect
 * rejection,2 possibly committed cache/allocation/render/removal failure.
 * Native allocation layout/OOM is not original heap equivalence. */
#ifdef __cplusplus
}
#endif
#endif

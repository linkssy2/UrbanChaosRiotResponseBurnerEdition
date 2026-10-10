#ifndef UC_GLASS_MODEL_H
#define UC_GLASS_MODEL_H
#include "glass_registry.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCGlassModelFactory {
    uint32_t capacity;
    const float *identity; /* actual source51E110 supplied by the parent */
    UCSceneModelGroup *(*current_primary)(void *);
    void *context;
} UCGlassModelFactory;
/* Actual91200 glass-handler selection ->9B760/9B780/ADE20/AD3D0 and9EE00
 * initialization/owned0 group append/name publication. Reuses SAME registry
 * and existing scene_model_create; no second factory list or renderer graph.
 * Bounded domain: the active factory contains the actual glass handler as the
 * only potential match (the real189AE0 request is exactly ".glass_pane").
 * Stable disjoint ASCII strings shorter than4096 bytes, capacity1..4096,
 * last-dot extension and first '=' length; compare is
 * case-insensitive over that length, including original prefix matches. Other
 * registered model classes and general graphics startup are NOT implemented.
 * name mayNULL (sourceempty fallback). It is copied to16 actual name bytes.
 * Current primary is read after matrix publication; getter may change external
 * state but keeps inputs, factory, returned group and model live and exclusive.
 * The descriptor and original identity matrix remain stable throughout call.
 * Output owns registry on success. Missing glass-handler match completes with
 * NULL output and no group effects. Allocation failure or failed real append
 * returns2; a nonnull output must be closed through its actual registry/group
 * cleanup, never retried.0complete,1native admission failure before effects.
 * Does not construct21D0E0 animation sidecars or189AE0 entity/audio/observer state. */
int uc_glass_model_create(const UCGlassModelFactory *,const char *resource,
                         const float matrix[16],const char *name,UCGlassRegistry **);
#ifdef __cplusplus
}
#endif
#endif

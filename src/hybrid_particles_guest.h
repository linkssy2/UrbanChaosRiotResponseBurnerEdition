#ifndef UC_HYBRID_PARTICLES_GUEST_H
#define UC_HYBRID_PARTICLES_GUEST_H
#include <stdint.h>
#include "../native/particle_billboard.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Owned snapshot at99DD0:9A22A, after the original class4 filler and3E9350
 * deferred state publication/reserve, before packet construction/expansion.
 * The reservation is uncommitted until original3E9370; native success skips
 * that commit. Nothing points into temporary guest stack memory.
 * This does not replace admission, simulation, sorting, texture or draw state. */
typedef struct UcHybridParticleBatch {
    uint32_t owner_va,source_va,source_texture_va,count,vertex_shader,pixel_shader;
    float clip_rows[4][4],up[4],right[4];
    UCParticleBillboard billboards[39];
    uint32_t last_raw[13]; /* Last original INLINE_ARRAY vertex, including unused word6. */
} UcHybridParticleBatch;
/* All guest CPU/FP state is preserved; snapshot reads never write guest RAM.
 * The host's ordered drain may consume its original GPU queues.
 * 0 falls through the original
 * emission. 1 means drawn. -1 means host failure after possible drawing: do
 * not replay; the host reports failure and disables future native attempts.
 * Any nonzero result requires original BL restoration then9A44D continuation. */
int uc_hybrid_particles_try_draw(uint32_t frame,uint32_t count);
void uc_hybrid_particles_read_counts(uint64_t *out,unsigned count);
/* Synchronous host boundary: drains prior guest commands, checks and binds
 * original parent state/resources, draws or rejects, restores parent state.
 * Rejection must happen before any draw; caller never retains the snapshot. */
int uc_hybrid_particles_host_draw(const UcHybridParticleBatch *);
#ifdef __cplusplus
}
#endif
#endif

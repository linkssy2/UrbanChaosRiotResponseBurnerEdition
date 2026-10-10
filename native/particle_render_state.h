#ifndef UC_PARTICLE_RENDER_STATE_H
#define UC_PARTICLE_RENDER_STATE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum UCParticleBlendFactor { UC_PARTICLE_ZERO, UC_PARTICLE_ONE,
    UC_PARTICLE_SOURCE_COLOR, UC_PARTICLE_INVERSE_SOURCE_COLOR,
    UC_PARTICLE_SOURCE_ALPHA, UC_PARTICLE_INVERSE_SOURCE_ALPHA };
enum UCParticleBlendOperation { UC_PARTICLE_ADD, UC_PARTICLE_REVERSE_SUBTRACT };
enum UCParticleCullWinding { UC_PARTICLE_CULL_NONE, UC_PARTICLE_CULL_CLOCKWISE,
    UC_PARTICLE_CULL_COUNTERCLOCKWISE };
typedef struct UCParticleRenderState {
    uint32_t blend_enabled,alpha_test,source_factor,destination_factor,operation;
    uint32_t cull_winding,alpha_comparison,alpha_reference,texture_alpha_kill,fog_enabled;
} UCParticleRenderState;
/* Complete999C0/99C40 effect-state projection, with the caller's existing state.
 * Comparison indices: never,less,equal,less/equal,greater,not-equal,greater/equal,
 * always. Reference is a byte. Texture alpha kill is BEFORE color modulation.
 * Authored modes0..6: opaque,add,alpha,null,alpha-add,subtract,alpha-subtract.
 * Null disables blend ONLY; unknown modes preserve blend/alpha configuration.
 * All setup paths disable culling, set comparison>= and reference1, enable
 * texture0 alpha kill. Cleanup disables fog/blend/alpha/texture kill, restores
 * ONE/ZERO/ADD and clockwise culling; it retains comparison/reference.
 * No depth/write-mask/sampler/fog-factor ownership, GPU calls, deferred caches
 * or command translation here. Boolean fields<=1, supported factors/operation
 * as above. Returns1 success,0 rejection without mutation. */
int uc_particle_render_begin(UCParticleRenderState *,uint32_t authored_mode);
int uc_particle_render_end(UCParticleRenderState *);
#ifdef __cplusplus
}
#endif
#endif

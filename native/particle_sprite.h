#ifndef UC_PARTICLE_SPRITE_H
#define UC_PARTICLE_SPRITE_H
#include "particle_draw.h"
typedef struct UCParticleSpriteState {
    UCParticleColorCurve curve;
    float speed, speed_delta, offset, offset_delta, lifetime_radians;
} UCParticleSpriteState;
typedef struct UCParticleSpriteAdmission {
    uint32_t count,blend;
    const void *texture;
} UCParticleSpriteAdmission;
typedef struct UCParticleSprite {float center[3],radius,angle,color[4];} UCParticleSprite;
/* 3A8650 + CRT rand, with explicit borrowed CRT seed (not the game/audio RNG).
 * Startup seed/call ordering is not supplied by this leaf. Stable disjoint
 * state/output; nearest/gradual arithmetic. Returns0 success,1 rejection. */
int uc_particle_sprite_random(uint32_t *crt_seed,float table[16]);
/* Complete23B570 preparation with borrowed resource/profile context. Blood flag
 * is the original resource owner's D+D8; filename classification is not here.
 * When blood is disallowed only admission.count changes. Otherwise texture,
 * blend and derived state publish even when thermal suppression makes count0.
 * No texture resolution, manager admission or GPU work. Inputs/outputs disjoint. */
int uc_particle_sprite_prepare(const UCParticleAsset *,unsigned blood,unsigned blood_enabled,
    unsigned thermal,uint32_t count,const void *texture,UCParticleSpriteState *,UCParticleSpriteAdmission *);
/* Complete23B6B0 records for an already admitted range. Table is the real
 * retained startup table. Near fade is supplied live configuration; transform
 * is original draw transform. It affects radius and ordinary alpha, but never
 * thermal alpha or camera-relative particles. No bounds culling/submission.
 * Finite bounded source values<=65536; no allocation. Returns1 on initial
 * rejection,2 nonfinite derived record (earlier outputs may remain),0 success. */
int uc_particle_sprite_draw(const UCParticleAsset *,const UCParticleSpriteState *,
    const UCParticle *,uint32_t count,const float matrix[16],const float table[16],
    unsigned near_fade,unsigned blood,unsigned thermal,UCParticleSprite *out);
#endif

#ifndef UC_PARTICLE_ASSET_H
#define UC_PARTICLE_ASSET_H
#include "particle_system.h"
#include <stddef.h>
/* Authored SimpleParticleSystem fields. No guest pointers or GPU objects. */
typedef struct UCParticleAsset {
    UCParticleDefinition simulation;
    uint32_t emitter_type,particle_type,blend,render_type;
    int32_t burst,depth_bias;
    float rate,emitter_lifetime;
    float color[3][4],hold[2],rotation[4],blur;
    float stretch_start,stretch_hold,stretch_end,stretch_hold_start,stretch_hold_end;
    float stretch_power,stretch_direction[2],line_length,thermal_color[4];
    uint8_t camera_relative,render_in_thermal,use_thermal_color;
    char texture[512];
} UCParticleAsset;
/* Explicit constructor defaults. Original omitted padding/unused members are
 * not replicated. Load accepts a bounded ASCII payload with the exact LE size
 * prefix, <=1MiB, names<=127 and values<=511. Output unchanged on rejection.
 * Numeric conversion uses the host C locale/CRT with bounded finite values,
 * not an original CRT or malformed-file equivalence claim. Strict nearest FP.
 * Unknown properties retain the original ignore behavior. Texture resolution
 * is deferred: the exact requested name is retained, not a successful lookup. */
void uc_particle_asset_reset(UCParticleAsset *);
int uc_particle_asset_load(const void *,size_t,UCParticleAsset *);
#endif

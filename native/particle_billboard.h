#ifndef UC_PARTICLE_BILLBOARD_H
#define UC_PARTICLE_BILLBOARD_H
#include <stdint.h>
typedef struct UCParticleBillboard {
    float center[3],radius,sine,cosine;
    uint32_t color;
} UCParticleBillboard;
typedef struct UCParticleBillboardView {
    float clip_rows[4][4],up[4],right[4],viewport_scale[3],viewport_offset[3];
} UCParticleBillboardView;
typedef struct UCParticleBillboardVertex {float clip[4],screen[3],color[4],uv[4],fog;} UCParticleBillboardVertex;
/* 99980/99960 conversion: nearest-even, then signed16/unsigned8 saturation.
 * Finite RGBA<=65536, disjoint output. Input is the original final draw color;
 * heat-haze blue/alpha transformation, if any, belongs to the outer consumer. */
int uc_particle_color_pack(const float rgba[4],uint32_t *argb);
/* Actual embedded17-instruction billboard program4EB158, for corner0..3.
 * Native fixed arithmetic; no guest instructions/addresses at runtime.
 * Original caller supplies retained view rows/bases, packed color and once-per-
 * particle rounded sine/cosine. Trig provenance, render admission, resources,
 * material/fog blending, GPU submission and scene ownership are not here.
 * Nearest/gradual arithmetic, finite disjoint inputs. Guard rejects zero clip W
 * or nonfinite intermediates without publishing output. Returns1 success,0 fail. */
int uc_particle_billboard_vertex(const UCParticleBillboard *,unsigned corner,
    const UCParticleBillboardView *,UCParticleBillboardVertex *);
/* Same finite homogeneous result before perspective division; screen is zero.
 * GPU submission uses this boundary so the GPU can clip camera-plane vertices,
 * including W=0. The CPU screen-coordinate oracle above retains its guard. */
int uc_particle_billboard_clip(const UCParticleBillboard *,unsigned corner,
    const UCParticleBillboardView *,UCParticleBillboardVertex *);
#endif

#ifndef UC_NATIVE_POINT_LIGHT_VERTEX_H
#define UC_NATIVE_POINT_LIGHT_VERTEX_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCPointLightVertexInput {
    float position[3];
    uint32_t packed_normal;
    uint8_t weights[2];
    const float (*skin_rows)[12]; /* NULL static; otherwise three resolved rows. */
} UCPointLightVertexInput;
typedef struct UCPointLightVertexConstants {
    float clip_rows[4][4], attenuation_rows[3][4], center[3];
    float viewport_scale[3], viewport_offset[3];
} UCPointLightVertexConstants;
typedef struct UCPointLightVertexOutput {
    float clip[4], screen[3], attenuation[3], direction[3], normal[3], color[4];
} UCPointLightVertexOutput;
/* Actual embedded basic diffuse programs4EE4B0/4EE6B0. Constants are explicit
 * already-resolved original receiver values, not invented world transforms.
 * Packed normals use signed11/11/10 fetch; skin uses all three weights, including
 * negative third weights. Normal and direction stay UNNORMALIZED: original
 * sampling supplies the normalization-cube behavior separately. No albedo,
 * receiver membership, light scheduler, texture sampling or blending is supplied.
 * Finite disjoint inputs and default nearest-even non-FTZ arithmetic required.
 * Zero clip W and nonfinite intermediate/output are additional native guards.
 * Returns1 on success,0 without output publication on invalid/unsupported input.
 * Output clip is before the source viewport operations; screen reproduces them.
 * No source GPU program, bytecode, guest memory or register state is executed. */
int uc_point_light_vertex(const UCPointLightVertexInput *,
    const UCPointLightVertexConstants *,UCPointLightVertexOutput *);
#ifdef __cplusplus
}
#endif
#endif

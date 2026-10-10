#ifndef UC_NATIVE_POINT_LIGHT_TEXTURES_H
#define UC_NATIVE_POINT_LIGHT_TEXTURES_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum { UC_LIGHT_CUBE_SIDE=128,UC_LIGHT_CUBE_WORDS=6*128*128,
       UC_LIGHT_VOLUME_SIDE=32,UC_LIGHT_VOLUME_BYTES=32*32*32 };
/* Exact A3B10/ACED0 and A4540 linear texels, before Xbox swizzle/storage.
 * Cube order+X,-X,+Y,-Y,+Z,-Z; packed words have signed B/G/R bytes and a
 * separate unsigned alpha from the original normalized-Y expression. Do not
 * reinterpret all four channels as SNORM. Volume is one unsigned byte/texel,
 * X-fastest thenY thenZ, with the authored zero boundary and falloff.
 * Caller owns contiguous storage; extra capacity remains untouched. Returns1
 * success,0 without publication for NULL/short buffers or nondefault rounding.
 * No allocation, GPU texture, sampler, renderer registration or swizzle owner
 * is supplied. Double-backed recovered x87, not physical80-bit equivalence. */
int uc_point_light_normalization(uint32_t *cube,size_t words);
int uc_point_light_attenuation(uint8_t *volume,size_t bytes);
#ifdef __cplusplus
}
#endif
#endif

#ifndef D3D8_MATERIAL_RECIPE_H
#define D3D8_MATERIAL_RECIPE_H
#include <stdint.h>
/* Value-only preparation. No guest pointers, COM objects, mutable texture
 * contents or claims about what is currently bound on the D3D11 context. */
typedef struct D3D8MaterialSampler {
    uint32_t u,v,w,min_filter,mag_filter,lod_bits,mip_filter;
} D3D8MaterialSampler;
typedef struct D3D8MaterialRecipe {
    D3D8MaterialSampler sampler[4];
    uint32_t unsupported; /* three diagnostic bits per stage: address/filter/LOD */
} D3D8MaterialRecipe;
#endif

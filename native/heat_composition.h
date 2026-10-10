#ifndef UC_HEAT_COMPOSITION_H
#define UC_HEAT_COMPOSITION_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCHeatVertex {float position[3],uv[2];} UCHeatVertex;
typedef struct UCHeatComposition {
    UCHeatVertex scene[4],mask[4],strips[19][30];
} UCHeatComposition;
/*9F730/A03B0, actual20x15 grid and +5 pixel sample offsets. View is the
 * original +8,+4,+24,+28 rectangle. Scratch dimensions are A5320's display/2.
 * The mask pass is alpha-only into scratch B; final strips sample B with
 * SRCALPHA/INVSRCALPHA and RGB writes. No framebuffer ownership here. */
int uc_heat_composition(uint32_t width,uint32_t height,const float view[4],
    const float grid[300][2],UCHeatComposition *);
#ifdef __cplusplus
}
#endif
#endif

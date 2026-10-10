#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/*2BB180(2,2,0) owner driven by2A9860/2BAEE0. Velocity acceleration is
 * clamped per update, as authored, rather than multiplied by delta. */
typedef struct UCBruteMovementBlend {float current,velocity;} UCBruteMovementBlend;
int uc_brute_movement_blend(UCBruteMovementBlend *,int32_t mode,float delta);
/*2B4AF0 specialized to2A9310's three non-null RunBatUp/Run/Run children. */
int uc_brute_movement_slider(float value,uint32_t *first,float *weight);
#ifdef __cplusplus
}
#endif

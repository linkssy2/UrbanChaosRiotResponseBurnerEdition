#ifndef UC_COP_OVERLAY_MATH_H
#define UC_COP_OVERLAY_MATH_H
#ifdef __cplusplus
extern "C" {
#endif
double uc_cop_overlay_facing(const float self[3],const float forward[3],const float target[3]);
int uc_cop_overlay_player_in_line(const float muzzle[3],const float target[3],const float player[3]);
#ifdef __cplusplus
}
#endif
#endif

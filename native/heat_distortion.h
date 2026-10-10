#ifndef UC_HEAT_DISTORTION_H
#define UC_HEAT_DISTORTION_H
#ifdef __cplusplus
extern "C" {
#endif
/* Original9F200/9F250. Table is512 floats; grid is15 rows of20 float2s.
 * Host double sin is the existing x87 oracle abstraction, not physical FSIN
 * equivalence. Caller owns one table and retained phase/grid for the effect.
 * Finite phase/previous, abs(phase)<=100000, finite table within[-1,1],
 * nearest/gradual FP and disjoint storage required. Returns1 on rejection,
 * 0 on success (unchanged phase leaves grid untouched). No framebuffer work. */
int uc_heat_distortion_table(float table[512]);
int uc_heat_distortion_grid(float *previous,float phase,const float table[512],float grid[300][2]);
#ifdef __cplusplus
}
#endif
#endif

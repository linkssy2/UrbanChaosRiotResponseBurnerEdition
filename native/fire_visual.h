#ifndef UC_FIRE_VISUAL_H
#define UC_FIRE_VISUAL_H
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCFireVisual {
    float matrix[16],size[3],rate[6];
} UCFireVisual;
/* Visual projection of190440's dimensions/placement and18FCA0/18F250/18F6A0.
 * Inputs are actual authored dimensions, pre-constructor position and the six
 * resolved resource rates. Admits only the infinite lifetime(-1) visual branch;
 * its18F150 trigger is the existing continuous trigger with adjusted rate.
 * No damage zone, child activation, audio, routing obstacle, entity registration
 * or extinguishing is completed here. Finite nonnegative bounded inputs,
 * nearest/gradual FP, disjoint output.0 success,1 rejection preserving output. */
int uc_fire_visual(const float dimensions[3],const float position[3],
    const float source_rates[6],float lifetime,UCFireVisual *);
#ifdef __cplusplus
}
#endif
#endif

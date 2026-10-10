#ifndef UC_NATIVE_THROWABLE_H
#define UC_NATIVE_THROWABLE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCMolotovLaunchInput {
    float origin[3],forward[3],target[3];
    float target_distance,pitch,charge;
    uint8_t shield_selected;
} UCMolotovLaunchInput;
typedef struct UCMolotovLaunch {float transform[16],velocity[3];} UCMolotovLaunch;
/* Full C6B70 item11 arithmetic and 5E960 frame, with supplied live tag6 origin,
 * tag0 forward and current aim result. Centimetres; render Y is up. Finite,
 * nonvertical/nondegenerate direction, charge[0,.5], standard FP mode. The
 * original x87 transcendental boundary uses host double/libm. Pure admission
 * precedes effects; 0 completed,1 unsupported, output unchanged on failure.
 * Does not create entities, debit inventory or claim Havok projectile physics. */
int uc_molotov_launch(const UCMolotovLaunchInput*,UCMolotovLaunch*);
#ifdef __cplusplus
}
#endif
#endif

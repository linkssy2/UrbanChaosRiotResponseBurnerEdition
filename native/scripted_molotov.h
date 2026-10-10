#ifndef UC_SCRIPTED_MOLOTOV_H
#define UC_SCRIPTED_MOLOTOV_H
#ifdef __cplusplus
extern "C" {
#endif
/* F6550 type0 launch, after a real target was resolved. The shared controller
 * collision delay is clamped in place before the ordered217/221 dispatches. */
typedef struct UCScriptedMolotov {float fuse,velocity[3],delay;} UCScriptedMolotov;
int uc_scripted_molotov(const float origin[3],const float target[3],float time,float *delay,UCScriptedMolotov *);
#ifdef __cplusplus
}
#endif
#endif

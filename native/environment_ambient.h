#ifndef UC_ENVIRONMENT_AMBIENT_H
#define UC_ENVIRONMENT_AMBIENT_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* One class450F40 world ambient owner after constructor offset zero and
 * world55800's actual manager-color load/reset. Native fields represent
 * original +4,+10,+1C,+28,+34; no second manager or current-color authority. */
typedef struct UCEnvironmentAmbient {float base[3],current[3],offset[3],target[3];uint32_t pending;} UCEnvironmentAmbient;
typedef enum UCEnvironmentAmbientSlot {UC_AMBIENT_INITIAL,UC_AMBIENT_TARGET,UC_AMBIENT_BASE,UC_AMBIENT_OFFSET} UCEnvironmentAmbientSlot;
/* Vectors finite in [-4096,4096]. Disjoint inputs except identical owned
 * vector alias allowed. Guards preserve state. Initial copies current+target
 * and clears pending; target sets pending even if equal. Base does not reset. */
int uc_environment_ambient_init(UCEnvironmentAmbient *,const float base[3]);
int uc_environment_ambient_set(UCEnvironmentAmbient *,UCEnvironmentAmbientSlot,const float rgb[3]);
int uc_environment_ambient_reset(UCEnvironmentAmbient *);
typedef struct UCEnvironmentAmbientHooks {
 void *(*manager)(void *); /* current graphics+30; captured before smoothing */
 float (*delta)(void *); /* actual world+14; read ONLY if pending */
 uint32_t (*world_mode)(void *); /* LOW BYTE, read ONLY with captured manager */
 void (*publish)(void *,void *manager,uint32_t argb);
} UCEnvironmentAmbientHooks;
/* Full57BE0 order. Completing hooks cannot mutate this owner; manager stays
 * valid until return. If pending, delta must be finite >=0. pow uses host libm
 * with exact original double0.10000000149011612 then float32 rounding; original
 * CRT exceptions/extended-precision parity are not claimed. All remaining
 * arithmetic is float32, packed RGB uses truncate/clamp, alpha255. Pending
 * clears only on all-component exact equality. Source world-mode selects
 * .18000000715255737 gray regardless of owner. Missing manager still smooths.
 * Static guards precede callbacks. Invalid dynamic delta rejects AFTER manager
 * and delta reads, without owner edits/publication; no callback rollback.
 * No allocation, entity scheduling, volume selection or rendering claim. */
int uc_environment_ambient_update(UCEnvironmentAmbient *,const UCEnvironmentAmbientHooks *,void *context);
#ifdef __cplusplus
}
#endif
#endif

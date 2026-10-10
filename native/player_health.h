#ifndef UC_NATIVE_PLAYER_HEALTH_H
#define UC_NATIVE_PLAYER_HEALTH_H
#ifdef __cplusplus
extern "C" {
#endif
/* Original2180A0 setter, finite nonnegative domain. Both borrowed float slots
 * must be valid, disjoint and belong to the same actor. Current health is the
 * existing inventory.health; maximum is the actor's one+A0 scalar, initialized
 * to0 by the fresh218550 constructor. This is not a second current-health owner
 * or a full actor constructor/reset. Invalid input leaves both slots untouched. */
int uc_player_health_set(float *current,float *maximum,float value);
#ifdef __cplusplus
}
#endif
#endif

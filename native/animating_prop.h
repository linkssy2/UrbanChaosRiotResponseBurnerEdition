#ifndef UC_ANIMATING_PROP_H
#define UC_ANIMATING_PROP_H
#include "animation_controller.h"
#ifdef __cplusplus
extern "C" {
#endif
/* 1FB630 after successful native allocation, table47EF60 Idle row8: repeat0,
 * blend argument0, action8. List/cache/model ownership remains caller-owned. */
UCAnimationStatus uc_animating_prop_controller(const UCAnimationClip *,UCAnimationController *);
/* Whole1EB760's quiet Idle-only projection: current=desired8, no on conditions,
 * no interrupt/death/hidden flags, no action audio restart, all flag outputs-1.
 * done is actual current controller's pure virtual2C query inverted (or absent).
 * Caller retains the completed controller until replacement. finished aliases
 * entity+1FC; apply completes actual1FB730/1FB630 and sees finished nonzero before
 * replacement; successful return publishes finished0. LoopIdle0 still records
 * completion. No different action, damage, script or sound completion claimed.
 *0 success,1 invalid before effects,2 committed apply failure. */
int uc_animating_prop_idle(uint8_t done,uint8_t loop,uint8_t *finished,
    int (*apply)(void *),void *owner);
#ifdef __cplusplus
}
#endif
#endif

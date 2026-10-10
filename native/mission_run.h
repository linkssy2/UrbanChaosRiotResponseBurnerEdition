#ifndef UC_MISSION_RUN_H
#define UC_MISSION_RUN_H
#include "mission_event.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Full2176C0, borrowing the same event slots/flagCD. decode completes the actual
 * installed8C handler, which may itself suspend through the same continuation.
 * yield suspends then returns after resumption; it may not pretend to suspend.
 * Both callbacks return0 on success; any failure stops with status2 after the
 * already committed prefix. No reentry, owner release, slot rebinding or aliasing
 * of descriptors/output with actor slots. The actual current-level identity and
 * actor source-index50 remain stable through callbacks (not just the supplied
 * descriptors); level metadata is immutable and valid. Actual
 * mutable script_index/cursor/flags are reread at the source's restart points.
 * Return0 completes with original signed result;1 rejects before effects;
 *2 is committed failure, not safe to retry. No artificial iteration budget or
 * skipped command. Concrete command decode and scheduling are required. */
int uc_mission_run(UCMissionEvent *,uint8_t *condition_started,
    const UCLevel *,uint32_t entity,void *owner,
    int (*decode)(void *,int32_t *),int (*yield)(void *),int32_t *result);
/* Same2176C0 body with the actual custom/entity descriptor borrowed directly. */
int uc_mission_run_descriptor(UCMissionEvent *,uint8_t *condition_started,
    const UCLevelEntity *,void *,int (*decode)(void *,int32_t *),
    int (*yield)(void *),int32_t *result);
#ifdef __cplusplus
}
#endif
#endif

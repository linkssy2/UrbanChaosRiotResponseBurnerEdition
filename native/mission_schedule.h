#ifndef UC_MISSION_SCHEDULE_H
#define UC_MISSION_SCHEDULE_H
#include "mission_conditions.h"
#include "mission_run.h"
#ifdef __cplusplus
extern "C" {
#endif
/* SAME actor event slots, CD/CE and level-owned conditions/flags/CRT stream.
 * Metadata and slot addresses remain stable; completing decode/destroy/yield
 * callbacks may change their values. yield must suspend the calling native
 * continuation. No second cursor, wait timer, condition-used array or budget.
 * Decode must execute the actual installed handler, not skip unsupported code.
 * Default2177C0/217830 and ordinary20A140 are the admitted virtual methods. */
typedef struct UCMissionSchedule {
    UCMissionEvent *event;
    uint8_t *condition_started,*conditions_enabled;
    const UCLevelEntity *definition;
    UCMissionCondition *conditions;
    uint32_t condition_count;
    uint8_t *flags;
    size_t flag_count;
    uint32_t *crt_seed;
    void *owner;
    void (*destroy)(void *,void *);
    int (*decode)(void *,int32_t *);
    int (*yield)(void *);
} UCMissionSchedule;
/* Whole217A80. Valid nonempty condition groups are required if bit4 is set;
 * out-of-range selected script is an actual NULL lookup/cancel, not rejection.
 * Returns0 completed,1 invalid before effects,2 committed callback/admission
 * failure. Callbacks retain all borrowed owners, and destroy really disposes
 * the captured command. Source-reachable unsupported states are never skipped. */
int uc_mission_condition_run(const UCMissionSchedule *);
/* Whole218AE0. Intentionally does not normally return: the owning coroutine
 * suspends at source yield points and is closed by its real lifetime owner.
 * Returns1 invalid before effects or2 committed failure; no silent stop. */
int uc_mission_schedule_run(const UCMissionSchedule *);
#ifdef __cplusplus
}
#endif
#endif

#ifndef UC_MISSION_CONDITIONS_H
#define UC_MISSION_CONDITIONS_H
#include "mission_event.h"
#ifdef __cplusplus
extern "C" {
#endif
/* One mutable level-owned conditional record. Rows/tests borrow the same
 * immutable ENT descriptor. used is original record+14, initialized to zero;
 * it belongs to the level record, not a second copy per polling entity. */
typedef struct UCMissionCondition {
    UCMissionGroup scripts;
    const uint8_t *tests;
    uint32_t test_count,repeat;
    uint8_t used;
} UCMissionCondition;
/* Decode the already loaded entity's first script collection. Output capacity
 * is caller-owned; outputs are scratch until return0. No allocation. Returns1
 * for invalid/truncated data/capacity. count is only published on success. */
int uc_mission_conditions_load(const UCLevelEntity *,UCMissionCondition *,
    uint32_t capacity,uint32_t *count);
/* Whole217840 with actual default2177C0/217830 dispatch used by ScriptController.
 * Same entity event fields and borrowed flagCD, level mission flag bytes and
 * mutable condition records. Metadata/pointers remain stable; a completing
 * command destructor may mutate event fields/flag values but not these owners,
 * condition metadata/used bytes, condition_started pointer, or callback binding.
 * No reentry. All represented slots disjoint; default handlers overwrite their
 * fields after destruction exactly. No overridden class handler is implied.
 * Returns0 completed,1 invalid before effects. Does not execute the selected
 * script/coroutine or claim a complete mission. */
int uc_mission_conditions_update(UCMissionEvent *,uint8_t *condition_started,
    UCMissionCondition *,uint32_t count,uint8_t *mission_flags,size_t flag_count,
    void (*destroy_command)(void *,void *),void *owner);
#ifdef __cplusplus
}
#endif
#endif

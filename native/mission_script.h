#ifndef UC_NATIVE_MISSION_SCRIPT_H
#define UC_NATIVE_MISSION_SCRIPT_H
#include "level_asset.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum UCMissionStatus {
    UC_MISSION_READY, UC_MISSION_YIELD, UC_MISSION_DONE,
    UC_MISSION_NEEDS_ENTITY, UC_MISSION_UNSUPPORTED, UC_MISSION_FORMAT,
    UC_MISSION_ARGUMENT, UC_MISSION_NOT_FOUND, UC_MISSION_BINDING_FAILED
} UCMissionStatus;
typedef struct UCMissionScript {
    const UCLevel *level; /* Must remain alive and immutable. */
    uint32_t entity, script, offset, opcode, argument;
    float wait_remaining;
    uint8_t waiting, done;
} UCMissionScript;
/* Native class factory/registry binding for original210CF0: ensure this authored
 * entity exists, returning1 only after native construction succeeds,0 if the
 * class is unsupported,-1 on failure. Existing entities must not be recreated.
 * NULL means unimplemented; no request is consumed or reported successful. */
typedef int (*UCMissionEnsureEntity)(void *context, uint32_t entity_index);

/* Borrowed source event-group rows; lifetime is the same immutable level.
 * Kept separate from selection because2175A0 checks priority BEFORE drawing
 * random state, and a single-candidate group consumes no random value. */
typedef struct UCMissionGroup {
    const uint8_t *rows;
    uint32_t count;
    int32_t priority;
} UCMissionGroup;
UCMissionStatus uc_mission_group(const UCLevelEntity *,uint32_t,UCMissionGroup *);

/* Base engine script commands216FA0/217BB0:5 switch-script,6/21 wait,
 * 10/11 set/clear flag,22 ensure entity,FFFFFFFF end. Other commands stop at
 * their original byte offset, never skipped. Flags use ENT.field bytes, matching
 * original210350, supplied/owned by the native mission. One call resumes one
 * original coroutine scheduling segment; YIELD is not an assertion of one
 * rendered frame. Timed waits include the original extra command-end yield.
 * This is an engine script interpreter, not a CPU/graphics compatibility VM. */
UCMissionStatus uc_mission_script_begin(const UCLevel *level, uint32_t entity,
    uint32_t script, UCMissionScript *out);
UCMissionStatus uc_mission_script_resume(UCMissionScript *script, float delta_seconds,
    uint8_t *flags, size_t flag_count, UCMissionEnsureEntity ensure_entity, void *context);
/* Original event-group selection216F20: first candidate fallback; with multiple
 * choices select first whose low<random_percent<high (strict endpoints).
 * Group metadata is preserved by level_asset as the third220A30 collection.
 * Returns its signed priority for caller's original2175A0 priority gate. */
UCMissionStatus uc_mission_select_group(const UCLevelEntity *entity, uint32_t group,
    uint32_t random_percent, uint32_t *script, int32_t *priority);
const char *uc_mission_status(UCMissionStatus status);
#ifdef __cplusplus
}
#endif
#endif

#ifndef UC_MISSION_TRIGGER_ACTOR_H
#define UC_MISSION_TRIGGER_ACTOR_H
#include "entity_fresh.h"
#include "entity_names.h"
#include "level_entity_slots.h"
#include "mission_conditions.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCMissionTriggerRow { uint32_t type,group; } UCMissionTriggerRow;
/* Embed once beside the SAME FreshView actor. D0/D4/D8/DC are this class's
 * actual startup latch, transition state and owned property rows. */
typedef struct UCMissionTriggerActor {
    uint8_t started;
    int32_t state,count;
    UCMissionTriggerRow *rows;
} UCMissionTriggerActor;
typedef struct UCMissionTriggerBody {
    const float *position,*distance88; /* same actor40 and squared-radius88 */
} UCMissionTriggerBody;
typedef struct UCMissionTriggerWorld {
    const UCEntityNames *names;
    const UCEntityEventRegistry *registry;
    const UCLevelEntitySlots *slots;
    void *owner;
    /* Pure projection of actual default2187A0 actor fields. No substitute
     * shape/position; NULL rejects an unsupported class virtual+48 override. */
    int (*body)(void *,const UCEntityEventTarget *,UCMissionTriggerBody *);
} UCMissionTriggerWorld;
/* 211450 -> default2187A0 shape3 ->2D3830. Actual loaded boxes retain two
 * endpoints per24 bytes; normalize them with2D38F0 ordering. Strict threshold
 * comparison. Finite values <=1e6, threshold[0,1e12], RN/noFTZ/DAZ. Other
 * shapes reject; all67 Westside custom triggers are shape3. PLAYER name IDs,
 * subject slots and generation registry are borrowed from their real owners.
 * Stable graph during query; resolver is pure and returns0 on completion.
 * Missing named entity rejects (original dereferences it); missing subject
 * entity follows the original false/continue path. No activation is performed.
 * Return0 complete,1 invalid/unsupported; output unchanged on error. */
int uc_mission_trigger_overlap(const UCLevelTrigger *,const UCMissionTriggerBody *,uint8_t *);
int uc_mission_trigger_query(const UCLevelTrigger *,const UCMissionTriggerWorld *,uint8_t *);
/* Whole2112D0 class suffix with required actual218A60 construction. Context
 * and metadata identities, row count/storage and condition metadata stay stable
 * through callbacks; allocation succeeds in the original comparison domain.
 * property_cursor aliases metadata's current cursor; conditions alias the
 * level-owned used bytes. group_count is the loaded actual collection count.
 * Construct base may mutate live FreshView fields but must complete its real
 * construction. NULL trigger represents original NULL source context.
 * Return0 complete,1 pre-effect rejection,2 committed dependency failure. */
int uc_mission_trigger_construct(UCMissionTriggerActor *,const UCEntityFreshView *,
    UCLevelTrigger *,int32_t source_index,uint32_t group_count,
    const uint8_t **property_cursor,UCMissionCondition *,uint32_t condition_count,
    const UCEntityFreshMemory *,void *owner,int (*construct_base)(void *));
/* Whole211610. Captures initial state/enable gate; rows/count and event fields
 * are live, but callbacks must preserve row storage/count and borrowed owners.
 * Source2175A0 effects are executed on the SAME event via descriptor helper.
 * Required suffix is actual218C40 scheduling, called even when disabled.
 * It may mutate actor/event state and must preserve lifetime until return.
 * No generic211B80 timer/flag trigger or TriggerRegion class is implied. */
int uc_mission_trigger_update(UCMissionTriggerActor *,UCLevelTrigger *,
    const UCMissionTriggerWorld *,UCMissionEvent *,uint32_t *crt_seed,
    void (*destroy_command)(void *,void *),void *owner,int (*update_actor)(void *));
/* Actual2102D0: release rows first, then required218B30 deactivation. Source
 * leaves its row pointer dangling; never read it again after completion. */
int uc_mission_trigger_deactivate(UCMissionTriggerActor *,const UCEntityFreshMemory *,
    void *owner,int (*deactivate_actor)(void *));
#ifdef __cplusplus
}
#endif
#endif

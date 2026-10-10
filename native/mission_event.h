#ifndef UC_MISSION_EVENT_H
#define UC_MISSION_EVENT_H
#include "mission_script.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Borrowed actual script-entity fieldsB0..CC and SAME entity5C/60 flags.
 * This view owns no script state. Cursor points into the immutable level;
 * command is owned by its concrete implementation. All slots remain live. */
typedef struct UCMissionEvent {
    uint32_t *flags,*secondary_flags;
    int32_t *priority,*condition_index;
    uint32_t *script_index;
    const uint8_t **cursor;
    void **command;
    uint8_t *new_script;
} UCMissionEvent;
/* Pure slot validation and binding to the existing FreshActor owner. Binding
 * publishes a borrowed view only; it does not initialize B8 or any actor field. */
struct UCEntityFreshView;
int uc_mission_event_valid(const UCMissionEvent *);
int uc_mission_event_bind(const struct UCEntityFreshView *,UCMissionEvent *);
/* Full2175A0 plus210F30/210F70/216F20 and ordinary20A140(40000000).
 * grouped uses its low byte exactly. UINT32_MAX request is an ignored event.
 * CRT random seed is borrowed from the session's actual shared owner; this is
 * not the game's separate250-word RNG. Only accepted multi-candidate groups
 * consume it. Missing direct scripts publishNULL without cancelling commands;
 * a selected invalid script still retains new priority/RNG, as source does.
 * Nonempty valid group metadata is required; original NULL-group dereference
 * is rejected before mutation. Native command destroy must actually complete;
 * it receives the OLD current command after cursor/flags/index publication,
 * may mutate live fields, and must retain this event/level/flag storage. No
 * reentry. Final commandNULL/new_script1 stores follow destruction. No fake
 * destructor, coroutine scheduler, typed command factory or mission completion.
 * Returns0 completed,1 invalid before effects; accepted is published on0 only.
 * All descriptors/output/seed/flags are disjoint with stable exclusive owners. */
int uc_mission_event_start(UCMissionEvent *,const UCLevel *,uint32_t entity,
    uint32_t request,uint8_t grouped,uint32_t *crt_seed,
    void (*destroy)(void *,void *),void *owner,uint8_t *accepted);
/* Same source lookup frontier for actual custom-trigger metadata. No copied
 * UCLevel/entity/actor state. NULL definition retains direct missing-script
 * behavior; grouped requests require a real nonempty descriptor group. */
int uc_mission_event_start_descriptor(UCMissionEvent *,const UCLevelEntity *,
    uint32_t request,uint8_t grouped,uint32_t *crt_seed,
    void (*destroy)(void *,void *),void *owner,uint8_t *accepted);
/* Full217670 cancellation. Mask flags BEFORE destroying the current command;
 * then clear command/new_script and reset priority/condition index to-1. Cursor/index
 * survive. Same lifetime/callback contract as start. */
int uc_mission_event_cancel(UCMissionEvent *,void (*destroy)(void *,void *),void *);
#ifdef __cplusplus
}
#endif
#endif

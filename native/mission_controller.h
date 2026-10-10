#ifndef UC_MISSION_CONTROLLER_H
#define UC_MISSION_CONTROLLER_H
#include "mission_event.h"
#ifdef __cplusplus
extern "C" {
#endif
/* The ScriptController120D50 view borrows its SAME event fields, model74,
 * groupD8 and session CRT seed. No copied actor/script authority. Model destroy
 * and release are separate actual lifetime operations, in that source order;
 * callbacks may change live event/group/model fields but must retain this view,
 * immutable level, flag slots and the captured model until its release. They
 * must complete without reentry. Destroy-command has mission_event's contract.
 * No callbacks are required for an absent model/command. All slots disjoint.
 * Return0 completed,1 invalid before effects,2 committed dependent failure.
 * Failure2 retains publications and must not be retried as a fresh update.
 * Concrete construction, coroutine execution and global scheduling are separate. */
typedef struct UCMissionController {
    UCMissionEvent *event;
    const UCLevel *level;
    uint32_t entity,*crt_seed,*group;
    void **model,*owner;
    void (*destroy_model)(void *,void *);
    void (*release_model)(void *,void *);
    void (*destroy_command)(void *,void *);
} UCMissionController;
int uc_mission_controller_update(const UCMissionController *);
int uc_mission_controller_update_descriptor(const UCMissionController *,const UCLevelEntity *);
#ifdef __cplusplus
}
#endif
#endif

#ifndef UC_MISSION_COMMAND_H
#define UC_MISSION_COMMAND_H
#include "mission_event.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Semantic scalar commands allocated by216FA0, dispatched by217BB0 and deleted
 * byEC3E0/217BA0. Native layout has no original vtable/guest pointer. C0 owns this
 * allocation; it must be released through destroy before closing a suspended
 * continuation. event start/cancel can use destroy directly. */
typedef struct UCMissionCommand { uint32_t opcode,argument; uint8_t value; } UCMissionCommand;
void uc_mission_command_destroy(void *unused,void *command);
/* Same live actor slots, current-level/source identity, world+14 delta and level
 * flag-byte owner. All remain valid and unaliased across callbacks. Only actual
 * default8C/90 dispatch and NULL BC are admitted. No shadow cursor/wait state.
 * level/script storage is immutable; cursor must be NULL or inside one of this
 * entity's scripts. ensure must finish real210CF0 effects (0 success); NULL is
 * allowed only when this command does not need it. yield really suspends and
 * resumes the current actorC8 continuation (0 success), never a no-op.
 * During yield, real event start/cancel may destroy/replace C0 and change other
 * live actor fields. Any replacement C0 is owned by this module. The local wait
 * survives cancellation; completion reloads C0 before deleting it. Callback may
 * not reenter decode or release/rebind these owners. Finite nonnegative delta,
 * finite duration and nearest rounding are the admitted arithmetic domain.
 * Return0 publishes original -1/-2;1 rejects before effects;2 is a committed
 * allocation/callback/arithmetic failure, not safe to retry. Unsupported opcode
 * or missing endpoint does not advance cursor. No general command/class claim. */
typedef struct UCMissionCommandContext {
    UCMissionEvent *event;
    void *const *wait_handle;
    const UCLevel *level;
    uint32_t entity;
    uint8_t *flags;
    size_t flag_count;
    const float *delta;
    void *owner;
    int (*yield)(void *);
    int (*ensure)(void *,uint32_t);
    /* Command61 retains the ID returned by210CF0 and waits for1F8FC0 to
     * stop resolving THAT generation, not merely an inactive descriptor. */
    int (*ensure_id)(void *,uint32_t,int32_t *);
    int (*alive)(void *,int32_t,uint8_t *);
    UCLevelLogic *logic; /* Commands52/53: SAME mutable trigger flags. */
    /*17/23/43: lookup210CA0; apply to live entity or queue208300 when absent.
     * mode0 removes,1 sets10000000,2 clears10000000. */
    int (*entity_action)(void *,uint32_t source,uint32_t mode);
    int (*music_event)(void *,const char *); /*57: optional live Sound_MusicManager.*/
    int (*operate)(void *,uint32_t target_source); /*16:217E0A, same sender.*/
} UCMissionCommandContext;
int uc_mission_command_decode(const UCMissionCommandContext *,int32_t *result);
/* Actual custom-trigger/actor metadata lookup, sharing the identical decoder.
 * flag_limit is the current level field; flag_count is readable storage size. */
int uc_mission_command_decode_descriptor(const UCMissionCommandContext *,
    const UCLevelEntity *,uint32_t flag_limit,int32_t *result);
#ifdef __cplusplus
}
#endif
#endif

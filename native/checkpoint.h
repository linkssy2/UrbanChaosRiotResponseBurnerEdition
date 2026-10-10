#ifndef UC_NATIVE_CHECKPOINT_H
#define UC_NATIVE_CHECKPOINT_H
#include "level_asset.h"
#include "mainplayer_state.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum UCCheckpointPhase {
    UC_CHECKPOINT_OPERATED, UC_CHECKPOINT_WAIT_OPERATED, UC_CHECKPOINT_RESTART,
    UC_CHECKPOINT_WAIT_RESTART, UC_CHECKPOINT_DELAY, UC_CHECKPOINT_POST,
    UC_CHECKPOINT_PARK
} UCCheckpointPhase;
typedef enum UCCheckpointStatus {
    UC_CHECKPOINT_READY, UC_CHECKPOINT_YIELD, UC_CHECKPOINT_POST_STARTED,
    UC_CHECKPOINT_PARKED, UC_CHECKPOINT_NEEDS_EVENT, UC_CHECKPOINT_NEEDS_POLL,
    UC_CHECKPOINT_FAILED, UC_CHECKPOINT_ARGUMENT, UC_CHECKPOINT_NOT_FOUND
} UCCheckpointStatus;
typedef struct UCCheckpoint {
    UCLevelRestart restart;
    UCCheckpointPhase phase;
    float remaining_delay;
    uint8_t restart_ready, reset_complete, initialized;
} UCCheckpoint;
/* Native2175A0 binding:1 started,0 ignored by the original event/priority
 * policy,-1 unsupported,-2 failed. No placeholder activation counts as1.
 * The native host owns event-group RNG, priority/cancellation and script tasks.
 * Poll matches the owner's CURRENT script busy bit, not an old task ticket:
 * 0 idle,1 busy,-1 unknown/unsupported,-2 failed. */
typedef int (*UCCheckpointStartEvent)(void *, uint32_t entity, uint32_t group);
typedef int (*UCCheckpointPollScript)(void *, uint32_t entity);
UCCheckpointStatus uc_checkpoint_begin(const UCLevel *level, int32_t number, UCCheckpoint *out);
/* One original136D10 coroutine segment. The host separately resumes native
 * scripts and entity tasks; it must supply actual game delta and ordering.
 * Waits OnOperated, then OnRestart, then0.25s, sets both ready flags, launches
 * OnPostRestart, and parks without declaring the post script complete.
 * Missing bindings stop at the current phase and may be retried. */
UCCheckpointStatus uc_checkpoint_resume(UCCheckpoint *checkpoint, float delta_seconds,
    UCCheckpointStartEvent start_event, UCCheckpointPollScript poll_script, void *context);

typedef struct UCCheckpointItem {
    uint32_t item;uint16_t clip,reserve;uint8_t seen,padding[3];
} UCCheckpointItem;
/* Actual pointer-free4C150 save record. Unwritten padding and unused rows are
 * preserved, not zeroed. No disk write, player factory or restore is implied. */
typedef struct UCCheckpointSnapshot {
    int32_t number;float health;uint8_t shield,flag9,padding[2];uint32_t count;
    UCCheckpointItem items[27];int32_t category[3],selected;
    uint32_t extra22,mission[2],statistics[18];
} UCCheckpointSnapshot;
typedef struct UCCheckpointSnapshotInput {
    const UCMainplayerState *inventory;
    const UCMainplayerItemSpec *specs; /*27 actual resolved definitions*/
    const uint32_t *item_extra; /*27 actual item+18 values*/
    const uint8_t *flag9; /*actual player4C5*/
    const int32_t *number;const uint32_t *mission,*statistics; /*2,18 words*/
} UCCheckpointSnapshotInput;
/* Stable disjoint borrowed owners, valid IDs and nonnegative bounded ammo.
 * Source ordering/duplicates retained. Type25 is omitted; eligible item0 is
 * also saved separately in shield. Category/active choices need not be owned.
 * Return0 completed,1 invalid (output unchanged). */
int uc_checkpoint_snapshot(const UCCheckpointSnapshotInput *,UCCheckpointSnapshot *);
/* CC2F0 CCA6C..CCB91 scalar saved-inventory branch, before C8930 presentation.
 * Replaces inventory/ammo, retaining unseen-item history as 429A0 does. Health
 * is at least the retail 200.0 constant. Caller publishes health maximum,
 * flashlight, mission/statistics, equipment resources and checkpoint transform.
 *0 complete,1 invalid without mutation. specs has27 resolved definitions. */
int uc_checkpoint_restore(const UCCheckpointSnapshot *,const UCMainplayerItemSpec *,
    UCMainplayerState *,uint32_t item_extra[27]);
/*136CC0 resumes the primary THEN the shared actor script on every pass.
 * Selected startup may consume several passes in one world update. Borrowed
 * fields are re-read after both callbacks; they must remain alive. Callbacks
 * must complete and ensure progress, as the source has no iteration cutoff.
 *0 completed,1 invalid before effects,2 committed callback failure. */
typedef struct UCCheckpointPump {
    const uint8_t *ready,*reset;
    const int32_t *number;
    void *owner;
    int (*primary)(void *),(*script)(void *);
    int (*current)(void *,int32_t *);
} UCCheckpointPump;
int uc_checkpoint_pump(const UCCheckpointPump *);
typedef struct UCCheckpointOperation {
    uint8_t *pending,*parked,*save_pending;
    const int32_t *number;
    const uint32_t *group;
    uint32_t entity;
    void *owner;
    int (*player)(void *,void **);
    int (*session)(void *,uint32_t *);
    int (*dead)(void *,void *,uint8_t *);
    int (*objectives_block)(void *,uint8_t *);
    int (*current)(void *,int32_t *);
    int (*select)(void *,int32_t);
    int (*capture_flags)(void *),(*capture_player)(void *); //60410 objective status,4C150 player snapshot
    int (*sound)(void *),(*message)(void *),(*save)(void *); //216BD0,136C50,16A60 adaptive reward; not disk I/O
    UCCheckpointStartEvent event;
} UCCheckpointOperation;
/*136E30 up to/including its permanent park. Pending is cleared BEFORE services;
 * parked is the native continuation, not a fabricated guest field. Required
 * callbacks are actual services, never successful placeholders. Objectives
 * scans ALL named OBJECTIVES and blocks if any actual D0==2. Player resolution
 * precedes session query; nonzero session bypasses only missing/dead-player gate.
 * Number/group are live across callbacks.0 idle/parked/completed,1 invalid,
 *2 committed failure; failure cannot be retried as though nothing happened. */
int uc_checkpoint_operate(const UCCheckpointOperation *);
#ifdef __cplusplus
}
#endif
#endif

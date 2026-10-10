#ifndef UC_NATIVE_PLAYER_EQUIPMENT_TRANSITION_H
#define UC_NATIVE_PLAYER_EQUIPMENT_TRANSITION_H
#include "mainplayer_actions.h"
#include "mainplayer_state.h"
#ifdef __cplusplus
extern "C" {
#endif
/* One borrowed inventory/action/control graph. The remaining scalar fields are
 * the actual player's switch fields, not a second equipment owner. Initial and
 * service-produced domain: item0/1/2/11/17 for update; request excludes current shield0 or requested item0/1/category2 quick-use, category0 input only, ordinary nonlowered presentation,
 * select-empty/previous are admitted for update only (actual42270 priority);
 * request rejects them. Return assumes actual world mode virtual20==0; both
 * flags commit without waiting for Lower, after the live completion query.
 * No special action5/6. All objects and services
 * remain valid and nonreentrant throughout a call. */
typedef struct UCPlayerEquipmentTransition {
    UCMainplayerState *inventory;
    UCMainplayerActionState *action;
    UCMainplayerEquipControl *control;
    uint32_t lowering_mode; /* +438 */
    int32_t previous_equipped, special_action; /* +598,+7F8 */
    uint8_t blocked, switch_blocked; /* +202,+304 */
    uint8_t lowered, raising, restore_lowered; /* +2AC,+2AD,+2AE */
    uint8_t dirty0,dirty1; /* +2D0,+2D1 */
} UCPlayerEquipmentTransition;
typedef struct UCPlayerEquipmentSwitchInput {
    float category_pressed[3]; /* Manager virtual8 channels4,5,6, mode1. */
    uint8_t current_available; /* Resolved42DA0 on the captured current item. */
} UCPlayerEquipmentSwitchInput;

/* Actual BE230 reset fields; no invented effects or duplicate action owner. */
typedef struct UCPlayerEquipmentReset {
    float at224[3],at2e0[7];
    uint8_t at2fc;
} UCPlayerEquipmentReset;
UCMainplayerStatus uc_player_equipment_clear(UCPlayerEquipmentReset *);
/* BFE00's admitted old-state0/7/8/13 paths have no cleanup call, including
 * return from cutscene13. Other old states remain outside this adapter. */
UCMainplayerStatus uc_player_equipment_idle_transition(UCMainplayerActionState *,uint32_t state,uint32_t stage);

typedef struct UCPlayerEquipmentServices {
    /* Pure admission: nonzero means every reachable service can complete while
     * preserving the domain/lifetimes. operation0=request,1=state8 update. No
     * implicit successful fallback exists for any required completing service. */
    int (*preflight)(void *,const UCPlayerEquipmentTransition *,uint32_t operation);
    void (*select_automatic)(void *); /* 42870, writes the SAME inventory. */
    void (*select_category0)(void *); /* 42840(0), including its selection timer. */
    void (*transition)(void *,uint32_t state,uint32_t stage); /* Full BFE00. */
    void (*action)(void *,uint32_t action); /* Full C1E60(action,0,0). */
    void (*switch_sound)(void *); /* 216BD0:41F50 ((item0||select_empty)&&!select_previous) selects ShieldEquip, else SwitchWeapon. */
    uint8_t (*controller_done)(void *); /* Live first current1FB4E0 result. */
    void (*commit_equipment)(void *); /* Full C8930, incl C2740/42270/resources. */
    uint8_t (*current_pending_reload)(void *); /* NEW current item+50 after commit. */
    void (*reload)(void *); /* D1F40, same-call tail after transition(7,0). */
    void (*clear_action)(void *); /* BE230's actual scalar reset. */
    uint8_t (*reload_gate)(void *); /* CF770, can change action state. */
    void (*reevaluate)(void *); /* D1AE0, can change action state. */
    void (*tail)(void *); /* BE870, after the ordinary state2 polling route. */
} UCPlayerEquipmentServices;

/* Full C4CB0 ordinary domain. Request changes state to8 only; the caller MUST
 * execute D25B0's prior published reload-marker commit against OLD equipment
 * before calling state8 update. Requests targeting item0/1 and category1/2 presses
 * are unsupported: original Taser selection owns a separate quick-use branch.
 * Hook effects are completing, not rollbackable. Failure is admission-only;
 * preflight must not promise completion for unsupported effect owners. */
UCMainplayerStatus uc_player_equipment_request(UCPlayerEquipmentTransition *,
    const UCPlayerEquipmentSwitchInput *,const UCPlayerEquipmentServices *,void *,uint8_t *requested);
/* Full CF8A0 for state8/stages0..2. Stage0 falls through to live stage1 poll
 * immediately; stage1 publication of Raise does NOT also run stage2 this call.
 * Selected-item reload resumes immediately before ordinary Raise. */
UCMainplayerStatus uc_player_equipment_update(UCPlayerEquipmentTransition *,
    const UCPlayerEquipmentServices *,void *);
#ifdef __cplusplus
}
#endif
#endif

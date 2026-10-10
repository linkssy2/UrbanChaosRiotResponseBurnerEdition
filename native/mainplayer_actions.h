#ifndef UC_NATIVE_MAINPLAYER_ACTIONS_H
#define UC_NATIVE_MAINPLAYER_ACTIONS_H
#include "animation_controller.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Resolved query values from BEB90. Item2/17 only. The item owner supplies
 * 434B0/438D0 results; input owner supplies the original channel11 edge value.
 * These queries must be stable for this call. Finite input only. */
typedef struct UCMainplayerFireGate {
    uint32_t item, manager_flags, input_flags;
    int32_t fire_mode;
    uint8_t alternate, can_current, can_other;
    float fire_pressed;
} UCMainplayerFireGate;
typedef struct UCMainplayerFireDecision {
    uint8_t allowed, alternate, misfire;
} UCMainplayerFireDecision;
UCAnimationStatus uc_mainplayer_fire_gate(const UCMainplayerFireGate *in,
    UCMainplayerFireDecision *out);

/* CF770 before its BFE00/D1F40 calls. reload_amount is actual43C00, NOT the
 * raw shared reserve: use uc_weapon_reload_amount for the admitted inventory.
 * can_primary is
 * 438D0(item,0), permission is mode virtual34(1), reload_pressed is channel7
 * edge. Forced/manual/reloading statistics remain explicit output requests.
 * A requested transition is state7,stage0 then D1F40 in the SAME update. */
typedef struct UCMainplayerReloadGate {
    uint32_t item, consumed;
    int32_t reload_amount;
    uint8_t lowered, pending, can_primary, blocked, permission;
    float reload_pressed;
} UCMainplayerReloadGate;
typedef struct UCMainplayerReloadDecision {
    uint8_t requested, forced_stat, manual_stat, tutorial_record;
} UCMainplayerReloadDecision;
UCAnimationStatus uc_mainplayer_reload_gate(const UCMainplayerReloadGate *in,
    UCMainplayerReloadDecision *out);

/* Fields owned by the admitted action fragments, in source order. No actor,
 * inventory, resource or controller lifetime is invented here. */
typedef struct UCMainplayerActionState {
    uint32_t state, stage, animation, input_flags; /* +170,+174,+3E8,+4AC */
    uint8_t ready_pulse, ready_latch, reload_latch; /* +198,+1A4,+1A5 */
} UCMainplayerActionState;

/* Ordinary PRIMARY item2/17 (434B0==4), state4/stage0 only.
 * enter: CFF4A..CFF76; apply returned Idle request when !=-1, then execute
 * actual item43310. It is an outstanding owner operation, not a successful shot.
 * animation: CFFEB..D001B AFTER43310; apply returned Fire through C1E60 owner.
 * Next evaluate439D0; when true CBA20 MUST complete before commit.
 * commit: D002D..D0042; only after the preceding owners complete (or439D0=false).
 * Then immediately reenter fire_wait; do not advance gameplay/animation time.
 * Each boundary is separate to prevent mutation past an unexecuted shot. */
UCAnimationStatus uc_mainplayer_fire_enter(uint32_t item,
    UCMainplayerActionState *state,int32_t *animation_request);
UCAnimationStatus uc_mainplayer_fire_animation(uint32_t item,
    UCMainplayerActionState *state,int32_t *animation_request);
UCAnimationStatus uc_mainplayer_fire_commit(uint32_t item,
    UCMainplayerActionState *state);
typedef enum UCMainplayerFireNext {
    UC_MAINPLAYER_FIRE_INPUT=0, UC_MAINPLAYER_FIRE_FINISH=1
} UCMainplayerFireNext;
/* CFBC0 entry latch + stage2 prefix, primary mode4/current Fire3 only.
 * FINISH: execute BFE00's BC6A0 cleanup, then fire_finish below, then D1AE0
 * (reload priority followed by fire gate), without waiting another update.
 * INPUT: C5510 mode-toggle, C4CB0 item-switch; only if state still4 D1A20;
 * only if STILL4 CF770. Then original BE870 tail remains outstanding.
 * No callback in this list may silently be treated as completed. */
UCAnimationStatus uc_mainplayer_fire_wait(uint32_t item,
    UCMainplayerActionState *state,uint8_t controller_done,
    UCMainplayerFireNext *next);
UCAnimationStatus uc_mainplayer_fire_finish(uint32_t item,
    UCMainplayerActionState *state);
/* D2015..D2038: state7/stage0; apply Reload4 before executing pending/new
 * reload ownership. This does not write item pending/ammo/progress, destroy
 * overlays, seek controllers, or play the reload request. */
UCAnimationStatus uc_mainplayer_reload_enter(uint32_t item,
    UCMainplayerActionState *state,int32_t *animation_request);

typedef struct UCMainplayerReloadOwner {
    uint32_t lowering_mode; /* player+438 */
    float progress, lowering; /* item+4C, player+1AC */
    uint8_t pending; /* item+50 */
} UCMainplayerReloadOwner;
typedef enum UCMainplayerReloadStart {
    UC_MAINPLAYER_RELOAD_LOAD_SOUND=0, UC_MAINPLAYER_RELOAD_RESUME=1
} UCMainplayerReloadStart;
/* D203D..D20B8, AFTER reload_enter's Reload4 has actually been applied.
 * LOAD_SOUND publishes pending1/progress0 BEFORE the outstanding
 * 216BD0(player,item+44,"Load",0) request. Execute it before ending this tick.
 * RESUME preserves pending/progress: first detach overlay+1A8 via virtual38,
 * destroy via virtual0(1), and clear its pointer if present; then seek every
 * CURRENT presentation-list controller (gun and arms, not fading controllers)
 * with reload_seek; only then reload_resumed publishes lowering=1.
 * This call does not destroy overlays, seek, lower, or play sound itself. */
UCAnimationStatus uc_mainplayer_reload_start(uint32_t item,
    const UCMainplayerActionState *,UCMainplayerReloadOwner *,UCMainplayerReloadStart *);
UCAnimationStatus uc_mainplayer_reload_resumed(uint32_t item,
    const UCMainplayerActionState *,UCMainplayerReloadOwner *);
/* Exact1FB1C0->2A7B70->A5C00 projection for the ordinary controller vtable:
 * writes phase ONLY, retaining previous, repeats, flags, weight and rate.
 * <=32 distinct current controllers, phase in [0,1). Not a list-wide reset. */
UCAnimationStatus uc_mainplayer_reload_seek(UCAnimationController *const *,
    uint32_t count,float phase);
/* D1F6D..D1FA5: reload_amount43C00!=0 clears the persistent ready latch. This
 * is the transfer amount from uc_weapon_reload_amount, not raw shared reserve.
 * Otherwise
 * previous published+198 sets it and clears lowering_mode. When reevaluate=1,
 * execute D1AE0 (including its lowered/input/reload/fire owners) BEFORE poll.
 * No new event is published here. GunHalfReload+195 is not read by D1F40. */
UCAnimationStatus uc_mainplayer_reload_wait(uint32_t item,int32_t reload_amount,
    UCMainplayerActionState *,UCMainplayerReloadOwner *,uint8_t *reevaluate);
/* D1FAC..D2014, AFTER any required D1AE0 and using the then-current controller.
 * Done is !controller || !(flags&0x40000000), from1FB4E0/BC410. Done AND
 * state still7 clears pending and returns Idle0. Apply Idle through C1E60,
 * then call reload_finish. Otherwise returns-1 and saves the original quotient
 * (phase/phase_scale)/float(1/phase_scale), preserving its intermediate rounding.
 * The item owner is the one captured at D1F40 entry, even if D1AE0 changes state.
 * No physical-x87 claim; finite ordinary state and RN with DAZ/FTZ off only. */
UCAnimationStatus uc_mainplayer_reload_poll(uint32_t item,
    const UCAnimationController *,UCMainplayerActionState *,
    UCMainplayerReloadOwner *,int32_t *animation_request);
/* BFE00's actual state7 table entry goes straight to BFE94: no cleanup call.
 * Invoke only after poll's Idle request has been applied. */
UCAnimationStatus uc_mainplayer_reload_finish(uint32_t item,UCMainplayerActionState *);
/* BF09F..BF0DA lowering update: <=.01 becomes+0; otherwise x-delta*x*6.
 * It is a separate ordinary player update, not implicit reload-start work. */
UCAnimationStatus uc_mainplayer_reload_lowering(float delta,float *lowering);

/* All failures preserve state and output; all inputs/outputs are disjoint.
 * These are ordered fragments, not a complete player tick. Secondary modes,
 * shot/damage/sound effects, switching and reload owner callbacks are separate.
 * No arbitrary default time, animation completion, or ammo result is supplied. */
#ifdef __cplusplus
}
#endif
#endif

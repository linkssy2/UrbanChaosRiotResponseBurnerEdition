#ifndef UC_NATIVE_MAINPLAYER_RELOAD_OVERLAY_H
#define UC_NATIVE_MAINPLAYER_RELOAD_OVERLAY_H
#include "mainplayer_actions.h"
#ifdef __cplusplus
extern "C" {
#endif

/* CF6A0's player+1A8. This controller only advances timing/events: D32A9 never
 * samples its pose. Inline storage replaces the original object and unused
 * sampler allocation. Clip identity is pointer identity (BE6D0), not a name.
 * Clip/annotations are borrowed immutable bank entries, retained through every
 * queued event's consumption, including after clear/replacement/completion. */
typedef struct UCMainplayerReloadOverlay {
    UCAnimationController controller;
    const UCAnimationClip *clip;
    const UCAnimationAnnotations *annotations;
} UCMainplayerReloadOverlay;

/* After C1D60(player,Reload4,item-definition) resolves a real clip, replaces
 * any old overlay and initializes with flags0. Pending resumes phase ONLY;
 * fresh publishes pending1/progress0 and requires Load sound before activate.
 * Sound is 216BD0(player,CURRENT-inventory-item+44,"Load",0), which need not
 * be the supplied item's bank. No sound request is silently executed here.
 * item2/17 only, finite forward controller domain. Failure preserves outputs. */
UCAnimationStatus uc_mainplayer_reload_overlay_start(uint32_t item,
    UCMainplayerReloadOverlay *,const UCAnimationClip *,const UCAnimationAnnotations *,
    UCMainplayerReloadOwner *,uint8_t *load_sound);
/* Invoke after the required Load sound owner completes (or immediately for
 * resume). BC430 sets BOTH high bits; no repeat/weight override. */
UCAnimationStatus uc_mainplayer_reload_overlay_activate(UCMainplayerReloadOverlay *);
/* 1F2380->BE6E0 releases the sampler/object; it does NOT clear flags (BC420).
 * Inline owner has no heap sampler. This releases only borrowed links, never
 * clip/annotations or item pending/progress. Also use before D1F40 resume seek. */
void uc_mainplayer_reload_overlay_clear(UCMainplayerReloadOverlay *);

/* D329B..D332B. Resolve the current item category and Reload clip through the
 * original owners. Nonzero category, missing clip or different clip discards
 * overlay WITHOUT clearing item pending/progress. Otherwise advance scalar
 * controller and APPEND events to the bound WEAPON state's existing queue;
 * do not reset the queue left by normal1F1D60. Ignore scalar return1/2 here.
 * dispatch=1 requires actual1F1CE0 dispatch before finish, even an empty queue.
 * Existing event modules handle qualified local handlers; unknown/global refs
 * remain explicit. No dispatch or latch publication is hidden in this call. */
UCAnimationStatus uc_mainplayer_reload_overlay_advance(UCMainplayerReloadOverlay *,
    uint32_t current_category,const UCAnimationClip *resolved_reload,float delta,
    UCAnimationEventQueue *,uint8_t *dispatch);
/* AFTER dispatch, with the CURRENT overlay (re-read player+1A8 if a handler
 * replaced it) and the item captured before advance. A null/deleted current
 * overlay is unsupported: original dereferences it; this API rejects it.
 * Flag30 still set:
 * save RAW phase (not D1F40's quotient). Otherwise clear overlay then pending0,
 * preserving progress. Finally caller performs late1F9610 publication exactly
 * once after normal and overlay dispatch; its pulses are read next tick.
 * No ammo mutation, +195/+1A5 alias, D0C30 switching or sound implementation. */
UCAnimationStatus uc_mainplayer_reload_overlay_finish(UCMainplayerReloadOverlay *,
    UCMainplayerReloadOwner *captured_item);

#ifdef __cplusplus
}
#endif
#endif

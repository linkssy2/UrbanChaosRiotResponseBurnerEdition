#ifndef UC_NATIVE_MAINPLAYER_PRESENTATION_H
#define UC_NATIVE_MAINPLAYER_PRESENTATION_H
#include "animation_controller.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct UCMainplayerPresentationResource {
    const char *model, *animation_catalog;
    uint32_t flashlight_mode;
    float field_of_view;
    float base_offset[3], aimed_offset[3];
} UCMainplayerPresentationResource;

typedef struct UCMainplayerPresentationAction {
    const char *alias, *animation_catalog;
    uint32_t action;
    int32_t repeats, blend_argument, owner_slot;
} UCMainplayerPresentationAction;

/* Fixed original item1/item2/item11/item17 definitions and normal fresh Westside item0
 * after54FE0 shield selection. Upgraded/burner shield selection is excluded. Unsupported items return ARGUMENT;
 * no registry, equipment selection or resource loading is implied. Returned
 * strings have immutable static lifetime. Every failure preserves outputs. */
UCAnimationStatus uc_mainplayer_presentation_resource(uint32_t item,
    UCMainplayerPresentationResource *out);
/* Original action0..17 alias and apply arguments. Flashlight bytes are supplied
 * owner state; nonzero is true. A named alias need not exist in the catalog:
 * the resource owner must resolve it before changing a controller. */
UCAnimationStatus uc_mainplayer_presentation_action(uint32_t item,uint32_t action,
    uint8_t flashlight_enabled,uint8_t flashlight_compatible,
    int32_t repeats,int32_t blend_argument,UCMainplayerPresentationAction *out);
/* Fresh successfully allocated/applied controller scalar state. Reuses the
 * qualified timing initializer; no allocation, cache, list or event ownership.
 * Caller clears old current controller flag bits1/8, then inserts the new one
 * before existing controllers of equal priority. Do not reset per frame.
 * Output and clip/request are disjoint. Request must come from action above. */
UCAnimationStatus uc_mainplayer_presentation_controller(const UCAnimationClip *clip,
    const UCMainplayerPresentationAction *request,UCAnimationController *out);
/* Post-equipment reset gate, after equipment commit. Writes action0 (Idle) or
 * -1 (no request). Does not choose which granted item becomes equipped. */
UCAnimationStatus uc_mainplayer_presentation_reset_action(uint8_t alternate_mode,
    int32_t equipped_item,int32_t *out);
/* Ordinary reload-trigger projection. First gate the PREVIOUS published pulse
 * against the current item's pending-reload byte, before advancing controllers.
 * Later publish this update's weapon events for the NEXT update. Fresh item
 * construction/disable clears pending reload; reload actions own later writes.
 * The gate reports whether the original ammo-commit call would run; it does not
 * execute it or modify pending state. Nonzero input bytes mean true. */
UCAnimationStatus uc_mainplayer_presentation_reload_gate(uint8_t previous_trigger,
    uint8_t pending_reload,uint8_t *commit_requested);
/* Exact admitted GunReloadTrigger/code4, zero-parameter event family. Unknown
 * events return UNSUPPORTED so callers cannot silently discard their effects.
 * Empty queues publish0; duplicates publish1. Read-only initialized annotation
 * owners must outlive this call. Queue/output are disjoint; errors preserve out.
 * Only the weapon state's local trigger registration feeds this player pulse.
 * Arms' unhandled markers remain outside this helper. */
UCAnimationStatus uc_mainplayer_presentation_reload_publish(
    const UCAnimationEventQueue *weapon_events,uint8_t *next_trigger);

#ifdef __cplusplus
}
#endif
#endif

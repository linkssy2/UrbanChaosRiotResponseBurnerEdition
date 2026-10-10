#ifndef UC_NATIVE_ANIMATION_EVENTS_H
#define UC_NATIVE_ANIMATION_EVENTS_H
#include "animation_controller.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCAnimationDispatch UCAnimationDispatch;
typedef uint8_t (*UCAnimationEventHandler)(UCAnimationEventRef,UCAnimationDispatch *);
typedef struct UCAnimationHandlers { uint32_t count;UCAnimationEventHandler items[16]; } UCAnimationHandlers;
struct UCAnimationDispatch {
    UCAnimationEventQueue queue;
    UCAnimationHandlers *local,*global;
    void *user;
};
/* Original uniqueness/cap16 and stable removal, including during callbacks.
 * A successful call reports added/removed as0 or1. Failure preserves outputs.
 * The result pointer must not overlap registry storage. */
UCAnimationStatus uc_animation_handler_add(UCAnimationHandlers *,UCAnimationEventHandler,int *added);
UCAnimationStatus uc_animation_handler_remove(UCAnimationHandlers *,UCAnimationEventHandler,int *removed);
/* Original local-first, first-true order and stable unhandled compaction.
 * Counts, local registry pointer and queue slots are live, not snapshotted.
 * Callbacks may add/remove handlers, append events or dispatch recursively.
 * Nested dispatch is supported only where it will not invoke a handled/null
 * slot; no null-marker callback equivalence is claimed.
 * The dispatch object, current registry storage, global registry identity and
 * annotation owners must remain alive; global pointer must not change. At every
 * callback boundary counts<=16 and every nonnull queue reference must be valid.
 * Null handled slots can exist internally; attempting to dispatch such a slot
 * is rejected rather than calling a handler with a null original event.
 * Arbitrary callback side effects are not transactional. An invalid callback
 * mutation returns ARGUMENT with those effects retained; caller must recover.
 * No gameplay handler or subscription-wrapper ownership is fabricated. */
UCAnimationStatus uc_animation_dispatch_events(UCAnimationDispatch *);
#ifdef __cplusplus
}
#endif
#endif

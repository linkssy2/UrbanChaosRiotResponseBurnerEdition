#ifndef UC_NATIVE_MAINPLAYER_EVENTS_H
#define UC_NATIVE_MAINPLAYER_EVENTS_H
#include "animation_events.h"
#ifdef __cplusplus
extern "C" {
#endif
enum UCMainplayerEventLatch {
    UC_MAINPLAYER_EVENT_RELOAD,UC_MAINPLAYER_EVENT_HALF_RELOAD,
    UC_MAINPLAYER_EVENT_THROW,UC_MAINPLAYER_EVENT_MELEE_HIT,
    UC_MAINPLAYER_EVENT_READY_TO_FIRE,UC_MAINPLAYER_EVENT_FOOT_LEFT,
    UC_MAINPLAYER_EVENT_FOOT_RIGHT,UC_MAINPLAYER_EVENT_HAND_LEFT,
    UC_MAINPLAYER_EVENT_HAND_RIGHT,UC_MAINPLAYER_EVENT_LADDER_SLIDE,
    UC_MAINPLAYER_EVENT_RUMBLE_SMALL,UC_MAINPLAYER_EVENT_RUMBLE_MEDIUM,
    UC_MAINPLAYER_EVENT_RUMBLE_BIG,UC_MAINPLAYER_EVENT_HEAL,
    UC_MAINPLAYER_EVENT_TWIRL,UC_MAINPLAYER_EVENT_COUNT
};
typedef struct UCMainplayerEventRegistration { const char *name;uint32_t player_offset; } UCMainplayerEventRegistration;
extern const UCMainplayerEventRegistration uc_mainplayer_event_registrations[UC_MAINPLAYER_EVENT_COUNT];
typedef struct UCMainplayerEvents {
    uint32_t codes[UC_MAINPLAYER_EVENT_COUNT];
    uint16_t pending; /* Membership projection of the original pending-code vector. */
    uint8_t published[UC_MAINPLAYER_EVENT_COUNT];
} UCMainplayerEvents;
/* D3910's 15 kind1 records in original registration order. Numeric codes must
 * come from the caller's qualified 1F9CF0 registry; this does not invent missing
 * name mappings. Registration clears the registered bytes; +197 is not owned. */
UCAnimationStatus uc_mainplayer_events_init(const uint32_t codes[UC_MAINPLAYER_EVENT_COUNT],UCMainplayerEvents *);
/* Fixed original XBE: D5840's 68 entries through 1FAE20/1F9CF0. The six absent
 * names resolve to code0, including ReadyToFire; do not invent unique codes.
 * No guest registry/container remains in the native runtime. */
UCAnimationStatus uc_mainplayer_events_init_original(UCMainplayerEvents *);
/* Local 1FAA60 for the animation state bound by BE050/1FAB30. Matches numeric
 * event.code only: name/type/parameter_count do not affect local consumption.
 * Reuses original-qualified local-first dispatcher with NO global handlers.
 * Other refs remain ordered in queue for real handlers or explicit unsupported
 * reporting. Arms must not be passed to the weapon's local subscription owner.
 * Collection accumulates across calls, leaves published bytes unchanged and
 * collapses duplicate pending codes only because 1F9610 observes membership.
 * Failure preserves state/queue. Annotation owners remain caller-owned. */
UCAnimationStatus uc_mainplayer_events_collect(UCMainplayerEvents *,UCAnimationEventQueue *);
/* 1F9610: overwrites every registered byte from pending membership, then clears
 * pending. Empty publication clears old pulses; collection alone never does.
 * Caller uses previous published bytes at the early gate, then publishes at
 * the later owner update (D33A4). No ammo, rumble, sound or shot effects occur. */
UCAnimationStatus uc_mainplayer_events_publish(UCMainplayerEvents *);
#ifdef __cplusplus
}
#endif
#endif

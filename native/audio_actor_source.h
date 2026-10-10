#ifndef UC_NATIVE_AUDIO_ACTOR_SOURCE_H
#define UC_NATIVE_AUDIO_ACTOR_SOURCE_H
#include "audio_source_controller.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCAudioActorView {
 void **helper; /* actual same entity+78; NULL or a live UCAudioSourceController */
 const float *position; /* actual same entity+40 three floats, never a copy owner */
} UCAudioActorView;
typedef struct UCAudioActorOverride {const void *argument;uint16_t volume;} UCAudioActorOverride;
typedef struct UCAudioActorBank {
 UCAudioSourceBank resource; /* actual borrowed resource identity/type */
 const UCAudioActorOverride **observer; /* SAME mutable bank+A0 slot */
} UCAudioActorBank;
typedef struct UCAudioActorServices {
 void *context;
 int (*preflight)(void *,const UCAudioActorView *,const UCAudioActorBank *); /*pure0admit*/
 UCAudioSourceController *(*allocate)(void *); /* distinct real fresh storage */
 const UCAudioSourceServices *source;
} UCAudioActorServices;
/* Full216C10→216B10/216100→216400 ordinary actor helper. Its one source owner
 * is reused from audio_source_controller; empty notifier storage remains the
 * same admitted native empty-list representation. No copied actor position,
 * engine, instance/track graph or bank observer authority. Successful allocator
 * domain; unexpected NULL gives committed2, not the original null dereference.
 * Constructor reads actual position AFTER allocation, publishes helper AFTER
 * initialization. Existing helper may have detached/other attached position as
 * source allows. Services preserve actor/helper/metadata lifetimes and helper
 * slot, except allocator effects before publication; source callbacks follow
 * their existing mutation contract. No reentry or notifier edits.
 * This caller does NOT enable flag24. Bank!=104 still ensures engine/clears25,
 * then returns accepted0. NULL bank returns0 before helper/volume admission.
 * Type/event projections are pure. Successful event temporarily publishes the
 * stack override to bank+A0, calls actual current engine request, then clears
 * observer to NULL (does not restore old value). Request may read the override
 * synchronously, not retain it. Argument is the actual borrowed pointer value.
 * Volume finite <=32768 when nonnegative, RN/default DAZ/FTZ. Negative gives
 * FFFF; nonnegative computes trunc(double(volume)*65535) then low16. Callback
 * FP exception observation excluded; callbacks preserve FP mode. Request must
 * complete real246BB0 work; this is not successful placeholder playback.
 * Status0complete/1pure rejection/2committed failure; accepted only written0.
 * On unexpected request failure the temporary observer is cleared for lifetime
 * safety; effects are retained and no original failure equivalence is claimed.
 * Caller owns later real source close and allocation release, never immediate
 * free after successful play. This does not implement generic actor teardown. */
int uc_audio_actor_play(const UCAudioActorView *,const UCAudioActorBank *,const char *,
 const void *argument,float volume,const UCAudioActorServices *,uint8_t *accepted);
#ifdef __cplusplus
}
#endif
#endif

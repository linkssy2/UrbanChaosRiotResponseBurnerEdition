#ifndef UC_NATIVE_ANIMATION_SOUND_H
#define UC_NATIVE_ANIMATION_SOUND_H
#include "animation_controller.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCAnimationSoundServices {
    void *context;
    const void *(*bank)(void *context,const void *owner);
    void (*play)(void *context,const void *owner,const void *bank,const char *name);
} UCAnimationSoundServices;
/* Full20BF50 projection. Type matching is ASCII case insensitive. Sound is
 * handled even with a null owner or parameter_count!=1; no parameter payload
 * is read. Otherwise query the captured owner's virtual44 bank, then request
 * event.name through216BD0 (whose result the source ignores). Completing
 * callbacks retain owner/name lifetime and execute their actual services.
 * Unknown types are left unhandled. Errors precede effects and preserve out. */
UCAnimationStatus uc_animation_sound(const UCAnimationEvent *event,
    const void *owner,const UCAnimationSoundServices *,uint8_t *handled);
/* BE1E0 plus41F80/41FA0 selection. Ordinary2/17 select active in every state;
 * state!=1 redirects active0/26 to previous. The selected slot must be0..26.
 * This resolves an index, not a fabricated bank or resource lifetime. */
UCAnimationStatus uc_animation_sound_item(uint32_t state,int32_t active,
    int32_t previous,uint32_t *item);
#ifdef __cplusplus
}
#endif
#endif

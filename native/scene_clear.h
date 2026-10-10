#ifndef UC_SCENE_CLEAR_H
#define UC_SCENE_CLEAR_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCSceneClearState {
 const void *const *owner;
 const uint8_t *depth,*texture,*ambient;
 const uint32_t *color;
 uint32_t *color_mask;
} UCSceneClearState;
typedef struct UCSceneClearHooks {
 void (*state)(void *,uint32_t method,uint32_t value);
 void (*clear)(void *,uint32_t flags,uint32_t color,float depth,uint32_t stencil);
 uint32_t (*busy)(void *);
 void (*submit)(void *);
} UCSceneClearHooks;
/* Whole96940: capture color-enable from collector+C/+14/+12, publish40358,
 * then read CURRENT color and depth gate, issue full-viewport clear and submit
 * if current SDK busy query is zero. Source strips alpha unconditionally;
 * requested F0 clears all four color channels despite the RGB draw mask.
 * Hooks are required completing actual services, not success substitutes.
 * They may mutate borrowed values but preserve storage/lifetimes and cannot
 * reenter this call. Pointer addresses/hooks remain stable and output mask
 * must not alias input fields.0 complete,1 invalid before effects.
 * Caller owns9E670 outer clear gate, actual current-view color, viewport,
 * SDK attachment formats and frame scheduling. No GPU/scene constructor. */
int uc_scene_clear(const UCSceneClearState *,const UCSceneClearHooks *,void *);
#ifdef __cplusplus
}
#endif
#endif

#ifndef UC_GUNFLARE_SPRITE_H
#define UC_GUNFLARE_SPRITE_H
#include <stdint.h>
typedef struct UCTextureResource UCTextureResource;
#ifdef __cplusplus
extern "C" {
#endif
struct UCParticleRegistry;
/* Borrow the actual embedded186980 fields, including GunFlare's authoritative
 * visible(B0), enabled(B8) and position(A4..AC). No particle lifecycle exists.
 * Slots remain live/disjoint until unregister; radius/position need not have
 * initialized values in the admitted quiet path and are never read there. */
typedef struct UCGunFlareSpriteSource {
    uint8_t *visible,*enabled;
    const UCTextureResource *const *texture;
    const float *position,*radius;
} UCGunFlareSpriteSource;
typedef struct UCGunFlareSpriteRead {
    uint32_t count,type;
    const UCTextureResource *texture;
} UCGunFlareSpriteRead;
/* Actual186980 initialization then same-registry89FF0 registration. token is
 * actual sprite+4. Existing texture/position/radius bits are untouched.
 * Preallocated pool-full is success/token0. Stable current graphics registry
 * is supplied; unregister through that SAME owner's remove before freeing the
 * borrowed source.1867D0 preserves the stored token after removal. */
int uc_gunflare_sprite_construct(struct UCParticleRegistry *,const UCGunFlareSpriteSource *,uintptr_t *token);
/* Exact186820 outputs then visible=0, including count1 metadata. This does not
 * implement186860 positive-count geometry or its shared random consumption. */
int uc_gunflare_sprite_read(const UCGunFlareSpriteSource *,UCGunFlareSpriteRead *);
typedef struct UCGunFlareSpriteContext {const void *environment;} UCGunFlareSpriteContext;
typedef struct UCGunFlareSpriteHooks {
    void *(*current_environment)(void *); /* current graphics+A4 */
    const UCGunFlareSpriteContext *(*environment_context)(void *,void *); /* returned owner+1C */
    void *(*current_camera)(void *); /* current graphics+50 */
} UCGunFlareSpriteHooks;
/* Actual99DD0 zero-count path: zero-return+10, current environment/context,
 * context+10 read, current camera, consuming186820, early return. Ordinary
 * first-person2/17 admits enabled==0. Required completing hooks may change the
 * visible/texture slots but preserve enabled==0, source/context lifetimes and
 * valid nonnull returned objects. No geometry/GPU/RNG callbacks are replaced.
 * Same-thread; no reentry/destruction/alias of outputs. Return0 completed,
 * 1 initial rejection without effects,2 committed service contract failure.
 * On2 do not retry. Source virtual+18 still runs in the shared epilogue; the
 * supported472CD8 target is the actual11030 RET, with no native effect. */
int uc_gunflare_sprite_draw_quiet(const UCGunFlareSpriteSource *,const UCGunFlareSpriteHooks *,void *);
#ifdef __cplusplus
}
#endif
#endif

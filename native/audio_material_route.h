#ifndef UC_NATIVE_AUDIO_MATERIAL_ROUTE_H
#define UC_NATIVE_AUDIO_MATERIAL_ROUTE_H
#include "audio_asset.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCAudioMaterialActor { void *controller; } UCAudioMaterialActor;
typedef struct UCAudioMaterialServices {
    void *context;
    int (*preflight)(void *,const UCAudioMaterialActor *,const void *bank);
    void *(*create)(void *,UCAudioMaterialActor *);
    uint8_t (*play)(void *,void *controller,const void *bank,const char *name);
} UCAudioMaterialServices;
/* Actual20D360 table4FC9F8, admitted indices0..35; NULL outside that range.
 * Immutable native strings preserve the retail spellings and repeated entries. */
const char *uc_audio_material_name(uint32_t material);
typedef struct UCAudioWorldMaterialServices {
    void *context;
    /*20D900(point,1). NULL is the original unavailable source branch.*/
    void *(*source)(void *,const float point[3],float radius);
    uint8_t (*play)(void *,void *source,const void *bank,const char *name,uint32_t flags);
} UCAudioWorldMaterialServices;
/*Whole20DA70 routing: same retained positional source for material, Default,
 * then unprefixed material only when prefix is nonnull. Actual playback and
 * spatial source pooling remain required caller services; no voice claim.*/
UCAudioStatus uc_audio_world_material_request(const void *bank,const float point[3],
    uint32_t material,const char *prefix,uint32_t flags,const UCAudioWorldMaterialServices *);
/* Original216C60 under the concrete20D360 material owner. The caller owns a
 * separate output buffer instead of the source global buffer51C500. Prefix
 * (NULL means empty) and material table stay immutable; all input/output spans
 * are valid and disjoint, and this route is not reentered. Capacity1..4096.
 * Pure preflight admits completing create/play hooks. Creation must return a
 * live nonnull controller. Play may update actor.controller, which is reloaded
 * on fallback, and its LOW BYTE decides fallback; the second result is ignored.
 * Missing bank still publishes prefix+Default. Output is the last requested
 * name, not a sound-success boolean. No resource, voice or actor ownership is
 * fabricated. Pre-effect rejection preserves actor/output; a completing hook
 * that returns NULL violates admission and returns an error after commitment. */
UCAudioStatus uc_audio_material_request(UCAudioMaterialActor *,const void *bank,
    uint32_t material,const char *prefix,char *output,size_t capacity,
    const UCAudioMaterialServices *);
#ifdef __cplusplus
}
#endif
#endif

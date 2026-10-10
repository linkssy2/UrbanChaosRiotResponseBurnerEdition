#ifndef UC_SCENE_AMBIENT_PASS_H
#define UC_SCENE_AMBIENT_PASS_H
#include "scene_texture_pass.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Pure A4890 projection for immutable, pre-resolved ordered float4 contributions.
 * Unpack baseARGB, add each float4 in list order, truncate/clamp into packedARGB.
 * count<=65536, finite arithmetic in signed conversion range; disjoint output.
 * Actual list/provider dispatch and spatial contributions remain caller owned.
 * Ordinary B3000 pointlight virtual+90=E9330 contributes exact float4zero.
 * Returns1 success,0 rejection preserving output. No allocation. */
int uc_scene_ambient_color(uint32_t base_argb,const float (*contributions)[4],uint32_t count,uint32_t *out);
typedef struct UCSceneAmbientState {uint32_t *fog_enabled,*dirty;} UCSceneAmbientState;
typedef struct UCSceneAmbientHooks {
 void (*render_state)(void *,uint32_t method,uint32_t value);
 void (*camera_position)(void *,float xyz[3]);
 uint32_t (*ambient)(void *,const float xyz[3]);
 float (*exposure)(void *);
 void (*world)(void *,const UCSceneModelRender *);
 uint32_t (*alpha_exempt)(void *,const UCSceneModelRender *); /* LOW BYTE */
 void (*receiver)(void *,const UCSceneModelRender *,UCSceneModelPass *);
} UCSceneAmbientHooks;
/* Full977A0 bracket, same ordered scene cursor/pass as96BE0. Initially ended
 * cursor returns1 without later reads. Owner presence is captured collector+C;
 * ambient_enabled is actual collector+12; blend consumes only low byte. Query,
 * light and all other pass fields inherited; mode/color/partition updated.
 * Source fog_enabled=0 and dirty|=3000 follow initial blend publication.
 * Nonzero owner+enabled: camera, ambient packedARGB, unpack into pass, exposure
 * callback, float32 component divisions occur in that order. Otherwise color0
 * and none of those providers invoked. alpha_ref is actual4EAC88. Blend path
 * checks each model+58 after world/partition; low-byte true disables alpha-test,
 * ref and depthwrite; false enables alpha-test/ref/depthwrite. Final blend and
 * alpha disable retained. Services may change only their own backend/provider
 * state, never borrowed inputs. Finite camera/nonzero finite exposure and finite
 * division are caller preadmission duties; violations return0 AFTER prior
 * effects, not rollback/retry guarantees. Other guards precede effects.
 * No allocation/guest VM/full99240 or current-level ambient selection claim. */
int uc_scene_ambient_pass(UCSceneTextureCursor *,UCSceneModelPass *,uint32_t owner_present,
 uint8_t ambient_enabled,uint32_t blend,uint32_t alpha_ref,const UCSceneAmbientState *,
 const UCSceneAmbientHooks *,void *context);
#ifdef __cplusplus
}
#endif
#endif

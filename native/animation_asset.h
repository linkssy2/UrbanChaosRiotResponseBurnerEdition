#ifndef UC_NATIVE_ANIMATION_ASSET_H
#define UC_NATIVE_ANIMATION_ASSET_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum UCAnimationStatus {UC_ANIMATION_OK,UC_ANIMATION_ARGUMENT,UC_ANIMATION_FORMAT,
 UC_ANIMATION_UNSUPPORTED,UC_ANIMATION_LIMIT,UC_ANIMATION_MEMORY} UCAnimationStatus;
typedef struct UCAnimationTemplate {
 char *source_label,**bone_names,**clip_names,**clip_aliases;
 uint8_t *bone_flags;uint32_t bone_count,clip_count;int32_t source_default;
} UCAnimationTemplate;
typedef struct UCAnimationKey {float rotation[4],translation[3];} UCAnimationKey;
typedef struct UCAnimationClip {
 uint8_t source_header[42];
 int32_t *source_tracks; /* Negative: bone=~value, translation fixed at key0. */
 float *key_times; /* Authored normalized times, not a uniform FPS grid. */
 UCAnimationKey *keys; /* Frame-major authored expansion/cold-seek lower samples.
   Omitted rotations retain prior key. This does NOT encode the history-dependent
   upper cache buffer required by the original forward/reverse sampler. */
 uint8_t *annotations;size_t annotation_bytes; /* Validated source event/tag tail;
   retained as data only. No event dispatch or argument semantics implemented. */
 uint32_t track_count,key_count,event_count,parameter_count,tag_count;
 int32_t root_track;float source_timing[3]; /* Phase rate, two blend durations;
   step=timing[0]*controller_rate*delta; time units follow the supplied delta. */
 uint32_t source_control,source_format;uint16_t source_flags;
} UCAnimationClip;
/* Input is a decompressed archive entry. Output must be zero/freed first.
 * Success owns all returned storage; failure leaves output empty.
 * <=64MiB input, <=4096 tracks, <=65536 keys, <=4M track/key pairs.
 * KFNM scope: source control0/2, quaternion format0/1/2, flags1/9, strict0..1 key times,
 * finite data, complete first rotation key. Unknown layouts are rejected.
 * This decodes asset samples, not interpolation/blending, world pose or events. */
UCAnimationStatus uc_animation_template_load(const void *,size_t,UCAnimationTemplate *);
/* Alias lookup in an immutable loaded template, with original ASCII case folding.
 * NULL alias means empty, as in 1F08C0. OK + UINT32_MAX means no match; no default
 * fallback. A hit is the clip_names/clip_aliases source index. Non-ASCII, matching
 * duplicate aliases, or reused matched clip paths are unsupported (resource
 * identity/insertion order is not owned here). Strings <=4095 bytes; output must
 * not overlap template storage. Failure leaves output unchanged. */
UCAnimationStatus uc_animation_template_find_clip(const UCAnimationTemplate *,const char *alias,uint32_t *index);
/* Original1F1610: try prefix+alias first, then unprefixed alias on a miss.
 * A supplied prefix requires a nonnull alias and combined length<1024 (the
 * original stack buffer). NULL prefix delegates directly to find_clip. Same
 * immutable catalog/ASCII/duplicate policy; failure preserves output. */
UCAnimationStatus uc_animation_template_find_action(const UCAnimationTemplate *,const char *prefix,const char *alias,uint32_t *index);
UCAnimationStatus uc_animation_clip_load(const void *,size_t,UCAnimationClip *);
/* Decode one borrowed 4- or 6-byte packed key. Output is unchanged on failure. */
UCAnimationStatus uc_animation_quaternion_decode(const void *,size_t,float out[4]);
void uc_animation_template_free(UCAnimationTemplate *);
void uc_animation_clip_free(UCAnimationClip *);
const char *uc_animation_status(UCAnimationStatus);
#ifdef __cplusplus
}
#endif
#endif

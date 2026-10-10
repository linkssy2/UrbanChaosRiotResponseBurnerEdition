#ifndef UC_NATIVE_SIMPLE_CUTSCENE_H
#define UC_NATIVE_SIMPLE_CUTSCENE_H
#include "entity_fresh.h"
#ifdef __cplusplus
extern "C" {
#endif
/* One authoritative 114A10 tail. Model/actor/ID/flags remain in FreshView.
 * Arrays and referenced resources stay alive through every completing service. */
typedef struct UCSimpleCutscene {
    const int32_t *finish_flags,*wait_flags,*prefade_flags;
    int32_t finish_count,wait_count,prefade_count,camera_id,end_group;
    uint32_t camera_sequence;
    int32_t restart,sound_bone;
    float elapsed;
    uint8_t waiting,fade_requested,switch_camera,fade,fade_up,fade_complete;
    uint8_t finishing,can_skip,goto_restart,skipped,ended,handed_off;
} UCSimpleCutscene;
/* Completing synchronous endpoints of115100. preflight is pure. Other hooks
 * may mutate this same live tail/base and their owners, but retain its storage,
 * valid arrays and finite RN/default-DAZ/FTZ arithmetic domain. No callback may
 * reenter update. A nonzero hook result stops after committed effects (2).
 * Missing hooks are rejected before effects (1), never treated as success.
 * resolve_clip returns NULL for the genuine resource miss. start_animation is
 * actual1FB630(...,0,1,-1), not a guessed duration. start_audio performs actual
 * 216B70 helper creation/bone lookup/114DF0 and216BD0 before bind_trigger runs.
 * bind_trigger is actual1FAAE0+21AEC0+1FAB30, targeting switch_camera directly.
 * duration/time return original x87 values; duration is rounded before time.
 * after_animation is actual218C40. OnEnd must complete within4096 resumed
 * suffix calls (or fail explicitly); callbacks can run the existing coroutine.
 * Child removal is actual lookup followed by20A140(1), including a true miss.
 * restart includes55E50 service,4A840/4C150 restore and world flags|2.
 * finish_flags/handoff are the full114D80/114F20 owners; they are required,
 * not zero-effect notifications. Construction/destruction are separate.
 * Callback-visible FP exception flags are outside this finite domain. */
typedef struct UCSimpleCutsceneServices {
    int (*preflight)(void *);
    int (*triggers_update)(void *);
    int (*model_enable)(void *);
    int (*flags_ready)(void *,int32_t,const int32_t *,uint8_t *);
    void *(*provider)(void *);
    int (*provider_ready)(void *,void *,uint8_t *);
    int (*resolve_clip)(void *,void **);
    int (*start_animation)(void *,void *);
    int (*start_audio)(void *);
    int (*bind_trigger)(void *,uint8_t *);
    int (*audio_busy)(void *,uint8_t *);
    int (*remove)(void *);
    int (*switch_bone)(void *,uint32_t);
    double (*delta)(void *);
    int (*skip_input)(void *,double *);
    int (*animation_done)(void *,uint8_t *);
    int (*animation_duration)(void *,double *);
    int (*animation_time)(void *,double *);
    int (*fade_request)(void *,uint32_t,float,int32_t,uint8_t *,int32_t,uint8_t);
    int (*sound_position)(void *);
    int (*advance_animation)(void *,float);
    int (*after_animation)(void *);
    int (*set_flag)(void *,int32_t);
    int (*end_event)(void *,int32_t);
    int (*remove_camera)(void *,int32_t);
    int (*restart)(void *,int32_t);
    int (*finish_flags)(void *);
    int (*handoff)(void *);
} UCSimpleCutsceneServices;
int uc_simple_cutscene_update(const UCEntityFreshView *,UCSimpleCutscene *,
    const UCSimpleCutsceneServices *,void *);
#ifdef __cplusplus
}
#endif
#endif

#ifndef UC_NATIVE_AUDIO_INSTANCE_H
#define UC_NATIVE_AUDIO_INSTANCE_H
#include "audio_event.h"
#ifdef __cplusplus
extern "C" {
#endif
/* One intrusive graph. Addresses must remain stable while any link is live.
 * No hidden allocation or duplicated scheduler/resource/RNG authority. */
typedef struct UCAudioLink {struct UCAudioLink *next,*previous;} UCAudioLink;
typedef struct UCAudioInstance {
    UCAudioLink links[6]; /* original08,10,18,20,28(tracks),30(buffer) */
    uint32_t references;
    void *bank,*aux40,*buffer,*group,*aux4c;
    const uint8_t *sound;
    uint32_t event,index;
    int16_t volume,pitch,parameter60,fade;
    uint32_t flags;
    int32_t state;
    uint32_t word6c;
} UCAudioInstance;
typedef struct UCAudioInstanceTrack {
    UCAudioLink membership,commands; /* original14,1C */
    uint32_t references;
    UCAudioInstance *parent;
    void *aux28,*voice,*bank;
    const uint8_t *definition;
    void *aux40;
    UCAudioTrackState state; /* SAME fields used by qualified event/scheduler */
    uint16_t parameters[7]; /* original52,54,56,58,5A,5C,5E */
} UCAudioInstanceTrack;
/* Original249C30/24CF10 represented fields. Fresh/unpublished storage only;
 * class dispatch words and untouched track04/64 are not native state. */
int uc_audio_instance_begin(UCAudioInstance *);
int uc_audio_instance_track_begin(UCAudioInstanceTrack *);
/* Exact243F40/243F60 edits on admitted live, reciprocal links. append requires
 * an isolated node distinct from head. remove permits an isolated node. These
 * local checks do not validate arbitrary addresses or the whole borrowed ring. */
int uc_audio_link_append(UCAudioLink *head,UCAudioLink *node);
int uc_audio_link_remove(UCAudioLink *node);
/* Original24B050; definition is NULL or a live immutable >=5-byte record.
 * Does not append membership. Its bit0 only SETS state.flags0200. */
int uc_audio_instance_track_bind(UCAudioInstanceTrack *,UCAudioInstance *,
                               void *bank,const uint8_t *definition,size_t bytes);
/* Full248DD0. All int32 states are preserved, including out-of-range values.
 * previous is optional distinct storage, written only on acceptance. */
int uc_audio_instance_state(UCAudioInstance *,int32_t next,int32_t *previous,
                            uint8_t *accepted);
/* Actual248730/2487C0. Unsigned ref arithmetic wraps exactly. release can
 * destroy at count1 when flags mask1 is set, not just count0. destroy is the
 * required completing actual deleting destructor (may free instance); never
 * a playback stop substitute. Return status0/1; refs result remains valid even
 * after deletion and must not alias the instance. */
uint32_t uc_audio_instance_retain(UCAudioInstance *);
int uc_audio_instance_release(UCAudioInstance *,void (*destroy)(void *,UCAudioInstance *),
                             void *context,uint32_t *remaining);
typedef struct UCAudioInstanceBinding {
    void *context;
    int (*preflight)(void *,const UCAudioInstance *,const void *buffer); /* pure */
    void (*retain_buffer)(void *,void *);
    void (*clear_buffer)(void *,void *); /* completing245640(buffer,0,0) */
    UCAudioLink *(*buffer_members)(void *,void *); /* pure actual buffer+28 */
    void (*release_buffer)(void *,void *);
} UCAudioInstanceBinding;
/* Full2489F0 + actual245670 membership. All services required for nonnull new
 * buffer; preflight admits every completing effect and prospective current
 * buffer, which must stay nonnull and yield a live reciprocal ring distinct
 * from instance.links[5] when membership is needed. Other hooks may mutate
 * instance.buffer and scalar fields; preserve
 * instance and live reciprocal rings, no reentry or mutation of its membership
 * during retain_buffer/clear_buffer. The current buffer is reloaded after both
 * hooks. Already attached membership is deliberately not moved. Captured old
 * buffer is released last. This does not construct a buffer or start a voice.
 * Returns0 completed,1 rejected before effects. */
int uc_audio_instance_bind_buffer(UCAudioInstance *,void *buffer,
                                 const UCAudioInstanceBinding *);
/* Actual245670/2456A0 for valid live instance/list storage. detach unlinks
 * before release; its completing destructor may free the instance. */
int uc_audio_instance_attach(UCAudioLink *buffer_members,UCAudioInstance *);
int uc_audio_instance_detach(UCAudioInstance *,void (*destroy)(void *,UCAudioInstance *),
                            void *context,uint32_t *remaining);
#ifdef __cplusplus
}
#endif
#endif

#ifndef UC_NATIVE_AUDIO_EVENT_H
#define UC_NATIVE_AUDIO_EVENT_H
#include "audio_asset.h"
#ifdef __cplusplus
extern "C" {
#endif

/* AMPC's independent stream at4FEAC0. The image seed is1; never alias game RNG. */
typedef struct UCAudioRandom { uint32_t seed; } UCAudioRandom;
/* Scalar helpers require valid nonnull pointers. All borrowed arguments in
 * mutating APIs are distinct/stable; these APIs do not construct source owners. */
uint16_t uc_audio_random(UCAudioRandom *);
int16_t uc_audio_random_range(UCAudioRandom *,int16_t first,int16_t last);

typedef struct UCAudioCommand {
    uint32_t time;
    uint16_t flags,repeat,reference;
    int16_t pitch[2],volume[2];
    uint8_t opcode,bytes;
} UCAudioCommand;
/* Opcode1/3 direct commands: flags0/20, repeat1, ref!=FFFF,
 * also direct opcode3 repeat0 (voice looping remains the playback owner's job).
 * alternate soundFFFF. Also opcode3 sequential selector flags0104, index<FFFF
 * with upper index word0; reference then means selector-table index. Selection
 * is a required create_voice service, never silently treated as a direct GUID.
 * Parsing is bounded and failure preserves output. */
UCAudioStatus uc_audio_command_read(const void *,size_t,UCAudioCommand *);
typedef struct UCAudioTrackState {
    uint64_t time;
    uint32_t origin;
    uint16_t flags,offset,resume_offset,repeat;
    uint8_t index,resume_index;
    int16_t pitch,volume;
    const void *resource;
    int32_t subsound;
} UCAudioTrackState;
typedef struct UCAudioTrackCommands {
    const uint8_t *data; /* Actual commands, first at ampt payload+12. */
    size_t bytes;
    uint16_t count;
} UCAudioTrackCommands;
typedef void (*UCAudioEnqueue)(void *,UCAudioTrackState *,
                             const UCAudioCommand *,uint64_t when);
/* 24E010 ordinary tracks, <=255 commands, exact command-boundary resume.
 * No special timing track/parent timeline (2491A0 returnsNULL), loops, choices
 * or random delays. The command flags0104 selection service is opaque here;
 * it runs later at resource creation. All commands are validated before effects. Ticks remain
 * original abstract units; this does NOT assume milliseconds. Comparison is
 * unsigned and horizon is exclusive. Each call resets cursor from resume,
 * exactly as source; advancing resume is NOT silently invented here.
 *
 * enqueue is required, completing and pre-admitted for every request. It owns
 * real queue storage/dispatch. It may execute an event synchronously and change
 * flags/pitch/volume/resource, but cannot change command storage or cursors,
 * origin/resume/time, invalidate owners, or reenter this scheduler. No hidden
 * queue, allocation, clock, voice limit policy or fabricated successful play. */
UCAudioStatus uc_audio_track_schedule(UCAudioTrackState *,
    const UCAudioTrackCommands *,uint64_t now,uint32_t horizon,
    UCAudioEnqueue,void *);

typedef struct UCAudioEventServices {
    void *user;
    /* Pure admission of completing downstream owners, before RNG/effects. */
    int (*preflight)(void *,const UCAudioTrackState *,const UCAudioCommand *);
    int (*has_voice)(void *); /* Pure current voice ownership query. */
    void (*pitch_changed)(void *,UCAudioTrackState *); /* 2495C0 */
    void (*volume_changed)(void *,UCAudioTrackState *); /* 248C50 */
    void (*create_voice)(void *,UCAudioTrackState *,const UCAudioCommand *); /*249CC0*/
    void (*start_voice)(void *); /*245430*/
} UCAudioEventServices;
/* 241610 opcode1/3 branch, ordinary parent sound instance present, flags0200
 * and2000 clear. Services implement actual voice ownership; creation may leave
 * no voice, as source allows. Calls complete synchronously and may consume the
 * SAME audio RNG, update live flags/parameters, but must keep flag2000 clear and
 * preserve any initially captured voice identity through start. They cannot
 * invalidate arguments or reenter this owner. preflight==0 rejects before effects. Does not allocate
 * or fake a command-node destructor; caller owns the queued command lifetime. */
UCAudioStatus uc_audio_event_execute(UCAudioTrackState *,const UCAudioCommand *,
    UCAudioRandom *,const UCAudioEventServices *);
/* 24D000 direct resident-WAVE path. resolve is the actual GUID/cache owner and
 * returns a live, ready resident WAVE (virtual+34 data!=NULL) orNULL. Resource
 * type/interface/readiness queries are pure; streams,
 * subbanks, selectors and mix-provider attachment are outside this domain. */
UCAudioStatus uc_audio_event_resolve(UCAudioTrackState *,const UCAudioCommand *,
    uint32_t reference_count,const void *(*resolve)(void *,uint16_t),void *);

/* Stable snapshot of248C50/2495C0 live inputs, voice present. The caller must
 * obtain these from actual owners, not replace group/fade state with zero.
 * group values are actual247CC0/247CF0 results; missing group contributes0.
 * Stock gain bounds are4839B0=-10000,4839B4=0. No gain/pitch unit conversion. */
typedef struct UCAudioMix {
    uint16_t sound_gain;
    int16_t sound_pitch,instance_gain,fade_gain,group_gain,instance_pitch,group_pitch;
    int16_t track_gain,track_pitch,sample_pitch;
    uint8_t mute,parent_timing_track;
} UCAudioMix;
int16_t uc_audio_event_gain(const UCAudioMix *,const UCAudioTrackState *);
int16_t uc_audio_event_pitch(const UCAudioMix *,const UCAudioTrackState *);
#ifdef __cplusplus
}
#endif
#endif

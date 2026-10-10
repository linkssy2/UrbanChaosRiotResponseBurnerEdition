#ifndef UC_NATIVE_AUDIO_RESIDENT_VOICE_H
#define UC_NATIVE_AUDIO_RESIDENT_VOICE_H
#include "audio_device_buffer.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Borrowed projection of the SAME immutable, resident WAVE interface returned
 * by24D000/QueryInterface(4FED88). These are original encoded byte units, NOT
 * decoded PCM frame units. NULL data/format retain source failure behavior. */
typedef struct UCAudioResidentWave {
    const void *data;uint32_t bytes;
    const UCAudioDeviceFormat *format;
    uint32_t play_start,play_length,loop_start,loop_length,positions[2];
} UCAudioResidentWave;
typedef struct UCAudioResidentEffects {
    void *context;
    /* Pure;0 admits completing effects, every prospective captured/current
     * voice/device and all immutable WAVE/format storage before any effects. */
    int (*preflight)(void *,const UCAudioInstance *,const UCAudioInstanceTrack *,const UCAudioCommand *);
    const UCAudioResidentWave *(*resource)(void *,UCAudioInstanceTrack *,const UCAudioCommand *); /*24D000*/
    UCAudioBuffer *(*voice)(void *,UCAudioInstance *,UCAudioInstanceTrack *); /*248A60(_,_,0)*/
    void (*auxiliary_gain)(void *,UCAudioDeviceBuffer *,int16_t); /*2523A0*/
    void (*sound_parameter)(void *,UCAudioDeviceBuffer *,uint32_t); /*251860*/
    int16_t (*sample_pitch)(void *,UCAudioInstance *,uint32_t); /*248BF0*/
    void (*route)(void *,UCAudioInstance *,UCAudioInstanceTrack *,UCAudioBuffer *,const UCAudioDeviceFormat *); /*248B20*/
    void (*pitch)(void *,UCAudioInstance *,UCAudioInstanceTrack *); /*2495C0*/
    void (*gain)(void *,UCAudioInstance *,UCAudioInstanceTrack *); /*248C50*/
    void (*filter)(void *,UCAudioInstance *,UCAudioInstanceTrack *,uint8_t); /*248B80*/
    const UCAudioDeviceEffects *device;
} UCAudioResidentEffects;
/* Full249CC0 ordinary AMPC→resident WAVE branch, on the existing graph.
 * instance.bank remains a valid AMPC; definition is NULL or>=6 immutable bytes;
 * sound is NULL or>=22 bytes. The resource service owns actual selector/RNG and
 * track.resource/subsound publication, and voice owns actual allocation/graph.
 * Neither service is a fabricated successful substitute. Other required hooks
 * complete original routing/mix effects; there is no playback/start here.
 *
 * Captured WAVE/format/voice/device identities remain alive and immutable except
 * device.flags and source-owned mix fields. Ordinary devices have no stream or
 * helper. Completing hooks may alter live instance.buffer/sound/flags,
 * voice.device/word48, command.repeat and track.flags, preserving valid owners;
 * source reads/captured identities are retained exactly. No reentry/owner frees,
 * bank/type/dispatch mutation, WAVE getter side effects or FP-mode changes.
 * Source2519A0 with helperNULL and actual BD660 are concrete quiet branches.
 *
 * 0 completed (bound is the original AL Boolean, including quiet true/false),
 * 1 rejected before effects. Inputs and bound are distinct stable storage.
 * Preflight admits every nested device operation; callback contract violation
 * is not a recoverable source path. NULL resource/voice/device/data/format is a
 * source-defined false result and retains already-performed effects. */
int uc_audio_resident_voice(UCAudioInstance *,UCAudioInstanceTrack *,UCAudioCommand *,
                           const UCAudioResidentEffects *,uint8_t *bound);
#ifdef __cplusplus
}
#endif
#endif

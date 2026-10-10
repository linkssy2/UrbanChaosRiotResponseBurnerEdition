#ifndef UC_NATIVE_AUDIO_DEVICE_BUFFER_H
#define UC_NATIVE_AUDIO_DEVICE_BUFFER_H
#include "audio_voice_create.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCAudioDeviceProvider {
    UCAudioLink devices; /* actual provider+4, one authoritative device ring */
    void *backend;       /* actual provider+10 backend, not a playback claim */
} UCAudioDeviceProvider;
typedef struct UCAudioDeviceBuffer {
    UCAudioLink membership,children; /* actual+8,+10 */
    uint32_t references,flags;
    void *buffer,*stream,*stream_helper;
    UCAudioDeviceProvider *provider;
    void *spatial; /* actual+30, borrowed by252BC0; not a truncated guest pointer */
    uint32_t channel_mask;int32_t gain;
    uint32_t values[4];
} UCAudioDeviceBuffer;
typedef struct UCAudioDeviceFormat {
    uint16_t tag,channels;uint32_t rate,bytes_per_second;
    uint16_t alignment,bits,extra;
} UCAudioDeviceFormat;
typedef struct UCAudioDeviceEffects {
    void *context;
    /* Pure admission of every completing backend effect and prospective live
     * voice.device if callbacks change it. No allocation/queue/RNG effects. */
    int (*preflight)(void *,const UCAudioBuffer *);
    UCAudioDeviceBuffer *(*allocate)(void *);
    void *(*create)(void *,void *backend,const UCAudioDeviceFormat *);
    void (*output)(void *,void *buffer); /* actual425F23(buffer,NULL) */
    void (*stop)(void *,void *buffer);   /* actual424FC5 */
    void (*filter)(void *,void *buffer,const uint32_t coefficients[6]);
    void (*format)(void *,void *buffer,const UCAudioDeviceFormat *);
    void (*data)(void *,void *buffer,const void *,uint32_t);
    void (*play_region)(void *,void *buffer,uint32_t start,uint32_t length);
} UCAudioDeviceEffects;
/* Full243970 type8→252A90/252100 ordinary buffer: flags2/102/202/302,
 * default48kHz16-bit stereo. SDK descriptor's other five scalar/pointer fields
 * are zero. A successful typed allocator is admitted; NULL gives committed
 * status2, outside source's subsequent null dereference. Backend createNULL IS
 * source-defined: the wrapper still joins provider.devices and is returned.
 * Status0 completed,1 pure rejection,2 unsupported committed allocationfailure.
 * Out is distinct live storage. Caller owns real final release/destruction. */
int uc_audio_device_create(UCAudioDeviceProvider *,uint32_t,
                          const UCAudioDeviceEffects *,UCAudioDeviceBuffer **out);
/* Actual246300,251890(NULL),251770,251D80(8000,0),245470(property1).
 * Only ordinary buffer wrappers (stream/helperNULL), actual246FC0 class.
 * Retains ring ordering including remove/reappend of an existing last member.
 * The real property1 setter does nothing; this is verified, not supplied success.
 * Completing backend callbacks may change voice.device/flags, preserving every
 * referenced wrapper/provider/reciprocal ring. They preserve wrapper fields and
 * provider membership; the same current device is reloaded at source frontiers.
 * Filter packet is exactly[1,0,0,32768,0,32768] for the source48kHz defaults.
 * Source hardware failures remain backend-owned; no silent replacement/start. */
int uc_audio_device_initialize(UCAudioBuffer *,const UCAudioDeviceEffects *);
/* Full251810/2517A0/251960 ordinary resident setters. Format/data metadata and
 * PCM remain borrowed through the required completing backend operations;
 * backend must retain ownership through playback. Flags are only ORed, never
 * reset when a later format becomes mono or a later data request is empty.
 * NULL format/data,zero bytecount and absent backend follow actual quiet paths.
 * No stream helper, loop scheduling, PCM decoding, voice start or SDK emulation. */
int uc_audio_device_format(UCAudioDeviceBuffer *,const UCAudioDeviceFormat *,const UCAudioDeviceEffects *);
int uc_audio_device_data(UCAudioDeviceBuffer *,const void *,uint32_t,const UCAudioDeviceEffects *);
int uc_audio_device_play_region(UCAudioDeviceBuffer *,uint32_t,uint32_t,const UCAudioDeviceEffects *);
#ifdef __cplusplus
}
#endif
#endif

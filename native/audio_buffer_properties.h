#ifndef UC_NATIVE_AUDIO_BUFFER_PROPERTIES_H
#define UC_NATIVE_AUDIO_BUFFER_PROPERTIES_H
#include "audio_device_buffer.h"
#ifdef __cplusplus
extern "C" {
#endif
/* One actual245100 payload owned by UCAudioBuffer.spatial and borrowed by
 * UCAudioDeviceBuffer.spatial. These are native fields, not Xbox offsets. */
typedef struct UCAudioSpatial {
    void *owner0,*owner4; UCAudioLink membership;
    uint32_t mode,flags;
    float range[2],position[3],velocity[3];
    uint32_t word38,word3c; float value40; uint32_t word44;
    float value48,value4c;
} UCAudioSpatial;
typedef struct UCAudioBufferProperties {
    void *context;
    int (*preflight)(void *,const UCAudioBuffer *,uint32_t,const void *); /* pure0admit */
    int (*device_parameter)(void *,UCAudioDeviceBuffer *,uint32_t,const void *);
} UCAudioBufferProperties;
/* Whole245470/2455C0 secondary interface. handled is the actual AL boolean,
 * separate from status0complete/1pre-effect rejection/2committed service error.
 * Value has the source property width and is disjoint from owners/handled;
 * getter output is disjoint too. Unknown properties require no value. Getter1
 * genuinely succeeds without touching output. Setter7 takes input parity,
 * not nonzero truth; setter9/getter9 transfer only two bytes.
 * Present device effects are required and run AFTER the same spatial write.
 * Hooks admit lifetimes and preserve inputs through completion. They may alter
 * current owner fields; no copied device/spatial authority is introduced. */
int uc_audio_buffer_set(UCAudioBuffer *,uint32_t,const void *,const UCAudioBufferProperties *,int *handled);
int uc_audio_buffer_get(const UCAudioBuffer *,uint32_t,void *,int *handled);

enum UCAudioSpatialOperation {UC_AUDIO_SPATIAL_MODE,UC_AUDIO_SPATIAL_MINIMUM,
    UC_AUDIO_SPATIAL_MAXIMUM,UC_AUDIO_SPATIAL_CURVE,UC_AUDIO_SPATIAL_POSITION,
    UC_AUDIO_SPATIAL_VELOCITY};
typedef struct UCAudioSpatialEffects {
    void *context;
    int (*preflight)(void *,const UCAudioDeviceBuffer *,uint32_t,const void *);
    /* Completing actual deferred backend call. stream selects source API;
     * value is u32(mode), float(distance), float[2](curve), float[3](vector).
     * Callback may change device.buffer/stream or input values, preserving all
     * pointed lifetimes. Each source reload occurs at its actual frontier. */
    void (*send)(void *,void *backend,unsigned stream,unsigned operation,const void *value);
} UCAudioSpatialEffects;
/* Whole251C70/251B80/251CB0/251D10 for properties3..6. Devices lacking flag4
 * are actual quiet source branches. NaNs suppress range/vector backend calls;
 * infinities are retained. Range copies and clamps before effects. The original
 * stream-range branch really calls BUFFER maximum for its third operation.
 * Default FP mode, no callback-visible FP exceptions. No backend/mixer provided
 * here and no audio success fabricated. Status0complete/1pre-effect rejection. */
int uc_audio_device_spatial(UCAudioDeviceBuffer *,uint32_t,const void *,const UCAudioSpatialEffects *);
#ifdef __cplusplus
}
#endif
#endif

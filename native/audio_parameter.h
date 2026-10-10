#ifndef UC_AUDIO_PARAMETER_H
#define UC_AUDIO_PARAMETER_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCAudioParameterCurve {
 const uint8_t *mapping,*points;uint16_t count;
} UCAudioParameterCurve;
typedef struct UCAudioParameterResult {uint16_t track,property;int16_t value;} UCAudioParameterResult;
/* Real AMPC LISTampp: one requested named sound-parameter with one mapping.
 * Missing name returns0/found0; malformed/unsupported requested program fails.
 * Borrowed immutable descriptors/points remain alive through evaluation. */
int uc_audio_parameter_find(const void*,size_t,const void*names,size_t names_size,
 uint16_t sound,const char*name,UCAudioParameterCurve*,int*found);
/* Complete249960 arithmetic for real parameters0gain,1pitch,2filter. Caller
 * owns actual selected track. Source count*increment+value,clamp,curve sample;
 * published value is the signed16 bit pattern, with float/truncation order. */
int uc_audio_parameter_curve(const UCAudioParameterCurve*,uint32_t count,
 float increment,float value,UCAudioParameterResult*);
#ifdef __cplusplus
}
#endif
#endif

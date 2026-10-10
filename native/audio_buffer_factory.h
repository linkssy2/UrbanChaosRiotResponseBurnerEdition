#ifndef UC_NATIVE_AUDIO_BUFFER_FACTORY_H
#define UC_NATIVE_AUDIO_BUFFER_FACTORY_H
#include "audio_buffer_properties.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Borrow actual group identity/+20 reference count/+70 priority. No second
 * group state. These fields and their owner remain alive through the buffer. */
typedef struct UCAudioBufferGroup {
    void *identity; uint32_t *references; const uint16_t *priority;
} UCAudioBufferGroup;
typedef struct UCAudioBufferDescriptor {
    uint32_t flags; uint16_t channels,priority;
} UCAudioBufferDescriptor;
typedef struct UCAudioBufferFactory {
    void *context;
    int (*preflight)(void *,const UCAudioBufferGroup *,const UCAudioBufferDescriptor *);
    UCAudioBuffer *(*allocate_buffer)(void *);
    UCAudioSpatial *(*allocate_spatial)(void *);
    void *(*current)(void *); /* actual241880(0), reloaded at each frontier */
    void *(*listener)(void *,void *engine); /* interface3; NULL allowed */
    double (*scale)(void *,void *listener);
    /* Required completing current engine+0C/type8 and2406E0. These do not
     * fabricate a backend. The former sees exact device flags; channels are
     * already published in the SAME buffer. NULL results are source-defined. */
    void *(*device)(void *,void *engine,uint32_t flags);
    void *(*steal)(void *,void *engine,uint32_t flag8,uint16_t priority);
    int (*initialize)(void *,UCAudioBuffer *); /* actual246300 if device present */
} UCAudioBufferFactory;
/* Complete248470 TYPE6 ->240D80/246FC0/2452C0/245100/245340/2456F0.
 * Fresh buffers only: no old owner34/owner38. Existing spatial high flag bits
 * come from allocated storage; constructor clears only low two bits. Real
 * allocations remain caller-owned; this function never clones storage or
 * silently frees a committed prefix. Source allocationNULL is preserved.
 * Callback views may change descriptor fields, spatial range before scaling,
 * current engine and group reference/priority fields. They preserve lifetimes,
 * fresh buffer ownership, hook/group descriptors and the default FP mode.
 * Allocation/initialize callbacks may change flags/device; prospective device
 * hooks must be preadmitted. No reentry, aliasing output, or invalid current
 * engine. Scale finite positive1e-20..1e20; callback FP status is not modeled.
 * Returns0 complete (including NULL buffer),1 pre-effect rejection,2 committed
 * dependency failure. On2 caller retains allocated prefix; output unchanged.
 * Only type6 dispatch is represented, not group/type5/instance/type7 factories,
 * source/device teardown or audible playback. */
int uc_audio_buffer_factory_create(const UCAudioBufferGroup *,
    const UCAudioBufferDescriptor *,const UCAudioBufferFactory *,UCAudioBuffer **);
#ifdef __cplusplus
}
#endif
#endif

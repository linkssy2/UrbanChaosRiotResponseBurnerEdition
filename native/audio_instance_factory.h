#ifndef UC_NATIVE_AUDIO_INSTANCE_FACTORY_H
#define UC_NATIVE_AUDIO_INSTANCE_FACTORY_H
#include "audio_buffer_factory.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCAudioInstanceFactory {
    void *context;
    int (*preflight)(void *,const UCAudioBufferGroup *); /* pure;0 admits */
    void *(*current)(void *); /* pure actual241880(0), nonnull engine */
    UCAudioInstance *(*allocate)(void *); /* actual type7 allocation; NULL allowed */
} UCAudioInstanceFactory;
/* Full245B80→240D80 TYPE7→249C30→2489C0/23F950. Uses the SAME instance
 * and actual group's borrowed identity/reference counter; priority is unused.
 * NULL group is the source quiet branch. The current engine's +0C dispatch is
 * admitted to the qualified type7 factory, not an arbitrary virtual factory.
 * Allocation provides fresh, unpublished native instance storage. Allocation
 * may mutate external state and the group's counter, but
 * preserve group identity/counter address, all descriptor storage and output
 * lifetime. No reentry. Constructor writes precede group publication/retain;
 * unsigned reference overflow is preserved. Allocation failure completes with
 * NULL; no group retain or rollback. Output is distinct from borrowed fields.
 * Returns0 completed,1 pre-effect rejection (including missing engine). No start, instance/group destructor or playback claim. */
int uc_audio_instance_factory_create(const UCAudioBufferGroup *,
    const UCAudioInstanceFactory *,UCAudioInstance **);
#ifdef __cplusplus
}
#endif
#endif

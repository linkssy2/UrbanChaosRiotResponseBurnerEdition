#ifndef UC_GLASS_ENTITY_UPDATE_H
#define UC_GLASS_ENTITY_UPDATE_H
#include "glass_entity.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCGlassEntityUpdate {
    UCGlassEntity *entity;
    const UCGlassEntityProperties *properties;
    const float *world;
    uint8_t *did_break, *forced_break; /* actual entity E9/EA, not copies */
} UCGlassEntityUpdate;
typedef struct UCGlassEntityAudio {
    void *context;
    int (*preflight)(void *,void *bank,const char *event,uint32_t argument,float volume);
    int (*play)(void *,void *bank,const char *event,uint32_t argument,float volume);
} UCGlassEntityAudio;
/* Actual189FB0 ->189DC0 ordinary bullet-break continuation. Uses the SAME
 * existing container and effect state. This admits NPCSmash=false, OnBroken
 * group=-1, forced EA=0, nonpositive rounded debris duration and finite world
 * normal Y within[-.5,.5]: the actual six Harbor panes satisfy these branches.
 * No replacement success for NPCs, scripted events, delay/yield or broadcasts.
 * Source216C10's NULL bank returns quietly. Otherwise audio is a REQUIRED real
 * actor endpoint, including lazy helper construction when absent; preflight is
 * pure and promises play completes. A recorded test boundary is not playback.
 * All borrowed fields remain live/disjoint, graph and parameters immutable
 * through audio except entity.state118 may change: source reloads it AFTER
 * the sound request before deciding whether to publish1. No reentry, freeing
 * or aliasing output slots; stable RN/gradual-underflow floating environment.
 * E9 initialization, entity/coroutine scheduling, actor late update and class
 * lifetime remain their real owners, not implemented by this continuation.
 *0complete,1pre-effect rejection,2committed audio failure; never retry2.
 * A nonzero E9 remains unchanged; its original outer state getter is pure. */
int uc_glass_entity_update(const UCGlassEntityUpdate *,const UCGlassEntityAudio *);
#ifdef __cplusplus
}
#endif
#endif

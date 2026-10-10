#ifndef UC_NATIVE_COLLISION_AGENT_UPDATE_H
#define UC_NATIVE_COLLISION_AGENT_UPDATE_H
#include "collision_agent_store.h"
typedef void (*UCCollisionAgentUpdateCallback)(void *context,UCCollisionAgent *agent,
    void *state,UCCollisionAgentBody *first,UCCollisionAgentBody *second,const void *input);
/* Source3294F0 dispatch on the SAME agent record used by contact storage.
 * Tags2/6 use inline payload after the native header; tag4 skips16 further
 * payload bytes. Other tags are actual no-ops and do not inspect input/table.
 * The caller supplies the effective registered callback table (0..256 slots),
 * not constructor defaults. A present NULL slot is an actual no-op; a missing
 * slot is unsupported. Entry allocation must contain entry_size readable bytes;
 * the selected callback owns its real state/effects and payload-size contract.
 * Table, context, input and both collidables are borrowed for the call. Callback
 * may mutate its actual agent/payload; no post-callback agent reads, allocation,
 * retention, storage movement or contact effects are supplied by this helper.
 * Invalid input preserves state and invokes no callback. No concurrent access. */
#ifdef __cplusplus
extern "C" {
#endif
UCCollisionStatus uc_collision_agent_update(UCCollisionAgent *,uint32_t entry_size,
    const UCCollisionAgentUpdateCallback *callbacks,uint32_t callback_count,
    void *context,const void *input);
#ifdef __cplusplus
}
#endif
#endif

#ifndef UC_NATIVE_COLLISION_AGENT_REMOVE_H
#define UC_NATIVE_COLLISION_AGENT_REMOVE_H
#include "collision_agent_store.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct UCCollisionAgentIsland {
    UCCollisionAgentStore *store;
    const void *world;
    uint8_t *dirty;
} UCCollisionAgentIsland;
typedef struct UCCollisionAgentRemoveBody {
    UCCollisionAgentIsland *island;
    uint8_t fixed;
} UCCollisionAgentRemoveBody;
typedef struct UCCollisionAgentRemoveHooks {
    UCCollisionAgentRemoveBody (*body)(void *context,const UCCollisionAgentBody *);
    const void *(*dispatcher)(void *context,const void *world);
    UCCollisionStatus (*preflight)(void *context,const void *dispatcher,void *manager);
    void *(*allocate)(void *context,size_t bytes,uint32_t memory_class);
    void (*release)(void *context,void *memory,size_t bytes,uint32_t memory_class);
    void (*destroy)(void *context,const void *dispatcher,UCCollisionAgent *,void *state,void *manager);
    void (*finish)(void *context,void *manager);
} UCCollisionAgentRemoveHooks;

/* Pure2FDFB0 ownership search. Compare BLOCK counts, scan first on ties using
 * full allocated half-open block ranges (including unused last-block bytes).
 * On miss choose the opposite island without scanning/verifying that island.
 * This is source selection, not proof of live agent membership. uintptr_t
 * ranges must not wrap; source guest-wrap cases are outside native admission.
 * No storage mutation; out/input must be disjoint and remain alive. */
UCCollisionStatus uc_collision_agent_remove_owner(const void *address,
    UCCollisionAgentIsland *first,UCCollisionAgentIsland *second,
    UCCollisionAgentIsland **out);

/* Outer2FE190 using the SAME actual collision_agent_store. Island/body/world
 * descriptors, sides and manager are borrowed through their last use. Pure
 * body/dispatcher getters expose current owners, not cloned collision graphs.
 * Same island marks it dirty regardless of fixed flags. Different islands:
 * first fixed selects second; else second fixed selects first; neither marks
 * dirty. Two dynamic sides use2FDFB0, dirty first then reload/dirty second island.
 * Selected island's world supplies dispatcher. Manager is captured BEFORE
 * actual inner removal/relocation; finish uses that manager AFTER inner success.
 * Finish must resolve its current teardown method then (source virtual+18).
 * Required preflight is pure and admits completing registered destroy/finish
 * effects plus allocator headroom. No fabricated success for missing effects.
 *
 * Native validation proves selected agent residence/backlinks before dirty
 * writes; pure owner misses remain permitted only in the standalone selector.
 * Existing store admission forbids live links spanning multiple stores: this
 * bounded outer slice therefore does not claim arbitrary island/contact worlds.
 * Metadata/getter outputs and both island descriptors stay stable except dirty
 * bytes, which cannot alias metadata. Store hooks retain their existing contract.
 * No concurrent mutation; during inner storage work its hooks may observe state
 * and change permitted inline/manager state only, without reentrant ownership
 * mutation. Manager must survive inner callbacks. After inner completion,
 * finish may enqueue deferred world work, change surviving bodies' current
 * island, and destroy the manager. It must not access a stale agent. No body,
 * island, store or manager is read by this controller after finish returns.
 * All guards/preflight failures preserve applied and dirty/storage state.
 * applied=1 before first dirty/inner effect. Any inner error leaves completed
 * effects intact, skips finish and forbids whole-call retry; no rollback.
 * Does not construct worlds/islands/agents, merge islands, or dirty the calling
 * rigid controller's later CURRENT island (that is its separate source effect).
 */
UCCollisionStatus uc_collision_agent_remove(UCCollisionAgent *,
    const UCCollisionAgentRemoveHooks *,void *context,int *applied);
#ifdef __cplusplus
}
#endif
#endif

#ifndef UC_NATIVE_COLLISION_AGENT_STORE_H
#define UC_NATIVE_COLLISION_AGENT_STORE_H
#include "collision_asset.h"

typedef struct UCCollisionAgent UCCollisionAgent;
typedef struct UCCollisionAgentBody UCCollisionAgentBody;
typedef struct UCCollisionAgentLink {
    UCCollisionAgent *agent;
    UCCollisionAgentBody *other;
} UCCollisionAgentLink;
struct UCCollisionAgentBody {
    const void *identity;
    UCCollisionAgentLink *data;
    uint32_t count, capacity_flags;
};
struct UCCollisionAgent {
    uint8_t tag, type, opaque_02[10];
    uint16_t index[2];
    void *manager;
    UCCollisionAgentBody *side[2];
    uint8_t opaque_1c[4];
};
typedef struct UCCollisionAgentStore {
    unsigned char **blocks;
    uint32_t count, capacity_flags;
    unsigned char *inline_block;
    uint32_t used_bytes;
    uint16_t entry_size, block_size;
    uint32_t active;
} UCCollisionAgentStore;
typedef struct UCCollisionAgentStoreHooks {
    void *(*allocate)(void *context, size_t bytes, uint32_t memory_class);
    void (*release)(void *context, void *memory, size_t bytes, uint32_t memory_class);
    void (*destroy)(void *context, UCCollisionAgent *agent, void *state, void *manager);
} UCCollisionAgentStoreHooks;

/* 32A340 +329870 storage removal, including 2E01D0 shrink policy.
 * This mutates the ACTUAL single block stream and two collidable link arrays;
 * it does not construct an agent, choose an island, merge islands or implement
 * 2FE190's final contact-manager virtual+18. Those outer effects remain required.
 * Store/body descriptors and identities are borrowed and remain alive. Their
 * backing arrays/blocks belong to this graph and use the supplied allocator.
 * Do not clone or attach a second authoritative store to the same agents.
 * Full blocks precede a partially/full used final block. Empty store used_bytes
 * equals block_size, including after final release. Entry/block sizes are native
 * byte sizes, pointer-aligned multiples; block_size is a multiple of entry_size.
 * Header is a typed projection of guest bytes00..1F; opaque inline state follows
 * sizeof(UCCollisionAgent), with tag4's state another16 bytes into that payload.
 * The complete native entry is copied on relocation, including opaque bytes.
 * No external pointer to a movable agent remains stable after removal; use the
 * repaired body lists. Dead tails retain bits, never live references.
 *
 * Domain: <=65535 live agents, block/list capacities<=65535, distinct collidable
 * sides and block allocations, valid live backlinks, no cross-store live links.
 * All buffers, output and descriptors are disjoint except the explicit embedded
 * one-block array. Caller owns synchronization and allocator provenance.
 * Validator checks every resident agent's backlinks and each referenced body's
 * live tail before it could be moved. It does not discover arbitrary extra
 * non-tail links to foreign stores; no-cross-store remains an admission contract.
 * Required hooks are non-reentrant and completing. Destroy can change inline
 * payload/manager-owned effects only; no graph/identity/header changes. Allocator
 * callbacks may observe intermediate changes but must not mutate the graph.
 * Native active guards same-store reentry; no concurrent/cross-store mutation.
 *
 * Entry validation errors leave storage/output unchanged. *applied becomes1
 * before the first callback/edit. A NULL allocation returns MEMORY after partial
 * effects (including an already unlinked side); forbidden destroy-header edits
 * similarly return ARGUMENT after callback effects. No rollback/resume/retry claim.
 * Completing world services must pre-admit allocations and callback lifetimes.
 * Successful removal preserves callback order, tail swaps/back-index repairs,
 * stream relocation/back-pointer repairs, shrink flags and released blocks.
 */
#ifdef __cplusplus
extern "C" {
#endif
UCCollisionStatus uc_collision_agent_store_validate(const UCCollisionAgentStore *);
UCCollisionStatus uc_collision_agent_store_remove(UCCollisionAgentStore *,
    UCCollisionAgent *, const UCCollisionAgentStoreHooks *, void *context, int *applied);
#ifdef __cplusplus
}
#endif
#endif

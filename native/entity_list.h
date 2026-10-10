#ifndef UC_ENTITY_LIST_H
#define UC_ENTITY_LIST_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCEntityList UCEntityList;
typedef struct UCEntityListNode {
    void *target;
    struct UCEntityListNode *next,*previous;
} UCEntityListNode;
typedef struct UCEntityListView {
    uint32_t count,capacity;
    const UCEntityListNode *nodes,*free,*head,*source_cursor;
    uint32_t source_word;
} UCEntityListView;
/* Actual221960/2219E0/221A10 node effects. The owner allocates one fixed node
 * array; requested capacity0 becomes1. Native admission: capacity<=65536,
 * successful host allocations, exclusive access, no concurrent mutation.
 * NULL return means rejected capacity/allocation failure before publication.
 * Representation omits the source vtable, uses native pointers, and retains
 * source+10/+18 initialized to0. Only constructor/prepend/remove reachable
 * graphs are admitted; external writers of those fields are not reconstructed.
 * All payloads, including NULL and duplicates, are borrowed; the list never
 * retains, dereferences or frees them. Their necessary lifetime is caller-owned.
 * Nodes remain allocated until destroy, but any mutation can change their
 * membership/meaning. Readonly views borrow actual storage, never a copy owner;
 * do not edit nodes. Capture next BEFORE removing current, as the source does.
 * No growth, callbacks, implied entity destruction or slot invalidation. */
UCEntityList *uc_entity_list_create(uint32_t capacity);
void uc_entity_list_destroy(UCEntityList *); /* NULL safe; may be nonempty. */
UCEntityListView uc_entity_list_view(const UCEntityList *); /* NULL: zero view. */
/* Return1 inserted/removed,0 full/miss,-1 NULL owner. Full/miss are unchanged.
 * Removal finds the FIRST forward pointer match, retains its stale target,
 * clears previous and pushes the node onto the free chain. Native remove's
 * result is an observation; original221A10 does not specify a return value. */
int uc_entity_list_prepend(UCEntityList *,void *target);
int uc_entity_list_remove_first(UCEntityList *,const void *target);
#ifdef __cplusplus
}
#endif
#endif

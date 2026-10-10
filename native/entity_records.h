#ifndef UC_ENTITY_RECORDS_H
#define UC_ENTITY_RECORDS_H
#include "entity_list.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCEntityRecords UCEntityRecords;
typedef struct UCEntityRecordBucket {
    UCEntityListNode *head;
    struct UCEntityRecordBucket *next,*previous;
    uint32_t index;
} UCEntityRecordBucket;
typedef struct UCEntityRecordsView {
    uint32_t capacity,bucket_count,source_count,source_word,first_index;
    const UCEntityListNode *nodes,*free,*source_head,*source_cursor;
    const UCEntityRecordBucket *buckets,*first;
} UCEntityRecordsView;
/* Actual221AE0/221BA0/221CA0/221D00 bucketed fixed pool. One authoritative
 * node array plus bucket array, independent of the live/pending entity lists.
 * Node target is a borrowed opaque record pointer, including NULL/duplicates;
 * no payload is retained, read or freed. Caller owns payload/object lifetimes.
 * Native admission: nodes1..65536, requested buckets0..65536; buckets0 becomes1
 * then clamps to node capacity. Failed host allocation returnsNULL before
 * publication. Source signed-negative/wrapping allocations are not represented.
 * Only constructor/mutator-reachable graphs, exclusive nonreentrant use.
 * Views borrow actual storage; never edit nodes/buckets. Mutations may recycle
 * their meaning but addresses remain allocated until destroy. Original stale
 * links/values and source+4/+10/+18 fields are preserved, not normalized.
 * No vtable/layout equivalence, record allocation, entity ID or world owner. */
UCEntityRecords *uc_entity_records_create(uint32_t nodes,uint32_t buckets);
void uc_entity_records_destroy(UCEntityRecords *); /* payloads remain alive */
UCEntityRecordsView uc_entity_records_view(const UCEntityRecords *);
/* Pure bounded admission for consumers. Requires finite bucket/node traversal;
 * require_nonempty also rejects any reachable bucket with a NULL head, as needed
 * by original207E50/207F70. Empty collection is valid. No graph normalization.
 * Source removal/reuse can produce empty reachable buckets and then cycles;
 * callers must not invoke searching mutators on a nonterminating graph. */
int uc_entity_records_traversable(const UCEntityRecords *,int require_nonempty);
/*1 success,0 sourcefalse,-1 NULL owner. Priority clamps signed to bucket range.
 * AFTER consumes a free node BEFORE searching for the parent value; missing
 * parent therefore returns0 WITH committed pool consumption/stale node links.
 * There is no invented rollback. Full PRIORITY or AFTER changes nothing. */
int uc_entity_records_insert_priority(UCEntityRecords *,void *value,int32_t priority);
int uc_entity_records_insert_after(UCEntityRecords *,const void *parent,void *value);
/* First pointer match in source bucket/forward-node traversal.
 * Return1 removed,0 miss,-1 NULL. Original removal has no specified result. */
int uc_entity_records_remove_first(UCEntityRecords *,const void *value);
#ifdef __cplusplus
}
#endif
#endif

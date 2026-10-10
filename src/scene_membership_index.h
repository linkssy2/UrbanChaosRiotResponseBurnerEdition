#ifndef UCRR_SCENE_MEMBERSHIP_INDEX_H
#define UCRR_SCENE_MEMBERSHIP_INDEX_H

#ifndef UCRR_SCENE_INDEX_OFFLINE_FIXTURE
#error This scene index is an offline candidate; production mutation/state contracts remain unproved.
#endif

/* OFFLINE CANDIDATE: no production caller is authorized by this header.
 *
 * Index a caller-owned immutable projection of eligible guest records. A known
 * predicate means the active dispatch target, including overrides, is exactly
 * B99E0. Its keys must be a validated unsigned, strictly ordered tree snapshot.
 * Unknown predicates are returned in record order for normal live dispatch.
 *
 * The caller MUST invalidate before any callback or possible mutation of tree
 * contents/links, records/order/masks, predicate dispatch or object lifetime.
 * Missing a notification is unsafe: this helper cannot observe guest writes.
 * A frame number, tree root/count or object+10C is NOT an adequate revision.
 * No complete production notification coverage has been established.
 *
 * This is only candidate selection. It does not reproduce skipped guest RAM,
 * registers, stack/trace writes, iterator state or context+8. Those contracts
 * must be proven before this can replace any part of the real scene traversal.
 * Storage and index are single-owner; no concurrent access is supported.
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct SceneMembershipRecord {
    const uint32_t *keys;
    size_t key_count;
    uint16_t flags;
    unsigned char known_original_predicate;
} SceneMembershipRecord;

typedef struct SceneMembershipEdge {
    uint32_t key;
    uint32_t ordinal;
} SceneMembershipEdge;

enum SceneMembershipInvalidation {
    SCENE_INDEX_INITIAL = 0,
    SCENE_INDEX_REBUILD,
    SCENE_INDEX_TREE_MUTATION,
    SCENE_INDEX_RECORD_MUTATION,
    SCENE_INDEX_MASK_MUTATION,
    SCENE_INDEX_PREDICATE_MUTATION,
    SCENE_INDEX_LIFETIME_MUTATION,
    SCENE_INDEX_CALLBACK,
    SCENE_INDEX_UNTRACKED_WRITE,
    SCENE_INDEX_INVALID_INPUT,
    SCENE_INDEX_CAPACITY
};

typedef struct SceneMembershipIndex {
    SceneMembershipEdge *edges;
    SceneMembershipEdge *scratch;
    uint32_t *unknown_ordinals;
    size_t edge_capacity, unknown_capacity;
    size_t edge_count, unknown_count;
    uint32_t record_count;
    uint64_t generation;
    uint64_t records_examined, keys_examined, radix_edge_visits;
    enum SceneMembershipInvalidation invalidation;
    unsigned char valid;
} SceneMembershipIndex;

typedef struct SceneMembershipCursor {
    const SceneMembershipIndex *owner;
    uint64_t generation;
    size_t edge, edge_end, unknown;
    unsigned char valid;
} SceneMembershipCursor;

typedef struct SceneMembershipCandidate {
    uint32_t ordinal;
    unsigned char needs_original_predicate;
} SceneMembershipCandidate;

enum SceneMembershipResult {
    SCENE_INDEX_FALLBACK = -1,
    SCENE_INDEX_END = 0,
    SCENE_INDEX_RECORD = 1
};

/* Initialization requires newly owned storage with no outstanding cursors. */
static void scene_membership_index_init(SceneMembershipIndex *index,
    SceneMembershipEdge *edges, SceneMembershipEdge *scratch, size_t edge_capacity,
    uint32_t *unknown_ordinals, size_t unknown_capacity)
{
    memset(index, 0, sizeof(*index));
    index->edges = edges;
    index->scratch = scratch;
    index->edge_capacity = edge_capacity;
    index->unknown_ordinals = unknown_ordinals;
    index->unknown_capacity = unknown_capacity;
}

static void scene_membership_index_invalidate(SceneMembershipIndex *index,
    enum SceneMembershipInvalidation reason)
{
    index->valid = 0;
    index->invalidation = reason;
    /* Saturation permanently prevents rebuild, avoiding stale-cursor ABA. */
    if (index->generation != UINT64_MAX) ++index->generation;
}

static int scene_membership_index_fail(SceneMembershipIndex *index,
    enum SceneMembershipInvalidation reason)
{
    index->invalidation = reason;
    index->valid = 0;
    return 0;
}

/* Storage ranges must be disjoint and sized as supplied. Build copies every
 * key and ordinal; source arrays may then be released. Four stable radix passes
 * preserve ordinal order within each unsigned key: O(records + edges + 1024).
 * Counts include duplicate record occurrences, never just unique object IDs.
 */
static int scene_membership_index_build(SceneMembershipIndex *index,
    const SceneMembershipRecord *records, size_t record_count,
    uint16_t mask, uint16_t expected)
{
    size_t ordinal, k, pass;
    scene_membership_index_invalidate(index, SCENE_INDEX_REBUILD);
    index->edge_count = index->unknown_count = 0;
    index->records_examined = index->keys_examined = index->radix_edge_visits = 0;
    if (index->generation == UINT64_MAX || record_count > UINT32_MAX ||
        (record_count && !records) ||
        (index->edge_capacity && (!index->edges || !index->scratch ||
                                  index->edges == index->scratch)) ||
        (index->unknown_capacity && !index->unknown_ordinals))
        return scene_membership_index_fail(index, SCENE_INDEX_INVALID_INPUT);
    index->record_count = (uint32_t)record_count;
    for (ordinal = 0; ordinal < record_count; ++ordinal) {
        const SceneMembershipRecord *record = &records[ordinal];
        ++index->records_examined;
        if ((uint16_t)(record->flags & mask) != expected) continue;
        if (!record->known_original_predicate) {
            if (index->unknown_count == index->unknown_capacity)
                return scene_membership_index_fail(index, SCENE_INDEX_CAPACITY);
            index->unknown_ordinals[index->unknown_count++] = (uint32_t)ordinal;
            continue;
        }
        if (record->known_original_predicate != 1 ||
            (record->key_count && !record->keys))
            return scene_membership_index_fail(index, SCENE_INDEX_INVALID_INPUT);
        if (record->key_count > index->edge_capacity - index->edge_count)
            return scene_membership_index_fail(index, SCENE_INDEX_CAPACITY);
        for (k = 0; k < record->key_count; ++k) {
            uint32_t key = record->keys[k];
            ++index->keys_examined;
            if (k && record->keys[k - 1] >= key)
                return scene_membership_index_fail(index, SCENE_INDEX_INVALID_INPUT);
            index->edges[index->edge_count].key = key;
            index->edges[index->edge_count++].ordinal = (uint32_t)ordinal;
        }
    }
    if (index->edge_count) {
        SceneMembershipEdge *src = index->edges, *dst = index->scratch;
        for (pass = 0; pass < 4; ++pass) {
            size_t counts[256] = {0}, offsets[256], offset = 0;
            SceneMembershipEdge *swap;
            for (k = 0; k < index->edge_count; ++k)
                ++counts[(src[k].key >> (pass * 8)) & 255u];
            for (k = 0; k < 256; ++k) {
                offsets[k] = offset;
                offset += counts[k];
            }
            for (k = 0; k < index->edge_count; ++k) {
                unsigned bucket = (src[k].key >> (pass * 8)) & 255u;
                dst[offsets[bucket]++] = src[k];
            }
            index->radix_edge_visits += 2 * (uint64_t)index->edge_count;
            swap = src; src = dst; dst = swap;
        }
    }
    index->valid = 1;
    return 1;
}

/* Outer order belongs to the caller: call this for each admitted outer in its
 * original order, including repeated outer keys. from_ordinal is a continuation
 * boundary in this exact snapshot, never an object identity.
 */
static SceneMembershipCursor scene_membership_index_seek(
    const SceneMembershipIndex *index, uint32_t outer_key, uint32_t from_ordinal)
{
    SceneMembershipCursor cursor;
    size_t lo, hi;
    memset(&cursor, 0, sizeof(cursor));
    if (!index->valid) return cursor;
    cursor.valid = 1;
    cursor.owner = index;
    cursor.generation = index->generation;
    lo = 0; hi = index->edge_count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (index->edges[mid].key < outer_key) lo = mid + 1;
        else hi = mid;
    }
    cursor.edge = lo;
    hi = index->edge_count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (index->edges[mid].key <= outer_key) lo = mid + 1;
        else hi = mid;
    }
    cursor.edge_end = lo;
    lo = cursor.edge; hi = cursor.edge_end;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (index->edges[mid].ordinal < from_ordinal) lo = mid + 1;
        else hi = mid;
    }
    cursor.edge = lo;
    lo = 0; hi = index->unknown_count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (index->unknown_ordinals[mid] < from_ordinal) lo = mid + 1;
        else hi = mid;
    }
    cursor.unknown = lo;
    return cursor;
}

static enum SceneMembershipResult scene_membership_index_next(
    const SceneMembershipIndex *index, SceneMembershipCursor *cursor,
    SceneMembershipCandidate *candidate)
{
    if (!index->valid || !cursor->valid || cursor->owner != index ||
        cursor->generation != index->generation)
        return SCENE_INDEX_FALLBACK;
    if (cursor->edge == cursor->edge_end && cursor->unknown == index->unknown_count)
        return SCENE_INDEX_END;
    if (cursor->unknown < index->unknown_count &&
        (cursor->edge == cursor->edge_end ||
         index->unknown_ordinals[cursor->unknown] < index->edges[cursor->edge].ordinal)) {
        candidate->ordinal = index->unknown_ordinals[cursor->unknown++];
        candidate->needs_original_predicate = 1;
    } else {
        candidate->ordinal = index->edges[cursor->edge++].ordinal;
        candidate->needs_original_predicate = 0;
    }
    return SCENE_INDEX_RECORD;
}
#endif

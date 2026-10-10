#ifndef UC_NATIVE_LEVEL_ASSET_H
#define UC_NATIVE_LEVEL_ASSET_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct UCLevelBytes { const uint8_t *data; uint32_t size; } UCLevelBytes;
typedef struct UCLevelScript { const char *name; UCLevelBytes code; } UCLevelScript;
typedef struct UCLevelEntity {
    const char *class_name, *asset_name, *field50, *name;
    float transform[16]; /* Unchanged row-major authored world transform. */
    uint32_t flags, active, field40, source_offset;
    UCLevelScript *scripts;
    uint32_t script_count, link_count;
    const char **links;
    UCLevelBytes script_records, extra, properties;
} UCLevelEntity;
typedef struct UCLevel {
    uint8_t *storage; size_t size;
    UCLevelEntity *entities;
    uint32_t entity_count, version, field, section_counts[4];
    UCLevelBytes remap, tail;
} UCLevel;
typedef struct UCLevelRestart {
    uint32_t entity_index;
    int32_t number;
    uint32_t on_restart[2], on_operated[2], on_post_restart[2], auto_fade;
    float transform[16];
    float yaw; /* Original BD150: -atan2(transform[8],transform[10]), radians. */
} UCLevelRestart;
typedef struct UCLevelIndices { uint32_t *data, count; } UCLevelIndices;
typedef struct UCLevelTrigger {
    uint32_t source_offset, kind, shape, packed, shape_field;
    int32_t number;
    float position[3], transform[16], dimensions[3], delay, elapsed;
    uint8_t flags; /* Source6C: all-tests1, fired2, pending4, once8, enabled16, script32. */
    UCLevelBytes boxes, flag_tests, flag_writes;
    UCLevelIndices subjects, targets;
    /* Actual220A30/220F70 custom script metadata; no fabricated entity class.
     * Use the existing group/condition readers on this borrowed descriptor. */
    UCLevelEntity script;
} UCLevelTrigger;
typedef struct UCLevelNode {
    uint32_t key, field10;
    float position[3];
    UCLevelIndices first, second;
} UCLevelNode;
typedef struct UCLevelLogic {
    const UCLevel *level;
    UCLevelTrigger *triggers;
    UCLevelNode *nodes;
    uint32_t trigger_count, node_count;
} UCLevelLogic;
typedef enum UCLevelStatus {
    UC_LEVEL_OK, UC_LEVEL_ARGUMENT, UC_LEVEL_FORMAT, UC_LEVEL_UNSUPPORTED,
    UC_LEVEL_LIMIT, UC_LEVEL_MEMORY, UC_LEVEL_NOT_FOUND
} UCLevelStatus;

/* Decompressed inner .ent bytes. Owns a copy and all returned metadata. Output
 * must be zero-initialized. Current scope: version16 entity prefix/version11
 * descriptors; subsequent three level sections are retained opaque, NOT
 * validated or executed. Script bytes and property/link IDs are data, never
 * host pointers. No script interpreter or entity simulation is included.
 * Bounds: 256MiB source/allocations, 100K records per collection. */
UCLevelStatus uc_level_load(const void *data, size_t size, UCLevel *out);
void uc_level_free(UCLevel *level);
/* Version16 tail213800/213E60, borrowing the same immutable level snapshot.
 * Owns only native descriptor/list arrays; free before the level. All script,
 * box and flag-pair bytes retain original storage/order. Negative source entity
 * indices are filtered as in213800, while node lists preserve every word.
 * Outputs must start zero; malformed/unsupported input leaves them unchanged.
 * Loading is not trigger execution, pathfinding or mission completion. */
UCLevelStatus uc_level_logic_load(const UCLevel *,UCLevelLogic *);
void uc_level_logic_free(UCLevelLogic *);
/* Original2104A0: first node whose key equals(id&FFF0FFFF). Borrowed result,
 * NULL for a completed miss. Argument failures leave output untouched.
 * Caller resolves ordinary entity indexes through210CA0 first when required. */
UCLevelStatus uc_level_node_find(const UCLevelLogic *,uint32_t id,const UCLevelNode **);
/* Source221420 allocates metadata.asset only for a nonzero serialized length.
 * The native text view normalizes absent and allocated-empty strings to "";
 * this accessor retains that distinction without changing the entity layout.
 * Borrows an unmodified successfully loaded level. present is disjoint storage,
 * written only on success; no allocation, cursor advance or string-content test.
 * The original allocator must have completed; allocation failure is not modeled. */
UCLevelStatus uc_level_entity_asset_present(const UCLevel *level,uint32_t index,
                                          uint8_t *present);
/* Exact RestartPoint payload semantics from137470. number==-1 selects lowest
 * signed checkpoint number, matching initial no-save restart-manager fallback.
 * Duplicate numbers use first source entry as original registration does.
 * Result is player body pose, NOT camera eye pose/cutscene state. */
UCLevelStatus uc_level_restart(const UCLevel *level, int32_t number, UCLevelRestart *out);
const char *uc_level_status(UCLevelStatus status);
#ifdef __cplusplus
}
#endif
#endif

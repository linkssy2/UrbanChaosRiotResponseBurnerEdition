#ifndef UC_LEVEL_ENTITY_SLOTS_H
#define UC_LEVEL_ENTITY_SLOTS_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Borrow the actual per-level authored slot+4/+8 fields, not the separate
 * generation-ID registry. Views/table storage stay alive through ensure. */
typedef struct UCLevelEntitySlot { int32_t *id; uint32_t *flags; } UCLevelEntitySlot;
typedef struct UCLevelEntitySlots {
    UCLevelEntitySlot *const *slots;
    const int32_t *count;
    size_t capacity;
    void *owner;
    int (*create)(void *,UCLevelEntitySlot *,uint32_t);
} UCLevelEntitySlots;
/* Whole210CF0. Index and live count compare signed; misses complete with ID-1.
 * Inactive slots publish (flags&~13)|2 before required210910 creation. create
 * returns0 only after completing its real effect (which may leave flag8 clear).
 * It may change table entries/count and the captured slot fields, but must not
 * release/rebind that slot/view/field storage or reenter this owner. Completion
 * reads the captured slot, not a replacement table entry. No shadow ID state.
 * Caller admits valid table entries and disjoint output; capacity is the actual
 * readable table extent. Return0 original result,1 invalid before effects,
 *2 committed service failure (out untouched, no rollback or automatic retry).
 * Class creation/registration is an explicit required service, not supplied here. */
int uc_level_entity_ensure(const UCLevelEntitySlots *,uint32_t index,int32_t *id);
/* Whole210CA0: encoded IDs with bitsF0000 pass through; ordinary indexes query
 * existing slots without creation. Flag10 returns-2 before flag8/id. */
int uc_level_entity_lookup(const UCLevelEntitySlots *,uint32_t index,int32_t *id);
#ifdef __cplusplus
}
#endif
#endif

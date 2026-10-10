#ifndef UC_ENTITY_REGISTRATION_H
#define UC_ENTITY_REGISTRATION_H
#include "entity_slots.h"
#include "entity_list.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCEntityCreation {
    uint8_t relative;
    int32_t priority,parent_id;
} UCEntityCreation;
typedef struct UCEntityRegistrationTarget {
    UCEntityEventTarget *identity;
    uint32_t *flags;
    int32_t *parent_id;
} UCEntityRegistrationTarget;
typedef struct UCEntityRegistration {
    UCEntityEventRegistry *registry;
    uint32_t *next_id;
    UCEntityList *live;
    UCEntityCreation *creation;
    void *owner;
    /* Pure read of the given current target's ACTUAL flags, never a copied
     * private identity map. A target resolved by this registry must be known. */
    uint32_t (*flags)(void *,const UCEntityEventTarget *);
    /* Complete207E50 on render(update=0) or update(update=1) collection.
     * Read current creation fields at their source frontiers. Return0 after
     * completing even a source insertion failure; record ownership must remain
     * accounted for. Nonzero means committed native service failure. */
    int (*insert_record)(void *,const UCEntityRegistrationTarget *,int32_t id,int update);
} UCEntityRegistration;
typedef struct UCEntityRegistrationResult {int32_t id;uint8_t live_inserted;} UCEntityRegistrationResult;
/*208150, including its207F00 relative-parent branch. Borrowed fields alias the
 * actual entity; flags/parent/creation/registry/counter/live/output are disjoint
 * stable owners. Exclusive nonreentrant call. Completing hooks may mutate
 * creation fields and entity flags/parent, but must retain the new acquired slot
 * and its target identity until return. Parent lookup uses this same registry.
 * No entity+54 publication here:20BDC0 writes that AFTER208150 returns.
 * Slot exhaustion still attempts live-list/record insertion using id=-1.
 * Live-list exhaustion retains the acquired slot and parent publication, skips
 * records and still clears flag8. These source failures are never rolled back.
 * Return0 completed,1 invalid before effects,2 committed service failure; only0
 * publishes output. Full factory/base/listener/placement lifetime remains caller
 * work. Successful gameplay callers must admit enough real capacity first. */
int uc_entity_register(const UCEntityRegistration *,const UCEntityRegistrationTarget *,UCEntityRegistrationResult *);
#ifdef __cplusplus
}
#endif
#endif

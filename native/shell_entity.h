#ifndef UC_SHELL_ENTITY_H
#define UC_SHELL_ENTITY_H
#include "shell_lifecycle.h"
#include "entity_fresh.h"
#include "entity_names.h"
#ifdef __cplusplus
extern "C" {
#endif
/* One actual shell state, borrowed by FreshView. Transform/ID/parent/model slots
 * MUST alias that state. No additional authoritative entity or group registry. */
typedef struct UCShellEntityConstruction {
    const UCEntityFreshContext *entities;
    const UCEntityFreshView *view;
    void *owner;
    UCEntityFreshTagWorld tag_world;
    UCEntityNames *(*current_names)(void *); /*actual current51C398+0C, pure */
    uint32_t tag;
} UCShellEntityConstruction;
/* Full11DFB0 composition: actual mode4 fresh construction, suffix/shared RNG,
 * current same-registry parent position, then PLAYER_BULLET_SHELLS insertion.
 * All borrowed storage/metadata remain stable and disjoint except documented
 * fresh registration/allocation mutations. Current names must resolve a valid
 * real owner at publication; its allocation may fail, reported as2 after suffix.
 * Return0 complete,1 rejected before effects,2 possible committed effects; never
 * retry2. The caller owns any registered shell/aux after failure. No outer class
 * factory/allocation, model attach, update, sound or retirement is claimed. */
int uc_shell_entity_construct(UCShellState *,const UCShellProperties *,UCGameRandom *,
    const UCShellEntityConstruction *);
#ifdef __cplusplus
}
#endif
#endif

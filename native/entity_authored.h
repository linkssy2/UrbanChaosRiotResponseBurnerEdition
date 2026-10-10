#ifndef UC_ENTITY_AUTHORED_H
#define UC_ENTITY_AUTHORED_H
#include "entity_fresh.h"
#include "mission_conditions.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual active descriptor+10 metadata and +14 source index. Conditions are
 * the SAME level-owned mutable used bytes, not a per-actor copy. asset_present
 * is serialized length!=0 (including length1/NUL), not asset_name[0]. */
typedef struct UCEntityAuthored {
    const UCLevelEntity *definition;
    int32_t source_index;
    uint8_t asset_present;
    UCMissionCondition *conditions;
    uint32_t condition_count;
} UCEntityAuthored;
/*20BDC0->218550->218A60 authored branch on the existing real registration,
 * aux and actor owners. active is reloaded at source frontiers after callbacks.
 * Metadata/storage remain live; callbacks may replace active only with another
 * valid authored view. Dynamic/null transitions are outside this overload.
 *0 completed,1 rejected before effects,2 committed failure; no rollback. */
int uc_entity_fresh_construct_authored(const UCEntityFreshContext *,
    const UCEntityFreshView *,const UCEntityAuthored *const *active);
/* Only authored20BDC0; no actor/aux allocation, group counter or condition
 * publication. aux84/actor may be NULL. Other authored/base contracts apply. */
int uc_entity_base_construct_authored(const UCEntityFreshContext *,
    const UCEntityFreshView *,const UCEntityAuthored *const *active);
/* Authored218A10->218550: base, real auxiliary, counter/group and health fields
 * only. Stops before218A60; actor fields B0 and later are untouched, including
 * script cursor/conditions. This is the LightFitting base (its own coroutine
 * occupies B0). Same allocation/registration/failure contracts as full fresh. */
int uc_entity_health_construct_authored(const UCEntityFreshContext *,
    const UCEntityFreshView *,const UCEntityAuthored *const *active);
#ifdef __cplusplus
}
#endif
#endif

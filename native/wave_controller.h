#ifndef UC_WAVE_CONTROLLER_H
#define UC_WAVE_CONTROLLER_H
#include <stdint.h>
#include "level_entity_slots.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original F05C0's retained descriptor list, not a second character registry.
 * The original object reserves32 entries. lookup is210CA0; status reads the
 * actual resolved Character+198. IDs-1/-2 retire a member without a status read.
 * Services must preserve this owner's fields and all borrowed storage. */
typedef struct UCWaveMembers {
 int32_t source[32];uint8_t active[32];int32_t count;
} UCWaveMembers;
typedef struct UCWaveQuery {
 void *owner;
 int (*lookup)(void *,int32_t source,int32_t *id);
 int (*status)(void *,int32_t id,uint32_t *flags);
} UCWaveQuery;
/*0 completed query,1 invalid before effects,2 failed service after earlier
 * member retirements (no retry). done is written only on0. First live member
 * returns done0 immediately; later members are deliberately untouched. Mask
 *20007 is copied from source, not a guessed health/death predicate. */
int uc_wave_members_complete(UCWaveMembers *,const UCWaveQuery *,uint8_t *done);
/* Same completion check using actual level slots; no duplicate ID map. */
int uc_wave_members_complete_slots(UCWaveMembers *,const UCLevelEntitySlots *,
 void *owner,int (*status)(void *,int32_t,uint32_t *),uint8_t *done);
#ifdef __cplusplus
}
#endif
#endif

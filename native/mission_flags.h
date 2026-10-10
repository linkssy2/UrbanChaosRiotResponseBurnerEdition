#ifndef UC_MISSION_FLAGS_H
#define UC_MISSION_FLAGS_H
#include "level_asset.h"
#ifdef __cplusplus
extern "C" {
#endif
/*210840/210890 use the ENT remap pairs, not the flag number as an index.
 * Preserve signed number>pair_count early return and first matching row,
 * including a negative target that suppresses a later duplicate.0 completed,
 * 1 invalid data before writes. Missing/negative mapping is a completed no-op. */
int uc_mission_flag_set(UCLevelBytes remap,uint8_t *flags,size_t count,int32_t number,uint8_t value);
/*C8973..C89F1: publish equipped pistol-family13 and Taser-family14 through
 * that SAME map. Item24 is deliberately not in the source pistol family.
 * Stable disjoint owners; no inventory or equipment-resource commit here. */
int uc_mission_equipment_flags(const UCLevel *,uint8_t *flags,size_t count,int32_t item);
#ifdef __cplusplus
}
#endif
#endif

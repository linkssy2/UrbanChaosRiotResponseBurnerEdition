#ifndef UC_CHARACTER_HAZARDS_H
#define UC_CHARACTER_HAZARDS_H
#include "level_asset.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCCharacterHazards UCCharacterHazards;
/* Original world928 owner571D8 starts with an empty list. This admission checks
 * the actual level has none of its three authored registrants; their factories
 * must not silently skip registration when admitted later. Dynamic factories
 * own the registrations below. No smoke simulation or speech is fabricated.
 * 0 success,1 invalid/unsupported/allocation failure. Outputs unchanged on1. */
int uc_character_hazards_create(const UCLevel *,UCCharacterHazards **);
void uc_character_hazards_close(UCCharacterHazards *);
/*4F980/4FA00 append in order, including duplicate IDs; 4F320/4F390/4F930
 * update/remove the first match. A missing ID is a completed no-op. Box data
 * retains the original12 floats (min/max,center,extent), even though4EFE0
 * examines only min/max. Finite source data only; native cap4096. */
int uc_character_hazards_sphere(UCCharacterHazards *,uint32_t id,const float position[3],float radius,uint32_t kind);
int uc_character_hazards_box(UCCharacterHazards *,uint32_t id,const float bounds[12],uint32_t kind);
int uc_character_hazards_move_sphere(UCCharacterHazards *,uint32_t id,const float position[3],float radius);
int uc_character_hazards_move_box(UCCharacterHazards *,uint32_t id,const float bounds[12]);
int uc_character_hazards_remove(UCCharacterHazards *,uint32_t id);
/* Full4F460→4EFE0: kind filter, strict sphere interior, inclusive AABB.
 * Position is the caller's actual head-pos property, not actor feet. Output
 * disjoint from input/owner, changed only on success. Empty list is real miss. */
int uc_character_hazards_query(const UCCharacterHazards *,const float position[3],uint32_t kind,uint8_t *hit);
/*4F4C0 ->4F0D0/45260: 1 means no matching hazard intersects the segment.
 * Sphere tangency is clear; AABB contact blocks. Empty matching list is clear.
 * Uses these same retained registrations, never scenery collision. */
int uc_character_hazards_segment_clear(const UCCharacterHazards *,const float from[3],const float to[3],uint32_t kind,uint8_t *clear);
#ifdef __cplusplus
}
#endif
#endif

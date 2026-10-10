#ifndef UC_CHARACTER_PLAYER_RANGE_H
#define UC_CHARACTER_PLAYER_RANGE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/*33B60: actor position is actual body virtual2C when present, entity position
 * otherwise. Player is its actual entity position. Query 'Look point' only
 * after the strict distance gate; do not substitute camera or head position.
 *0completed,1invalid,2property servicefailed. False is legitimate missingproperty. */
int uc_character_player_range(const float actor[3],const float player[3],
    float range,float angle_degrees,void *owner,
    int (*look_point)(void *,float matrix[16],uint8_t *found),uint8_t *inside);
#ifdef __cplusplus
}
#endif
#endif

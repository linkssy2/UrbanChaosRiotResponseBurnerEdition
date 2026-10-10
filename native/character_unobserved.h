#ifndef UC_CHARACTER_UNOBSERVED_H
#define UC_CHARACTER_UNOBSERVED_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCCharacterPlayerView {float position[3],head_direction[3];}UCCharacterPlayerView;
/*32EA0 projected actual live player positions and 'head direction' values.
 * Caller uses source default(0,1,0) only when that actual property is absent.
 * 0completed,1invalid; no guessed visibility/raycast or camera-frustum test. */
int uc_character_unobserved(const float actor[3],const UCCharacterPlayerView *,size_t count,uint8_t *);
/*33AA0 tests actual controller+10 list count, not animation pending state. */
uint8_t uc_character_tasks_empty(uint32_t actual_task_count);
#ifdef __cplusplus
}
#endif
#endif

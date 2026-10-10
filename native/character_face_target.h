#ifndef UC_CHARACTER_FACE_TARGET_H
#define UC_CHARACTER_FACE_TARGET_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Whole33E70: target yaw clamped to one update's radians; authored orientation
 * rows rotate in the original SSE order, translation row remains unchanged.
 * Actor position is distinct from its virtual2C matrix.0 success,1 badargument. */
int uc_character_face_target(const float world[16],const float position[3],const float target[3],float limit,float out[16],uint8_t*changed);
#ifdef __cplusplus
}
#endif
#endif

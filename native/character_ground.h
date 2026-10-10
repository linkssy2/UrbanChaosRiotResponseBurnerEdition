#ifndef UC_CHARACTER_GROUND_H
#define UC_CHARACTER_GROUND_H
#include <stdint.h>
typedef struct UCCharacterGroundServices {
    void *owner;
    /*214230: return1 hit,0 miss,-1 failure; positions are render centimetres.*/
    int (*ray)(void *,const float from[3],const float to[3],uint32_t type,
        uint32_t group,uint32_t mask,float impulse,float point[3],float normal[3]);
} UCCharacterGroundServices;
/*Full2B50E0. Missing ground returns the probe origin's Y (position+100).*/
int uc_character_ground(const float position[3],uint32_t group,
    const UCCharacterGroundServices *,float *height,float normal[3],uint8_t *found);
#endif

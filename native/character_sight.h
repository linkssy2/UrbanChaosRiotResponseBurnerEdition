#ifndef UC_CHARACTER_SIGHT_H
#define UC_CHARACTER_SIGHT_H
#include <stdint.h>
typedef struct UCCharacterSightActor {
    const void *identity;
    float position[3];
    int32_t parent;
    uint16_t group;
} UCCharacterSightActor;
typedef struct UCCharacterSightServices {
    void *owner;
    /*1 found,0 absent,-1 unsupported. Retain actual identity and parent58.*/
    int (*resolve)(void *,int32_t,UCCharacterSightActor *);
    int (*point)(void *,const UCCharacterSightActor *,const char *,float[3]);
    int (*forward)(void *,const UCCharacterSightActor *,float[3]);
    /*214230:1 hit,0 clear,-1 unsupported. A hit can have no actor.*/
    int (*ray)(void *,const float[3],const float[3],uint32_t,uint16_t,uint32_t,UCCharacterSightActor *);
} UCCharacterSightServices;
/*E05C0 with full20FF40/20FE90 policy. Finite caller inputs, RN float32 and
 * original x87 precision53. Ray/world/property owners are supplied, never
 * guessed from scene pixels.0 completed,1 invalid/unsupported; visible is only
 * published on completion. Singular horizontal vectors preserve unordered
 * comparison behavior, including the original short-ray clear result.*/
int uc_character_sight(const UCCharacterSightActor *,int32_t target,float cosine,
    uint8_t head,const float offset[3],uint8_t characters,
    const UCCharacterSightServices *,uint8_t *visible);
#endif

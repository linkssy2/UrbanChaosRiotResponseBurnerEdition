#ifndef UC_CHARACTER_LOOK_H
#define UC_CHARACTER_LOOK_H
#include "game_random.h"
#include "entity_names.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual actor+194 owner3BE30. Direction/forward are unwritten until first
 * reset/update. direction_known records that provenance; it is not guest data. */
typedef struct UCCharacterLook {
    float remaining;int32_t last_frame;float direction[3],forward[3];
    int32_t target;uint8_t nearby,direction_known;
} UCCharacterLook;
typedef struct UCCharacterLookActor {float world[16],head[3];uint8_t has_head;} UCCharacterLookActor;
typedef struct UCCharacterLookServices {
    void *owner;
    /* SAME live registry, actual world transform and optional 'head pos'.
     * Missing entity is success with found=0; unsupported properties fail. */
    int (*actor)(void *,int32_t,UCCharacterLookActor *,uint8_t *found);
    const UCEntityNames *names;int32_t player;
} UCCharacterLookServices;
int uc_character_look_init(UCCharacterLook *);
/*3BED0 consumes one shared draw even when setting the existing target. */
int uc_character_look_target(UCCharacterLook *,UCGameRandom *,int32_t);
/*3C510 and complete target selection3C120/3C080/3BF00. Frame is actual
 * world+0C, incremented before update. Unknown initial direction, coincident
 * target, malformed services and nonfinite data fail explicitly. Successful
 * missing-self query outputs zero and preserves owner/RNG. A failed service
 * may follow committed draws; enclosing session must stop, not retry. */
int uc_character_look_step(UCCharacterLook *,UCGameRandom *,const UCCharacterLookServices *,
                           int32_t self,uint32_t frame,float delta,float output[3]);
#ifdef __cplusplus
}
#endif
#endif

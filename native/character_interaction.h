#ifndef UC_CHARACTER_INTERACTION_H
#define UC_CHARACTER_INTERACTION_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/*17BD80 for the authored1070 option set: automatic interaction, body look,
 * no player lock/animations/quit flags, no initial/pre/after script or post text.
 * The real dialogue, turn completion and script continuation remain owners. */
typedef struct UCCharacterInteraction {
    uint32_t phase;
    float elapsed,range,facing;
    int32_t next,script_index;
    uint32_t script_type;
    const char *text;
} UCCharacterInteraction;
typedef struct UCCharacterInteractionServices {
    void *owner;
    int (*player_present)(void *);
    int (*busy)(void *);
    int (*flags_ready)(void *);
    int (*in_range)(void *,float range,float facing);
    int (*stop_turn)(void *);
    int (*turn)(void *,int begin);
    int (*turn_ready)(void *);
    int (*speech_ready)(void *);
    int (*say)(void *,const char *);
    int (*speaking)(void *);
    int (*script)(void *,uint32_t,int32_t);
    int (*cancel_speech)(void *);
    int (*transition)(void *,int32_t);
    int (*base_update)(void *,uint8_t);
} UCCharacterInteractionServices;
/*Queries return0/1, negative on failure; completing effects return0. Nonzero
 * after effects is committed failure, never a request to retry the same tick. */
int uc_character_interaction_update(UCCharacterInteraction *,float delta,uint8_t expensive,
    const UCCharacterInteractionServices *);
#ifdef __cplusplus
}
#endif
#endif

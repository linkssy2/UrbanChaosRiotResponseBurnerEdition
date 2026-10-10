#ifndef UC_CHARACTER_APPEARANCE_H
#define UC_CHARACTER_APPEARANCE_H
#include "game_random.h"
#include "character_loop.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCCharacterAppearance {
    UCCharacterLoopController *controller;
    void *actor;
    uint8_t active;
} UCCharacterAppearance;
typedef struct UCCharacterAppearanceServices {
    void *owner;
    UCGameRandom *random;
    /* Required real body/skin publication, rooted behavior property lookup,
     * body equipment visibility and behavior exit. Stable borrowed owners;
     * callbacks must finish their effects, not just record successful calls. */
    int (*has_body)(void *,void *actor);
    void (*skin)(void *,void *actor,int32_t selection);
    int (*property)(void *,UCCharacterLoopController *,const char *name);
    void (*equipment)(void *,void *actor,unsigned group,int32_t enabled);
    void (*exit)(void *,UCCharacterAppearance *);
} UCCharacterAppearanceServices;
/* Complete13B250 with32660 scalar entry; skin service performs11F030 then
 * body.virtual68(0,&actor.skin). Equipment resolves body.virtual6C on EACH
 * call then virtual4(group,enabled). State/body lifetime cannot end until exit.
 * Original requires a body for equipment even when optional skin is skipped.
 * Shared RNG must remain valid; no reseed/clone or other RNG consumption in
 * callbacks. Return1 invalid before effects,0 completed. This does not create
 * the NPC body, draw equipment or supply missing behavior ownership. */
int uc_character_appearance_enter(UCCharacterAppearance *,UCCharacterLoopController *,const UCCharacterAppearanceServices *);
#ifdef __cplusplus
}
#endif
#endif

#ifndef UC_GUNFLARE_H
#define UC_GUNFLARE_H
#include "game_random.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCGunFlareModels {
    const char *name;
    void *const *instances;
    uint32_t count;
} UCGunFlareModels;
typedef struct UCGunFlare {
    float world[16],sprite_position[3];
    int32_t parent;
    uint32_t selected,variant,counter; /* selected==UINT32_MAX means none. */
    uint8_t first_person,pending,visible,sprite_enabled;
} UCGunFlare;
typedef struct UCGunFlareServices {
    void *user;
    int (*particles)(void *,const char *name,const float matrix[16],UCGameRandom *);
    int (*tint)(void *,void *model,const float rgba[4]);
    int (*visible)(void *,void *model,unsigned enabled);
    int (*transform)(void *,void *model,const float matrix[16]);
    int (*parent_exists)(void *,int32_t parent,unsigned *exists);
    int (*remove)(void *);
} UCGunFlareServices;
/* Bounded full186C40 ordinary pistol2/shotgun17, first- or third-person,
 *187820 and1878C0. Shotgun payload+4 must be zero (no alternate smoke).
 * Borrow up to16 cached model groups/256 instances each, in original list order.
 * Exact case-sensitive asset names; completed model construction/registration,
 * source world matrix, parent registry, simulation scheduling and all effects
 * belong to caller. Calls must run at entity update boundaries, not presentation.
 * Completing callbacks return0; may consume shared RNG only in particles, and
 * must not alter these owners/model lists/FP environment. Disjoint valid owners.
 * Finite world components abs<=65536, nearest/gradual FP, valid shared RNG.
 * Returns0 success,1 initial rejection,2 partial service failure,3 invalid RNG
 * result after consumption. Stop on partial failure; never retry the same event.
 * No constructor, event bus, GPU, light flash or alternate-shot claim. */
int uc_gunflare_shot(UCGunFlare *,const UCGunFlareModels *,unsigned groups,
                    unsigned item,UCGameRandom *,const UCGunFlareServices *);
int uc_gunflare_tick(UCGunFlare *,const UCGunFlareModels *,unsigned groups,
                    const UCGunFlareServices *);
int uc_gunflare_transform(UCGunFlare *,const UCGunFlareModels *,unsigned groups,
                         UCGameRandom *,const UCGunFlareServices *);
#ifdef __cplusplus
}
#endif
#endif

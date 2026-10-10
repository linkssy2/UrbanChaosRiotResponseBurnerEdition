#ifndef UC_CHARACTER_ROOT_UPDATE_H
#define UC_CHARACTER_ROOT_UPDATE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCCharacterRootUpdate {
 int32_t order,regeneration_flag; /*root8C/B0*/
 uint8_t restrained;float restraint_time,damage_time; /*92/94/DC*/
 uint32_t *status; /*actual actor198, shared with damage reaction*/
} UCCharacterRootUpdate;
typedef struct UCCharacterRootUpdateServices {
 void*owner;
 double(*health)(void*);double(*maximum)(void*);
 int(*common)(void*); /*actual root virtual78*/
 int(*emote)(void*,uint32_t,uint32_t);
 int(*flag)(void*,int32_t); /*negative error, else Boolean*/
 double(*delta)(void*);
 int(*set_health)(void*,float); /*actual actor virtual64*/
} UCCharacterRootUpdateServices;
/*Complete368E0; zero success,1 invalid,2 required owner failed.*/
int uc_character_root_update(UCCharacterRootUpdate*,uint8_t hurt_status,uint8_t regenerate,const UCCharacterRootUpdateServices*);
#ifdef __cplusplus
}
#endif
#endif

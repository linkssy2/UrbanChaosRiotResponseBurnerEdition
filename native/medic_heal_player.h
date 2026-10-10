#ifndef UC_MEDIC_HEAL_PLAYER_H
#define UC_MEDIC_HEAL_PLAYER_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCMedicHealPlayer {
 int32_t phase,pillbox;float origin[3],last_player[3];uint8_t delivered,spawn;
} UCMedicHealPlayer;
typedef struct UCMedicHealPlayerServices {
 void*owner;
 int(*player)(void*,float[3]);int(*actor)(void*,float[3]);
 int(*boolean)(void*,uint8_t root,const char*,uint8_t fallback,uint8_t*);
 int(*completion)(void*,int32_t*); /*36250: NULL is3*/
 int(*say)(void*,int32_t,int32_t);int(*stop_aim)(void*);int(*stop)(void*);
 int(*clip)(void*,uint8_t recovery); /*inner2C, completion,0,0*/
 int(*nav_target)(void*,const float[3]);int(*nav_stop)(void*);
 int(*run)(void*,int32_t*); /*33020(1,1,0,1,0)*/
 int(*cancel_speech)(void*); /*21A580(0,1)*/
 int(*aim)(void*,const float[3],uint32_t); /*inner18 mode3+completion*/
 int(*event)(void*,uint32_t); /*player87/86, zero DWORD payload*/
 int(*random_pair)(void*,int32_t*);int(*heals)(void*,int32_t*);
 int(*bone_world)(void*,float[16],float[16]); /*L Hand + actor body world*/
 int(*pillbox)(void*,const float[16],int32_t*); /*source MeleeWeapon descriptor*/
 int(*delivered)(void*); /*147,float100 to player,root,foreground in order*/
 int(*remove)(void*,int32_t);int(*leave)(void*);int(*base)(void*,float);
} UCMedicHealPlayerServices;
/*182C20 state machine; entry182980 sets phase0 and watches the two authored
 * markers. Exit1829E0 is the completing owner's lifecycle, not a health shortcut.*/
int uc_medic_heal_player_update(UCMedicHealPlayer*,float,const UCMedicHealPlayerServices*);
void uc_medic_heal_pillbox_matrix(const float[16],const float[16],float[16]);
#ifdef __cplusplus
}
#endif
#endif

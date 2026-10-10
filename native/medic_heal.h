#ifndef UC_MEDIC_HEAL_H
#define UC_MEDIC_HEAL_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCMedicHealCheck {int32_t state,child;float elapsed;} UCMedicHealCheck;
typedef struct UCMedicHealServices {
 void*owner;double(*delta)(void*);
 int(*exists)(void*,int32_t); /*1 found,0 missing,-1 unsupported*/
 int(*heals)(void*,int32_t*); /*root heals left default0*/
 int(*nearby)(void*,float,float); /*controller33B60,1/0/-1*/
 int(*visible)(void*); /*player0 ->E05C0(id,-1,1,{0,0,0},0)*/
 int(*player)(void*); /*4ACE0(0),1/0/-1*/
 double(*health)(void*); /*actual player218120*/
 int(*property)(void*,unsigned scope,const char*,uint8_t); /*root0/player1/foreground2,1/0/-1*/
 int(*dialogue_busy)(void*); /*21A4C0,1/0/-1*/
 double(*percent_health)(void*); /*player0 property, default100*/
 int(*spawn)(void*,const char*,int32_t*); /*32580 empty159C0 descriptor*/
 int(*queue)(void*,int32_t); /*32D10*/
 int(*emote)(void*,uint32_t,uint32_t); /*DED50*/
 int(*base)(void*,float); /*32680*/
} UCMedicHealServices;
void uc_medic_heal_init(UCMedicHealCheck*); /*1C5910 suffix after32660*/
int uc_medic_heal_update(UCMedicHealCheck*,float,const UCMedicHealServices*);
#ifdef __cplusplus
}
#endif
#endif

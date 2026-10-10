#ifndef UC_MEDIC_HEAL_DIALOGUE_H
#define UC_MEDIC_HEAL_DIALOGUE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCMedicHealOrder {int32_t command,previous;} UCMedicHealOrder;
/*184200/1842F0 scalar subclass; vectors are forwarded to its real manager.*/
int uc_medic_heal_order_choose(UCMedicHealOrder*,int32_t,void*,int(*complete)(void*,int32_t));
int uc_medic_heal_order_integer(const UCMedicHealOrder*,const char*,int32_t*,uint8_t*found);
typedef struct UCMedicHealDialogue {int32_t phase,client;uint8_t lock_player,locked,head_only;} UCMedicHealDialogue;
typedef struct UCMedicHealDialogueServices {
 void*owner;
 int(*completion)(void*,int32_t*); /*-1 absent*/
 int(*boolean)(void*,const char*,uint8_t,uint8_t*);
 int(*loop)(void*); /*inner34(all optional flags0), source dialogue loop*/
 int(*lock)(void*,uint8_t); /*149 byte*/
 int(*head)(void*,float[3]); /*source absent property leaves zero*/
 int(*look)(void*,const float[3],uint8_t head_only); /*inner20 or18(mode2)*/
 int(*say)(void*,int32_t,int32_t);int(*stop_aim)(void*);int(*stop_look)(void*);
 int(*create_client)(void*,int32_t*);int(*remove_client)(void*,int32_t);
 int(*command)(void*,int32_t,int32_t*);int(*nearby)(void*,float,float,uint8_t*);
 int(*heal_child)(void*); /*32580 HealPlayer+325F0 priority0*/
 int(*child)(void*,int32_t*);int(*leave)(void*);int(*base)(void*,float);
 int(*base_exit)(void*); /*328F0; leave above dispatches the actual subclass9C*/
} UCMedicHealDialogueServices;
/*168B80/168A40. Only original owners create clients, deliver events or animate.*/
int uc_medic_heal_dialogue_update(UCMedicHealDialogue*,float,const UCMedicHealDialogueServices*);
int uc_medic_heal_dialogue_exit(UCMedicHealDialogue*,const UCMedicHealDialogueServices*);
#ifdef __cplusplus
}
#endif
#endif

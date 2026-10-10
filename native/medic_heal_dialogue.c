#include "medic_heal_dialogue.h"
#include <math.h>
#include <string.h>
#define CALL(n,...)do{if(!h->n||h->n(h->owner,__VA_ARGS__))return 2;}while(0)
#define CALL0(n)do{if(!h->n||h->n(h->owner))return 2;}while(0)
static int look(UCMedicHealDialogue*s,const UCMedicHealDialogueServices*h){float head[3]={};uint8_t healing=0;CALL(head,head);CALL(boolean,"MedicAutoHeal is healing",0,&healing);if(healing)head[1]+=50.f;CALL(look,head,s->head_only);return 0;}
static int remove_client(UCMedicHealDialogue*s,const UCMedicHealDialogueServices*h){CALL(remove_client,s->client);s->client=-1;return 0;}
int uc_medic_heal_dialogue_exit(UCMedicHealDialogue*s,const UCMedicHealDialogueServices*h){
 if(!s||!h)return 1;CALL0(stop_aim);CALL0(stop_look);int result=remove_client(s,h);if(result)return result;if(s->locked){CALL(lock,0);s->locked=0;}CALL0(base_exit);return 0;
}
int uc_medic_heal_dialogue_update(UCMedicHealDialogue*s,float dt,const UCMedicHealDialogueServices*h){
 if(!s||!h||!isfinite(dt)||dt<0)return 1;uint8_t value=0;int32_t n=0;
 if(s->phase>=0&&s->phase<=3){CALL(completion,&n);if(n){CALL(boolean,"MedicAutoHeal is healing",0,&value);if(value){CALL(boolean,"MedicAutoHeal is transitioning",0,&value);if(!value)CALL0(loop);}}}
 switch(s->phase){
 case 0:if(s->lock_player&&!s->locked){CALL(lock,1);s->locked=1;}if(look(s,h))return 2;s->phase=1;break;
 case 1:CALL(say,61,50);s->phase=2;break;
 case 2:CALL(create_client,&s->client);s->phase=3;break;
 case 3:if(look(s,h))return 2;CALL(command,s->client,&n);CALL(nearby,300.f,180.f,&value);if(!value)n=6;
  if(n==5||n==6){if(remove_client(s,h))return 2;if(n==6)CALL(say,62,50);CALL0(stop_aim);CALL0(stop_look);
   if(n==5){CALL0(heal_child);s->phase=4;}else s->phase=5;}break;
 case 4:CALL(child,&n);if(n==-1)s->phase=5;break;
 case 5:CALL0(leave);return 0;
 }
 CALL(base,dt);return 0;
}

int uc_medic_heal_order_choose(UCMedicHealOrder*s,int32_t command,void*owner,int(*complete)(void*,int32_t)){
 if(!s)return 1;if(command!=5&&command!=6)return 0;s->previous=s->command=command;
 return !complete||complete(owner,command)?2:0;
}
int uc_medic_heal_order_integer(const UCMedicHealOrder*s,const char*n,int32_t*out,uint8_t*found){
 if(!s||!n||!out||!found)return 1;*found=1;
 if(!strcmp(n,"show order on hud"))*out=0;
 else if(!strcmp(n,"heal player yes")||!strcmp(n,"heal player no")||!strcmp(n,"force orders menu"))*out=1;
 else if(!strcmp(n,"current order"))*out=s->command;
 else *found=0;return 0;
}

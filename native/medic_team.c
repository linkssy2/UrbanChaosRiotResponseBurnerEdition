#include "medic_team.h"
#include <string.h>
int uc_medic_team_join(int32_t *order,const char *const defaults[3],const UCMedicTeamJoin*j,const UCMedicTeamServices*h){
 if(!order||!defaults||!j||!h)return 1;
 if(*order!=-1)return 0;
 if(j->count<0||(j->count&&!j->flags)||!h->heartbeat||!h->client||!h->copy_flags||!h->condition)return 1;
 const char *a=j->heartbeat?j->heartbeat:defaults[0],*b=j->heartbeat?j->joined:defaults[1],*c=j->heartbeat?j->left:defaults[2];
 if(h->heartbeat(h->owner,a,b,c)||h->client(h->owner,order)||h->copy_flags(h->owner,j->count,j->flags,j->quit))return 2;
 for(unsigned i=0;i<3;i++)if(h->condition(h->owner,j->conditions[i]))return 2;
 return 0;
}
int uc_medic_order_choose(UCMedicOrder*s,int32_t value,const UCMedicOrderServices*h){
 if(!s||!h)return 1;
 if(value!=1&&value!=2&&value!=3&&value!=4&&value!=7)return 0;
 if(value==3||value==4){
  if(!h->tutorial||h->tutorial(h->owner,value))return 2;
  if(value==3){int32_t id;uint8_t found;
   if(!h->manager_integer||h->manager_integer(h->owner,"Look Object ID",&id,&found))return 2;
   if(found)s->object=id;
  }
 }
 s->previous=s->command=value;
 return !h->complete||h->complete(h->owner,value)?2:0;
}
int uc_medic_order_integer(const UCMedicOrder*s,const char*name,int32_t*out,uint8_t*found,const UCMedicOrderServices*h){
 if(!s||!h||!name||!out||!found)return 1;
 uint8_t disabled=0,available=0;
 if(!h->root_boolean||h->root_boolean(h->owner,"disable context sensitive",&disabled,&available))return 2;
 *found=1;
 if(!strcmp(name,"follow me")||!strcmp(name,"wait")){*out=1;return 0;}
 if(!strcmp(name,"current order")){*out=s->command;return 0;}
 if(!strcmp(name,"object id")||!strcmp(name,"orders menu shown")){
  if(!h->manager_integer||h->manager_integer(h->owner,!strcmp(name,"object id")?"Look Object ID":name,out,found))return 2;
  return 0;
 }
 if(!strcmp(name,"heal person")||!strcmp(name,"open door")){
  *out=0;const int door=!strcmp(name,"open door");if(door&&available&&disabled)return 0;
  int32_t id=-1,type;uint8_t exists;
  if(!h->manager_integer||h->manager_integer(h->owner,"Look Object ID",&type,&exists))return 2;
  if(exists)id=type;
  if(s->previous==(door?7:3)&&s->object==id)return 0;
  if(h->manager_integer(h->owner,"Look Object Type",&type,&exists))return 2;
  *out=exists&&type==(door?1:3);return 0;
 }
 if(!strcmp(name,"heal player")){
  *out=0;if(s->previous==4)return 0;
  float health;uint8_t exists;
  if(!h->player_health||h->player_health(h->owner,&health,&exists))return 2;
  if(exists&&health>=100.f)return 0;
  int32_t type;if(!h->manager_integer||h->manager_integer(h->owner,"Look Object Type",&type,&exists))return 2;
  *out=exists&&type==4;return 0;
 }
 *found=0;return 0;
}

#include "medic_root_policy.h"
#include <math.h>
#define CALL(n,...)do{if(!h->n||h->n(h->owner,__VA_ARGS__))return 2;}while(0)
#define CALL0(n)do{if(!h->n||h->n(h->owner))return 2;}while(0)
int uc_medic_root_enter(UCMedicRootPolicy*s,void*controller,const UCMedicRootServices*h){
 if(!s||!h||!h->actor_support||!h->character_type)return 1;
 CALL(base_enter,controller);CALL(bank,1,s->bank);
 CALL(body,11,"Assets_CharacterAnimation\\CharacterSpecific\\CCA_Paramedic\\CCA_Paramedic_Animations.sttxt",0,0);
 CALL(initial,s->initial,controller);uintptr_t support=0;CALL(support_create,&support);if(!support)return 2;s->support=support;
 CALL(support_attach,support);*h->actor_support=support;CALL(set_health,800.f);
 s->dialogue_stamp=0.f;s->damage_stamp=0.f;
 int32_t heals;CALL(integer,"heals left",0,&heals);if(heals>0){CALL(tag,"SELECTABLEITEMS",1);}
 if(*h->character_type==4){
  s->cues[0]="Heartbeat-Medic-1";s->cues[1]="Heartbeat-Medic-11";s->cues[2]="Heartbeat-Medic-12";
  s->cues[3]="Obituaries-JaneDelaney-1";s->cues[4]="Obituaries-JaneDelaney-2";s->cues[5]="Obituaries-JaneDelaney-3";s->alternate=1;
 }else if(*h->character_type==5){
  s->cues[0]="Heartbeat-Medic-2";s->cues[1]="Heartbeat-Medic-21";s->cues[2]="Heartbeat-Medic-22";
  s->cues[3]="Obituaries-TeraHart-1";s->cues[4]="Obituaries-TeraHart-2";s->cues[5]="Obituaries-TeraHart-3";s->alternate=1;
 }else{s->cues[0]="Heartbeat-Medic-3";s->cues[1]="Heartbeat-Medic-31";s->cues[2]="Heartbeat-Medic-32";s->alternate=0;}
 int32_t secondary;CALL(secondary_create,"CheckHealPlayer",&secondary);s->secondary=secondary;CALL(start,secondary,controller);
 CALL(watch,&s->put_down,"MedicPutdownBox");CALL(watch,&s->pick_up,"MedicPickupBox");return 0;
}
int uc_medic_root_exit(UCMedicRootPolicy*s,const UCMedicRootServices*h){
 if(!s||!h||!h->actor_support||!h->order)return 1;
 CALL(stop,s->secondary);CALL0(clear_children);if(*h->order!=-1){CALL(flags,(uint32_t)*h->order);}
 if(s->support){CALL(release_support,s->support,1);}*h->actor_support=0;
 CALL(tag,"SELECTABLEITEMS",0);CALL(unwatch,&s->put_down);CALL(unwatch,&s->pick_up);CALL0(base_exit);return 0;
}
int uc_medic_root_damage(UCMedicRootPolicy*s,uint32_t flags,float amount,const UCMedicRootServices*h){
 if(!s||!h||!h->delta||!isfinite(*h->delta)||!isfinite(s->dialogue_stamp)||!isfinite(s->damage_stamp))return 1;
 const float delta=*h->delta,limit=s->damage_stamp+1.f;
 if(delta>limit){if(!(flags&0x40000)&&delta>s->dialogue_stamp+8.f){CALL(dialogue,"Medic: Help me!",-1);s->dialogue_stamp=*h->delta;}}
 else{s->damage_stamp=delta;CALL(damage,flags,amount);}return 0;
}
int uc_medic_plain_enter(UCMedicRootPolicy*s,int32_t source,int32_t*speech,void*controller,const UCMedicRootServices*h){
 if(!s||!speech||!h||!h->actor_support||!h->character_type)return 1;
 CALL(base_enter,controller);CALL(bank,1,s->bank);
 CALL(body,11,"Assets_CharacterAnimation\\CharacterSpecific\\CCA_Paramedic\\CCA_Paramedic_Animations.sttxt",0,0);
 CALL(initial,s->initial,controller);uintptr_t support=0;CALL(support_create,&support);if(!support)return 2;s->support=support;
 CALL(support_attach,support);*h->actor_support=support;CALL(set_health,800.f);
 if(*h->character_type==4){s->cues[0]="Heartbeat-Medic-1";s->cues[3]="Obituaries-JaneDelaney-1";s->cues[4]="Obituaries-JaneDelaney-2";s->cues[5]="Obituaries-JaneDelaney-3";s->alternate=1;}
 else if(*h->character_type==5){s->cues[0]="Heartbeat-Medic-2";s->cues[3]="Obituaries-TeraHart-1";s->cues[4]="Obituaries-TeraHart-2";s->cues[5]="Obituaries-TeraHart-3";s->alternate=1;}
 else{s->cues[0]="Heartbeat-Medic-3";s->alternate=0;}
 CALL(authored_secondary,source,controller,speech);
 int32_t secondary;CALL(secondary_create,"CheckHealPlayer",&secondary);s->secondary=secondary;CALL(start,secondary,controller);
 CALL(watch,&s->put_down,"MedicPutdownBox");CALL(watch,&s->pick_up,"MedicPickupBox");return 0;
}
int uc_medic_plain_exit(UCMedicRootPolicy*s,int32_t speech,const UCMedicRootServices*h){
 if(!s||!h||!h->actor_support)return 1;
 CALL(stop,speech);CALL(stop,s->secondary);
 if(s->support){CALL(release_support,s->support,1);}*h->actor_support=0;
 CALL(unwatch,&s->put_down);CALL(unwatch,&s->pick_up);CALL0(base_exit);return 0;
}

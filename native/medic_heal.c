#include "medic_heal.h"
#include <math.h>
#define CALL(f,...) do{if(!h->f||h->f(h->owner,__VA_ARGS__))return 2;}while(0)
#define BOOL(v,f,...) do{if(!h->f)return 2;v=h->f(h->owner,__VA_ARGS__);if(v<0)return 2;}while(0)
#define BOOL0(v,f) do{if(!h->f)return 2;v=h->f(h->owner);if(v<0)return 2;}while(0)
void uc_medic_heal_init(UCMedicHealCheck*s){if(s){s->state=0;s->child=-1;s->elapsed=0;}}
int uc_medic_heal_update(UCMedicHealCheck*s,float delta,const UCMedicHealServices*h){
 if(!s||!h||!isfinite(s->elapsed)||!isfinite(delta)||delta<0)return 1;
 if(s->elapsed<1.f){if(!h->delta)return 2;double dt=h->delta(h->owner);if(!isfinite(dt)||dt<0)return 2;s->elapsed=(float)(dt+s->elapsed);return 0;}
 int found;if(s->child!=-1){BOOL(found,exists,s->child);if(!found)s->child=-1;}
 int32_t heals;CALL(heals,&heals);
 int nearby;BOOL(nearby,nearby,300.f,15.f);if(nearby)BOOL0(nearby,visible);
 BOOL0(found,player);if(found){
  if(!h->health)return 2;double health=h->health(h->owner);if(!isfinite(health))return 2;
  if(health>0){BOOL(found,property,0,"allow heal",1);if(found){BOOL(found,property,1,"incapacitated",0);if(!found){
   if(s->state==1){if(s->child==-1&&!nearby)s->state=0;}
   else if(s->state==0&&nearby){
    BOOL(found,property,2,"can heal",1);if(found){BOOL0(found,dialogue_busy);if(!found){
     if(!h->percent_health)return 2;double percent=h->percent_health(h->owner);if(!isfinite(percent))return 2;
     if(percent<100.){
      if(heals>0){int32_t child;CALL(spawn,"MedicHealPlayer",&child);s->child=child;CALL(queue,child);}
      else CALL(emote,0x4c,0x32);
      s->state=1;
     }
    }}
   }
  }}}
 }
 CALL(base,delta);return 0;
}

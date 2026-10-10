#include "order_manager.h"
#include <math.h>
#define CALL(f,...) do{if(!h->f||h->f(h->owner,__VA_ARGS__))return 2;}while(0)
#define CALL0(f) do{if(!h->f||h->f(h->owner))return 2;}while(0)
void uc_order_manager_init(UCOrderManager*s){if(s){*s=(UCOrderManager){0};s->forced_client=-1;}}
int uc_order_manager_select(UCOrderManager*s,const UCOrderManagerServices*h,const UCOrderManagerAction*a,
 uint32_t count,uint32_t clients,int32_t parent,uint32_t pad,uint32_t*eligible,uint32_t capacity){
 if(!s||!h||count>8192||capacity<count||(count&&(!a||!eligible))||!isfinite(s->remaining))return 1;
 for(uint32_t i=0;i<count;++i)if(!a[i].name||a[i].slot>3)return 1;
 CALL0(look);s->available=s->forcing=s->held=0;s->forced_client=-1;uint8_t forced=0;
 for(uint32_t i=0;i<count;++i){uint8_t yes=0;CALL(hud,a[i].hud,0);
  if(a[i].contextual){CALL(property,a[i].client,a[i].name,0,&yes);
   if(yes){CALL(veto,a[i].name,&yes);if(yes)s->available=1;}}
  CALL(property,a[i].client,"force orders menu",0,&yes);
  if(yes){s->remaining=3;s->forcing=1;s->forced_client=a[i].client;forced=1;}
 }
 uint8_t control=0;CALL(control,&control);if(!control){CALL(hud,pad,0);return 0;}
 uint32_t n=0;
 for(uint32_t i=0;i<count;++i){if(s->forced_client!=-1&&s->forced_client!=a[i].client)continue;
  uint8_t yes=0;CALL(hud,a[i].hud,0);CALL(property,a[i].client,a[i].name,0,&yes);
  if(yes){CALL(veto,a[i].name,&yes);if(yes){eligible[n++]=i;if(a[i].contextual&&s->opened)s->remaining=3;}}
 }
 uint8_t valid=0,blocked=0;CALL(property,parent,"orders valid",1,&valid);
 if(!valid){s->invalid_last=1;blocked=1;}
 else if(s->invalid_last){s->invalid_last=0;blocked=1;}
 else if(clients){uint8_t gameplay=0;CALL(gameplay,&gameplay);if(gameplay){double value=0;CALL(input,31,0,&value);if(value!=0)s->held=1;
  if(!(s->remaining>0)){uint8_t pressed=0;const uint32_t queries[]={30,32,33,31};
   for(unsigned i=0;i<4&&!pressed;++i){CALL(input,queries[i],i==3?0:2,&value);pressed=value!=0;}
   if(pressed){s->remaining=3;s->opened=s->forcing=1;}
  }
 }}
 if(!s->opened&&!forced)s->remaining=0;
 if(!blocked){if(s->remaining>0)for(uint32_t i=0;i<n;++i){CALL(hud,a[eligible[i]].hud,1);}
  for(uint32_t i=0;i<n;++i){uint8_t gameplay=0;CALL(gameplay,&gameplay);if(!gameplay)continue;
   const UCOrderManagerAction*v=&a[eligible[i]];double value=0;CALL(input,30+v->slot,2,&value);if(value==0)continue;
   CALL(hud,v->hud,2);s->remaining=0;CALL(send,v->client,v->command);CALL0(sound);
   if(s->forced_client!=-1)CALL0(forced);CALL(prompt,i);
  }
 }
 if(s->remaining>0){double delta;CALL(delta,&delta);s->remaining=(float)((double)s->remaining-delta);}
 if(!(s->remaining>0))s->opened=0;
 CALL(hud,pad,(uint8_t)(s->available&&s->forced_client==-1));return 0;
}
#undef CALL
#undef CALL0

#include "wave_controller.h"
typedef struct Slots {const UCLevelEntitySlots*slots;void*owner;int(*status)(void*,int32_t,uint32_t*);} Slots;
static int lookup(void*p,int32_t index,int32_t*id){return uc_level_entity_lookup(((Slots*)p)->slots,(uint32_t)index,id);}
static int status(void*p,int32_t id,uint32_t*flags){Slots*s=p;return s->status(s->owner,id,flags);}
int uc_wave_members_complete_slots(UCWaveMembers*w,const UCLevelEntitySlots*slots,void*owner,int(*read)(void*,int32_t,uint32_t*),uint8_t*done){
 if(!slots||!read)return 1;Slots s={slots,owner,read};UCWaveQuery q={&s,lookup,status};return uc_wave_members_complete(w,&q,done);
}
int uc_wave_members_complete(UCWaveMembers*w,const UCWaveQuery*q,uint8_t*done){
 if(!w||!q||!done||w->count>32||!q->lookup||!q->status)return 1;
 for(int32_t i=0;i<w->count;i++)if(w->active[i]){
  int32_t id;if(q->lookup(q->owner,w->source[i],&id))return 2;
  if(id!=-1&&id!=-2){
   uint32_t flags;if(q->status(q->owner,id,&flags))return 2;
   if(!(flags&0x20007u)){*done=0;return 0;}
  }
  w->active[i]=0;
 }
 *done=1;return 0;
}

#include "character_root_update.h"
#include <math.h>
int uc_character_root_update(UCCharacterRootUpdate*s,uint8_t hurt,uint8_t regen,const UCCharacterRootUpdateServices*h){
 if(!s||!s->status||!h||!h->health||!h->maximum||!h->common||!isfinite(s->damage_time))return 1;
 float before=(float)h->health(h->owner);if(!isfinite(before)||h->common(h->owner))return 2;
 float after=(float)h->health(h->owner);if(!isfinite(after))return 2;
 if(after>0&&before>after){
  float half=(float)(h->maximum(h->owner)*.5),quarter=(float)(h->maximum(h->owner)*.25);
  unsigned code=quarter>after&&before>quarter?0x30:half>after&&before>half?0x2f:s->order==-1?8:10;
  if(!h->emote||h->emote(h->owner,code,0x32))return 2;
 }
 if(s->order!=-1){float health=(float)h->health(h->owner);if(h->maximum(h->owner)*.25>health){
  if(!h->emote||h->emote(h->owner,0x1b,0x32))return 2;}}
 if(!s->restrained&&(*s->status&0x20004)){s->restrained=1;s->restraint_time=3.5f;}
 if(hurt&&s->damage_time<2.f)*s->status|=0x8000;else *s->status&=~0x8000u;
 if(regen&&s->damage_time>10.f&&!(*s->status&1)){
  if(s->regeneration_flag!=-1){if(!h->flag)return 2;int enabled=h->flag(h->owner,s->regeneration_flag);if(enabled<0)return 2;if(!enabled)return 0;}
  float max=(float)h->maximum(h->owner);double current=h->health(h->owner);float value=(float)current;
  if(max>current){if(!h->delta||!h->set_health)return 2;double delta=h->delta(h->owner);if(!isfinite(delta)||delta<0)return 2;
   if(h->set_health(h->owner,(float)(delta*5.+value)))return 2;}
 }
 return 0;
}

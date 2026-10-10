#include "character_heartbeat.h"
#include <math.h>
int uc_character_heartbeat_enter(const UCCharacterHeartbeat*s,const UCCharacterHeartbeatServices*h){
 if(!s||!h||!h->enter||!h->health||!h->maximum||s->count<0||(s->count&&!s->flags)||!isfinite(s->percent))return 1;
 if(h->enter(h->owner))return 2;
 if(s->count>0){if(!h->root_exists)return 2;int found=h->root_exists(h->owner);if(found<0)return 2;
  if(found&&(!h->regeneration_gate||h->regeneration_gate(h->owner,s->flags[0])))return 2;}
 float health=(float)h->health(h->owner);double maximum=h->maximum(h->owner);
 if(((double)health/maximum)*100.>s->percent){if(!h->set_health)return 2;
  double max_again=h->maximum(h->owner);if(h->set_health(h->owner,(float)((max_again*(double).01f)*s->percent)))return 2;}
 return 0;
}
int uc_character_heartbeat_update(const UCCharacterHeartbeat*s,uint8_t expensive,const UCCharacterHeartbeatServices*h){
 if(!s||!s->text||!h||!h->monitor_ready)return 1;int ready=h->monitor_ready(h->owner);if(ready<0)return 2;
 if(!ready)return !h->base||h->base(h->owner,expensive)?2:0;
 const char*text=s->text;if(!*text){if(!h->resource)return 2;int found=h->resource(h->owner,&text);if(found<0)return 2;if(!found)text="Heartbeat-None-1";}
 if(!text||!h->show||h->show(h->owner,text)||!h->transition)return 2;
 int changed=h->transition(h->owner,s->next);if(changed<0)return 2;
 if(!changed&&(!h->exit||h->exit(h->owner)))return 2;return 0;
}
void uc_character_invulnerable(uint32_t*flags,int32_t*suppression,uint8_t enemy,uint8_t player,uint8_t reactions,uint8_t tazer){
 *flags&=0xfffffff4u;if(player)*flags|=1;if(enemy)*flags|=2;
 if(!reactions)*suppression=(int32_t)((uint32_t)*suppression+1);else if(*suppression>0)--*suppression;
 if(tazer)*flags|=4;if(player||enemy)*flags|=8;
}

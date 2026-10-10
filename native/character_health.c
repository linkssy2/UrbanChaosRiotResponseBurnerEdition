#include "character_health.h"
#include <math.h>
#include <stdio.h>
#define CALL(name,...) do{if(!h->name||h->name(h->owner,__VA_ARGS__))return 2;}while(0)
#define CALL0(name) do{if(!h->name||h->name(h->owner))return 2;}while(0)
int uc_character_hit_count(UCCharacterHitStatistics*s,uint32_t frame,unsigned offset,int eligible){
 if(!s||!s->values||(offset!=8&&offset!=12&&offset!=16&&offset!=20))return 1;if(!eligible)return 0;
 if(s->last_frame==frame){if(offset!=8||s->last_offset==8)return 0;
  if(s->last_offset!=12&&s->last_offset!=16&&s->last_offset!=20)return 1;
  --s->values[s->last_offset/4];++s->values[2];s->last_offset=8;
 }else{s->last_frame=frame;++s->values[offset/4];s->last_offset=offset;++s->values[16];}return 0;
}
int uc_character_health(UCCharacterHealth*s,uint32_t flags,float damage,const UCCharacterHealthServices*h){
 if(!s||!h||!isfinite(damage))return 1;if(flags&0x80000)return 0;
 if(!h->health||!h->actor_status||!h->death_kind||!isfinite(*h->health)||!isfinite(s->elapsed))return 1;
 uint8_t yes=0;CALL(boolean,"no damage",&yes);int allowed=!yes;uint32_t kind=flags&0x1f0feu;if(kind==0x2000)allowed=0;
 const int alive=*h->health>0,from_player=(flags&0x40000)!=0,player_enemy=from_player&&s->enemy;int shot=0;
 if(player_enemy){
  if(flags&2){CALL(stat,"SuccessfulMelee",1.f);}
  else if(kind==0x2000){CALL(stat,"SuccessfulTaser",1.f);}
  else if(!(flags&0x1000000)){shot=1;CALL(stat,"SuccessfulShot",1.f);}
  if(flags&0x20000){CALL(boolean,"hard to shoot",&yes);if(!yes){
   CALL(body,UC_HEALTH_OUTER94,&yes);if(yes){CALL(body,UC_HEALTH_OUTER88,&yes);if(yes){CALL(body,UC_HEALTH_INNER78,&yes);if(!yes){
    CALL(body,UC_HEALTH_STANCE14,&yes);if(yes){CALL(difficulty,1);}
    else{CALL(body,UC_HEALTH_STANCE18,&yes);if(yes){int32_t player,target;CALL(player,&player);CALL(target,&target);if(player==target){CALL(difficulty,0);}}}
   }}}
  }}
 }else if(from_player){
  if(!(*h->actor_status&0x32257)){if(!s->special){CALL(dialogue,(flags&0x400000)?0x49u:kind==0x2000?0x21u:0x20u,75);}
   else if(kind!=0x2000){CALL(dialogue,0x80000223u,75);}}
  if(flags&0x200000)damage*=.5f;
 }
 switch(kind){
 case 4:case 0x10:
  if(player_enemy){CALL(stat,"LegShot",1.f);CALL(counter,20,shot);}damage*=.6f;
  if(allowed){CALL(set_health,(float)((double)*h->health-damage));}break;
 case 8:case 0x20:
  if(player_enemy){CALL(stat,"ArmShot",1.f);CALL(counter,16,shot);}damage*=.7f;
  if(allowed){CALL(set_health,(float)((double)*h->health-damage));}break;
 case 2:case 0x40:case 0x4000:
  if(player_enemy){CALL(stat,"TorsoShot",1.f);CALL(counter,12,shot);}
  if(allowed){CALL(set_health,(float)((double)*h->health-damage));}break;
 case 0x80:
  if(allowed){
   int32_t player,target;int close=0;CALL(player,&player);CALL(player_integer,player,"target id",&target);
   if(target==h->id){CALL(player_integer,player,"target surface",&target);if(target==9){double distance;CALL(player_scalar,player,"target distance",&distance);close=650.>distance;}}
   if(player_enemy&&(!(flags&0x200000)||close)){
    if(s->elapsed>.25f&&shot){
     CALL(stat,"HeadShot",1.f);CALL(counter,8,shot);char text[16];CALL(headshot_text,text);int32_t count,goal;CALL(counter_value,8,&count);CALL(headshot_goal,&goal);
     if(count==goal){CALL(message,3,"Objectives-HUD-5","NewFrontEnd-Objectives-12",text,1);s->headshot_award=1;}
     else{CALL(counter_value,8,&count);CALL(headshot_goal,&goal);
      if(count<goal){CALL(message,1,"HUD-Headshot-1","NewFrontEnd-Objectives-12",text,1);}
      else{CALL(counter_value,8,&count);snprintf(text,sizeof text,"%i",count);CALL(message,2,"HUD-Headshot-1","HUD-ScoreScreen-21",text,1);}}
     CALL(sound,"Banks\\Impacts.amp","Headshot");
     if(!h->gore_disabled&&(flags&0x2000000)){*h->death_kind=close?10:9;
      CALL(effect,"Bip01 Neck","bullet_hit\\bloodheadshot.simpleparticlesystem");CALL(effect,"Bip01 Neck","bullet_hit\\bloodheadshot2.simpleparticlesystem");
     }else *h->death_kind=8;
    }
    damage=10000;CALL(set_health,-1.f);
   }else{
    if(player_enemy){CALL(counter,12,shot);}damage=(float)((double)damage*1.5f);CALL(set_health,(float)((double)*h->health-damage));
   }
  }break;
 case 0x1000:
  if(allowed){if(s->order!=-1){CALL(set_health,(float)((double)*h->health-(double)damage*.75f));}
   else{yes=s->enemy;if(!yes){CALL(special_actor,&yes);}if(yes){if(s->mode==5){CALL0(fatal_interrupt);}CALL(set_health,-1.f);}
    else{CALL(set_health,(float)((double)*h->health-damage));}}}break;
 default:break;
 }
 if(alive){yes=(uint8_t)(allowed&&damage>0);if(!yes){CALL(boolean,"do damage indicator",&yes);}
  if(yes&&from_player){int32_t player;CALL(player,&player);CALL(indicator,player,damage,(uint8_t)((flags&0x11000)!=0));}}
 if(!s->enemy&&from_player&&*h->health<=0){s->friendly_hurt=1;s->friendly_hurt_time=3.5f;}
 if(player_enemy)s->elapsed=0;
 if(*h->health<=0&&player_enemy&&alive){
  CALL(stat,"SuccessfulKill",1.f);CALL0(increment_kills);
  if(flags&0x400000){CALL(notice,"HUD-Headshot-2");}if(flags&0x800000){CALL(notice,"HUD-Headshot-7");}
  int32_t type;CALL(enemy_type,&type);if(type!=-1){CALL(dda_kill,type);}
 }return 0;
}

#include "medic_heal_player.h"
#include <math.h>
#include <string.h>
#define CALL(n,...)do{if(!h->n||h->n(h->owner,__VA_ARGS__))return 2;}while(0)
#define CALL0(n)do{if(!h->n||h->n(h->owner))return 2;}while(0)
static float distance(const float*a,const float*b){float x=a[0]-b[0],y=a[1]-b[1],z=a[2]-b[2];return (x*x+y*y)+z*z;}
void uc_medic_heal_pillbox_matrix(const float*a,const float*b,float*out){for(unsigned r=0;r<4;r++)for(unsigned c=0;c<4;c++)out[r*4+c]=(a[r*4]*b[c]+a[r*4+1]*b[4+c])+(a[r*4+2]*b[8+c]+a[r*4+3]*b[12+c]);}
int uc_medic_heal_player_update(UCMedicHealPlayer*s,float dt,const UCMedicHealPlayerServices*h){
 if(!s||!h||!isfinite(dt)||dt<0)return 1;float player[3];CALL(player,player);
 int32_t done=0,n=0;uint8_t value=0;
 switch(s->phase){
 case 0:CALL(actor,s->origin);CALL(say,63,50);s->phase=1;CALL0(stop_aim);break;
 case 1:CALL(boolean,0,"MedicAutoHeal is transitioning",0,&value);if(!value){s->phase=2;CALL(boolean,0,"MedicAutoHeal is healing",0,&value);if(value)CALL(clip,1);}break;
 case 2:CALL(completion,&done);if(done)s->phase=3;break;
 case 3:{float actor[3];CALL0(stop);CALL(actor,actor);float d=distance(actor,player);CALL(boolean,1,"move to heal player",1,&value);
  if(value&&d>40000.f){memcpy(s->last_player,player,12);CALL(nav_target,player);s->phase=4;}else s->phase=5;break;}
 case 4:{float actor[3];CALL(actor,actor);float d=distance(actor,player);int32_t status=0;
  if(!(d<40000.f))CALL(run,&status);
  if(d<40000.f||status==4){CALL0(nav_stop);CALL0(stop);s->phase=5;}
  else if(distance(s->origin,player)>490000.f){CALL0(cancel_speech);CALL(say,62,50);CALL0(stop);CALL0(leave);return 0;}
  else if(distance(player,s->last_player)>40000.f){memcpy(s->last_player,player,12);CALL(nav_target,player);}break;}
 case 5:CALL(aim,player,3);s->phase=6;break;
 case 6:CALL(completion,&done);if(done){CALL(event,87);CALL(clip,0);CALL(random_pair,&n);CALL(say,n?67:66,50);s->phase=7;}break;
 case 7:CALL(completion,&done);if(done){CALL(event,86);CALL0(stop);CALL(heals,&n);CALL(say,n==1?69:n==2?68:70,50);CALL0(leave);return 0;}break;
 }
 if(s->phase==7&&s->spawn&&s->pillbox==-1){float bone[16],world[16],matrix[16];CALL(bone_world,bone,world);uc_medic_heal_pillbox_matrix(bone,world,matrix);CALL(pillbox,matrix,&s->pillbox);s->spawn=0;}
 if(s->delivered&&s->pillbox!=-1){CALL0(delivered);CALL(remove,s->pillbox);s->pillbox=-1;s->delivered=0;}
 CALL(base,dt);return 0;
}

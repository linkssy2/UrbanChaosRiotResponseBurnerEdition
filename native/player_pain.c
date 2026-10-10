#include "player_pain.h"
#include <math.h>
#include <string.h>
void uc_player_pain_init(UCPlayerPain*s){if(s)memset(s,0,sizeof *s);}
int uc_player_pain_trigger(UCPlayerPain*s,unsigned channel){
 if(!s||channel>1)return 1;
 s->channel[channel][0]=.8f;s->channel[channel][1]=channel?.2666666805744171142578125f:1.6f;
 s->channel[channel][2]=channel?3.f:0.f;return 0;
}
int uc_player_pain_step(UCPlayerPain*s,float dt,void*owner,int(*blur)(void*,float,float,float)){
 if(!s||!blur||!isfinite(dt)||dt<0||dt>1)return 1;
 for(unsigned i=0;i<2;i++)for(unsigned k=0;k<3;k++)if(!isfinite(s->channel[i][k]))return 1;
 for(unsigned i=0;i<2;i++){
  float*c=s->channel[i];if(c[0]<=0)continue;
  if(c[2]<=0){double a=(double)c[0]-(double)dt*c[1];c[0]=(float)a;if(a<0)c[0]=0;}
  else c[2]=(float)((double)c[2]-dt);
  if(blur(owner,-150,600,c[0]))return 2;
 }return 0;
}

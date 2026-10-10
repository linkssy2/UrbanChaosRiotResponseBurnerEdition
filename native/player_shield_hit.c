#include "player_shield_hit.h"
#include "point_light_receiver.h"
#include "firstperson_motion.h"
#include <math.h>
#include <string.h>
static int finite_values(const float*p,unsigned count){for(unsigned n=0;n<count;n++)if(!isfinite(p[n]))return 0;return 1;}
static double random_range(UCGameRandom*r,float a,float z){int32_t draw;uc_game_random_next(r,&draw);return((double)draw*((double)z-a))*0x1p-31+a;}
int uc_player_shield_recoil(UCPlayerShieldRecoil*s,UCPlayerImpactState*k,const float frame[16],unsigned type,const float v[3],UCGameRandom*r){
 static const float amount[]={.2f,1,2,2},shake[]={.3f,0,1,1.5f},impact[]={0,0,0,1000};
 if(!s||!k||!frame||!v||!r||r->first>=250||r->second>=250||type>3||!finite_values(v,3)||!finite_values(&s->horizontal,3))return 1;
 float world[16],inverse[16];memcpy(world,frame,48);world[12]=world[13]=world[14]=0;world[15]=1;
 if(!uc_point_light_inverse(world,inverse))return 1;
 float x=(inverse[0]*v[0]+inverse[4]*0.f)+(inverse[8]*v[2]+inverse[12]);
 float z=(inverse[2]*v[0]+inverse[6]*0.f)+(inverse[10]*v[2]+inverse[14]);
 if(sqrt((double)x*x+(double)z*z)<.001f){x=1;z=0;}
 float length=(float)(1./sqrt(((double)x*x+(double)z*z)+0.));
 s->horizontal=(length*x)*amount[type];s->vertical=(length*z)*amount[type];
 if(shake[type]!=0){k->yaw=(float)(random_range(r,25,50)*shake[type]);if(random_range(r,0,1)>.5f)k->yaw=0.f-k->yaw;k->pitch=(float)(random_range(r,-30,-30)*shake[type]);}
 if(s->impact<100){s->impact+=impact[type];if(s->impact>100)s->impact=115;}
 return 0;
}
int uc_player_shield_surface(UCPlayerShieldMarks*s,const float frame[16],UCGameRandom*r,float out[3]){
 if(!s||!frame||!r||!out||s->count>UC_PLAYER_SHIELD_MARKS||r->first>=250||r->second>=250||!finite_values(frame,16))return 1;
 float size=(float)random_range(r,2,3.5f),x=(float)random_range(r,(float)((double)size-29),(float)(29.-size)),y=(float)random_range(r,size,(float)(50.-size));
 float angle=(float)random_range(r,0,6.2831854820251465f),alpha=(float)random_range(r,.5f,1);
 if(s->count<UC_PLAYER_SHIELD_MARKS){
  float cs=(float)cos((double)angle),sn=(float)sin((double)angle),negative=0.f-size;
  float corner[4][2]={{negative,negative},{size,negative},{negative,size},{size,size}};UCPlayerShieldMark*m=s->marks+s->count++;
  for(unsigned n=0;n<4;n++){m->point[n][0]=x+(corner[n][0]*cs-corner[n][1]*sn);m->point[n][1]=y+(corner[n][0]*sn+corner[n][1]*cs);m->point[n][2]=1;}
  m->alpha=alpha;
 }
 for(unsigned n=0;n<3;n++)out[n]=(frame[n]*x+frame[4+n]*y)+(frame[8+n]*0.f+frame[12+n]);
 return 0;
}
int uc_player_shield_recover(UCPlayerShieldRecoil*s,float*weight){
 if(!s||!weight||!finite_values(&s->horizontal,3))return 1;
 s->horizontal=s->horizontal-s->horizontal*.2f;s->vertical=s->vertical-s->vertical*.2f;
 if(s->horizontal>-.1f&&s->horizontal<.1f)s->horizontal=0;
 if(s->vertical>-.1f&&s->vertical<.1f)s->vertical=0;
 if(s->impact>100&&s->impact-.5f<=100)s->impact=0;
 float amount=(s->impact-100)*.06666667014360428f;if(amount<0)amount=0;
 float inverse=1-amount;*weight=1-inverse*inverse;
 s->impact=s->impact-.5f;if(s->impact<0)s->impact=0;return 0;
}
int uc_player_hit_angles(UCPlayerImpactState*k,float*rate,const UCPlayerShieldRecoil*s,float delta,float angles[3]){
 if(!k||!rate||!s||!angles||!finite_values(&k->intensity_x,4)||!finite_values(&s->horizontal,3)||!finite_values(angles,3)||!isfinite(delta)||delta<0)return 1;
 unsigned active=s->horizontal!=0||s->vertical!=0;
 UCMotionSmoother intensity={k->intensity_x,k->intensity_y,active?15.f:5.f};
 if(uc_motion_smoother_step(&intensity,active?.2f:0,delta))return 1;
 if(active)intensity.current=.2f;
 k->intensity_x=intensity.current;k->intensity_y=intensity.target;*rate=intensity.rate;
 float yaw=(float)(((double)delta*k->yaw*intensity.current)*.01745329238474369f);
 angles[0]=(float)((((double)delta*k->pitch*intensity.current)*.01745329238474369f)+angles[0]);
 angles[1]=(float)((double)yaw+angles[1]);return 0;
}
int uc_player_shield_spark_sequence(const UCShieldSparkServices*h){
 static const char*names[]={"Shield-bullethitsparkbase.simpleparticlesystem","Shield-bullethitspark1.simpleparticlesystem","Shield-bullethitspark2.simpleparticlesystem","Shield-bullethitspark3.simpleparticlesystem"};
 if(!h||!h->world||!h->feedback||!h->particles||!h->yield||!h->delta||!h->remove)return 1;
 if(h->feedback(h->owner,h->world+12,1000,.05f,1,3))return 2;
 for(unsigned i=0;i<4;i++)if(h->particles(h->owner,names[i],h->world))return 2;
 float elapsed=0;for(;;){if(h->yield(h->owner))return 2;double value=h->delta(h->owner)+elapsed;elapsed=(float)value;if(value>=3)break;}
 if(h->remove(h->owner)||h->yield(h->owner))return 2;return 0;
}

int uc_player_shield_bind(const UCModel*m,int32_t*out){
 static const uint8_t name[16]="shieldmain";
 if(!m||!out||!m->bones||!m->bone_count||m->bone_count>4096)return 0;
 int32_t found=-1;
 for(uint32_t i=0;i<m->bone_count;i++){
  const uint8_t*p=m->bones[i].source_metadata+4;unsigned j;
  for(j=0;j<16;j++){unsigned a=p[j];if(a>='A'&&a<='Z')a+=32;if(a!=name[j])break;if(!a)break;}
  if(j==16||(!p[j]&&!name[j])){found=(int32_t)i;break;}
 }
 *out=found;return 1;
}
int uc_player_shield_geometry(const UCPlayerShieldMarks*s,unsigned first,unsigned count,const float*m,float shade,int32_t item,int32_t action,uint8_t dim,UCParticleQuad*out){
 if(!s||!m||(!out&&count)||s->count>300||first>s->count||count>s->count-first||dim>1||!finite_values(m,16)||!isfinite(shade))return 1;
 if(dim)shade*=.1f;
 for(unsigned i=0;i<count;i++){const UCPlayerShieldMark*a=s->marks+first+i;UCParticleQuad*q=out+i;
  for(unsigned j=0;j<4;j++)for(unsigned k=0;k<3;k++)q->position[j][k]=(m[k]*a->point[j][0]+m[4+k]*a->point[j][1])+(m[8+k]*a->point[j][2]+m[12+k]);
  if(item==0&&action!=12&&action!=13){q->color[0]=q->color[1]=q->color[2]=shade;q->color[3]=a->alpha;}else memset(q->color,0,16);
 }return 0;
}

#include "character_attention.h"
#include <math.h>
#include <string.h>
static float distance_squared(const float*a,const float*b){float x=a[0]-b[0],y=a[1]-b[1],z=a[2]-b[2];return(z*z+y*y)+x*x;}
static int inside(const UCCharacterAttention*s,int32_t target,float radius,const UCCharacterAttentionServices*h){
 float world[16],point[3];int result;if(target==-1)return 0;result=h->world(h->owner,target,world);if(result<=0)return result;
 result=h->head(h->owner,target,point);if(result!=1)return-1;result=h->world(h->owner,s->actor,world);if(result!=1)return-1;
 return radius*radius>distance_squared(world+12,point);
}
int uc_character_attention_init(UCCharacterAttention*s,int32_t actor,const UCCharacterAttentionServices*h){
 float world[16];if(!s||!h||!h->world)return 1;memset(s,0,sizeof*s);s->target=-1;s->actor=actor;
 return h->world(h->owner,actor,world)<0?1:0;
}
void uc_character_attention_reset(UCCharacterAttention*s){s->target=-1;s->elapsed=0;}
int uc_character_attention_step(UCCharacterAttention*s,const UCCharacterAttentionServices*h){
 int result;if(!s||!h||!h->world||!h->head)return 1;s->elapsed=(float)(s->elapsed+h->delta);if(s->target==-1)return 0;
 result=inside(s,s->target,600,h);if(result<0)return 1;if(result&&h->head(h->owner,s->target,s->point)!=1)return 1;return 0;
}
int uc_character_attention_near(const UCCharacterAttention*s,const UCCharacterAttentionServices*h){
 float world[16];if(!s||!h||!h->world)return-1;if(s->target==-1)return 0;
 if(h->world(h->owner,s->actor,world)!=1)return-1;return 360000.f>distance_squared(world+12,s->point);
}
int uc_character_attention_select(UCCharacterAttention*s,int32_t target,float radius,const UCCharacterAttentionServices*h){
 float world[16],point[3],d[3],inverse,dot;int result;if(!s||!h||!h->world||!h->head)return-1;
 if(h->world(h->owner,s->actor,world)!=1||h->head(h->owner,target,point)!=1)return-1;
 for(unsigned i=0;i<3;i++)d[i]=point[i]-world[12+i];
 inverse=(float)(1./sqrt((double)d[2]*d[2]+(double)d[1]*d[1]+(double)d[0]*d[0]));
 for(unsigned i=0;i<3;i++)d[i]*=inverse;dot=(world[10]*d[2]+world[9]*d[1])+world[8]*d[0];
 if(!(dot>0))return 0;result=inside(s,target,radius,h);if(result<=0)return result;
 s->target=target;s->elapsed=0;memcpy(s->point,point,12);return 1;
}

int uc_character_dialogue_zone(int32_t actor,const UCCharacterZoneServices*h){
 static const char*names[2]={"HOT ZONE","COLD ZONE"};static const uint32_t events[2]={107,28};
 if(!h||!h->named||!h->query)return-1;
 for(unsigned group=0;group<2;group++){
  const int32_t*ids=0;uint32_t count=0,index=0;
  if(h->named(h->owner,names[group],&ids,&count)||(count&&!ids))return-1;
  while(index<count){uint8_t handled=0,reply=0;
   if(h->query(h->owner,ids[index],events[group],actor,&handled,&reply))return-1;
   if(handled&&reply)return(int)group+1;
   ++index;if(h->named(h->owner,names[group],&ids,&count)||(count&&!ids))return-1;
  }
 }
 return 0;
}

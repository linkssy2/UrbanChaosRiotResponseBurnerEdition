#include "character_look.h"
#include <math.h>
#include <string.h>
#include <float.h>
static int random_valid(const UCGameRandom*r){return r&&r->first<250&&r->second<250;}
static double random_value(UCGameRandom*r,float a,float b){int32_t draw=0;uc_game_random_next(r,&draw);return((double)b-a)*(double)draw*4.6566128730773926e-10+a;}
int uc_character_look_init(UCCharacterLook*s){if(!s)return 1;s->remaining=0;s->last_frame=-1;s->target=-1;s->nearby=0;s->direction_known=0;return 0;}
int uc_character_look_target(UCCharacterLook*s,UCGameRandom*r,int32_t id){if(!s||!random_valid(r))return 1;s->target=id;s->remaining=(float)random_value(r,.2f,1);return 0;}
static int actor(const UCCharacterLookServices*h,int32_t id,UCCharacterLookActor*a,uint8_t*found){
    *found=0;if(h->actor(h->owner,id,a,found)||*found>1)return 1;if(!*found)return 0;
    if(a->has_head>1)return 1;for(unsigned i=0;i<16;i++)if(!isfinite(a->world[i]))return 1;
    if(a->has_head)for(unsigned i=0;i<3;i++)if(!isfinite(a->head[i]))return 1;return 0;
}
static int difference(const UCCharacterLookServices*h,int32_t id,const float*p,const float*offset,float*d,uint8_t*found){
    UCCharacterLookActor a;if(actor(h,id,&a,found))return 1;if(!*found)return 0;
    for(unsigned i=0;i<3;i++)d[i]=a.has_head?a.head[i]-(p[i]+offset[i]):a.world[12+i]-p[i];return 0;
}
static int eligible(const UCCharacterLookServices*h,const UCCharacterLook*s,int32_t id,const float*p,const float*offset,int*yes){
    float d[3];uint8_t found=0;*yes=0;if(difference(h,id,p,offset,d,&found))return 1;if(!found)return 0;
    double distance=sqrt(((double)d[2]*d[2]+(double)d[1]*d[1])+(double)d[0]*d[0]);
    if(!(distance<1000))return 0;if(distance==0)return 1;float inverse=1.f/(float)distance;
    for(unsigned i=0;i<3;i++)d[i]*=inverse;
    *yes=((s->forward[2]*d[2]+s->forward[1]*d[1])+s->forward[0]*d[0])>0;return 0;
}
static int choose(const UCCharacterLookServices*h,UCCharacterLook*s,UCGameRandom*r,int32_t self,const float*p,const float*offset,int*chosen){
    const char*names[3]={"VERY_INTERESTING_LOOK","INTERESTING_LOOK","UNINTERESTING_LOOK"};
    const float weights[3]={60,32,5};const int32_t*ids[3];uint32_t counts[3];*chosen=0;
    for(unsigned i=0;i<3;i++)if(uc_entity_names_view(h->names,names[i],ids+i,counts+i))return 1;
    float roll=(float)random_value(r,0,100),accum=0;
    for(unsigned i=0;i<3;i++){
        if(counts[i]&&weights[i]+accum>roll){int32_t start=0;if(uc_game_random_range(r,0,(int32_t)counts[i],&start))return 1;
            uint32_t index=(uint32_t)start;for(uint32_t j=0;j<counts[i];j++,index++){
                if(index>=counts[i])index=0;int32_t id=ids[i][index];if(id==self||id==s->target)continue;
                int yes=0;if(eligible(h,s,id,p,offset,&yes))return 1;if(yes){s->target=id;*chosen=1;return 0;}
            }
        }
        if(i)accum+=weights[i];
    }return 0;
}
static void rotate(const float*m,const float*v,float*out){for(unsigned i=0;i<3;i++)out[i]=(m[i]*v[0]+m[4+i]*v[1])+(m[8+i]*v[2]+0.f);}
int uc_character_look_step(UCCharacterLook*s,UCGameRandom*r,const UCCharacterLookServices*h,int32_t self,uint32_t frame,float dt,float*out){
    if(!s||!random_valid(r)||!h||!h->actor||!h->names||!out||!isfinite(dt)||dt<0||!isfinite(s->remaining)||s->nearby>1||s->direction_known>1)return 1;
    UCCharacterLookActor a;uint8_t found=0;if(actor(h,self,&a,&found))return 1;if(!found){memset(out,0,12);return 0;}
    const float*p=a.world+12;float offset[3]={0,0,0},forward[3]={0,0,1};if(a.has_head)offset[1]=a.head[1]-p[1];
    int32_t current;memcpy(&current,&frame,4);uint32_t next_bits=(uint32_t)s->last_frame+1;int32_t next;memcpy(&next,&next_bits,4);
    if(current<=next&&!s->direction_known)return 1;rotate(a.world,forward,s->forward);
    if(current>next){s->remaining=(float)random_value(r,1,5);s->last_frame=current;memcpy(s->direction,s->forward,12);s->target=-1;s->direction_known=1;}
    else{
        s->last_frame=current;s->remaining=(float)((double)s->remaining-dt);const int32_t*ids;uint32_t count=0;
        if(uc_entity_names_view(h->names,"EXTREMELY_INTERESTING_LOOK",&ids,&count))return 1;
        if(count){s->target=ids[0];s->remaining=(float)random_value(r,.2f,1);}
        else{
            UCCharacterLookActor player;uint8_t exists=0;if(actor(h,h->player,&player,&exists))return 1;double distance=FLT_MAX;
            if(exists){double x=(double)player.world[12]-p[0],y=(double)player.world[13]-p[1],z=(double)player.world[14]-p[2];distance=sqrt((z*z+y*y)+x*x);}
            uint8_t near=(uint8_t)(distance<400);
            if(!s->nearby&&near){s->target=h->player;s->remaining=(float)random_value(r,2.5f,5);}
            else if(s->remaining<=0){int chosen=0;if(choose(h,s,r,self,p,offset,&chosen))return 1;
                if(!chosen){float v[3];v[0]=(float)random_value(r,-.4f,.4f);double y=random_value(r,-.1f,.1f);v[1]=(float)y;
                    float inverse=(float)(1./sqrt((y*v[1]+(double)v[0]*v[0])+1.));v[0]*=inverse;v[1]*=inverse;v[2]=inverse;
                    rotate(a.world,v,s->direction);s->target=-1;
                }s->remaining=(float)random_value(r,1,5);
            }s->nearby=near;
        }
    }
    float d[3];if(difference(h,s->target,p,offset,d,&found))return 1;
    if(found){double inverse=1./sqrt(((double)d[0]*d[0]+(double)d[1]*d[1])+(double)d[2]*d[2]);if(!isfinite(inverse))return 1;
        for(unsigned i=0;i<3;i++)s->direction[i]=(float)(inverse*d[i]);}
    out[0]=(s->direction[0]*100.f+offset[0])+p[0];out[1]=p[1]+(s->direction[1]*100.f+offset[1]);out[2]=p[2]+(s->direction[2]*100.f+offset[2]);
    for(unsigned i=0;i<3;i++)if(!isfinite(out[i]))return 1;return 0;
}

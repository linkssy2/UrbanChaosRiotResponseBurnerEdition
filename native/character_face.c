#include "character_face.h"
#include "euler_rotation.h"
#include <math.h>
#include <string.h>
static float random_float(UCGameRandom*r,float first,float last){
    int32_t draw=0;uc_game_random_next(r,&draw);
    return(float)(((double)last-first)*(double)draw*4.6566128730773926e-10+first);
}
int uc_character_face_init(UCCharacterFace*s,UCGameRandom*r){
    if(!s||!r||r->first>=250||r->second>=250)return 1;
    UCCharacterFace n={0};const UCCharacterFaceNoise profiles[4]={
        {1,-1,.25f,.5f,1,1.5f,0,{0}}, {1,0,.05f,1,.3f,.8f,0,{0}},
        {1,-1,.1f,.7f,2,3,0,{0}}, {0,-1,.14f,.1f,.8f,1.5f,0,{0}}};
    memcpy(n.noise,profiles,sizeof profiles);
    for(unsigned i=0;i<4;i++)n.noise[i].remaining=random_float(r,n.noise[i].minimum*.5f,n.noise[i].maximum*.5f);
    *s=n;return 0;
}
static void noise(UCCharacterFaceNoise*n,UCGameRandom*r,float dt){
    n->remaining-=dt;if(n->remaining>0)return;
    if(n->value[0]==0&&n->value[1]==0&&n->value[2]==0){
        int32_t draw=0;uc_game_random_next(r,&draw);float chance=(float)draw*4.6566128730773926e-10f;
        if(n->chance>chance){
            memset(n->value,0,sizeof n->value);n->value[n->axis]=random_float(r,0.f-n->magnitude,n->magnitude);
            if(n->secondary!=-1)n->value[n->secondary]=random_float(r,0.f-n->magnitude,n->magnitude);
        }else memset(n->value,0,sizeof n->value);
    }else memset(n->value,0,sizeof n->value);
    n->remaining=random_float(r,n->minimum,n->maximum);
}
static float spring(float dt,float value,float target,float*rate){
    float desired=((1.f-1.f/(dt*10.f+1.f))*(target-value))/dt,change=desired-*rate;
    if(fabsf(desired)>fabsf(*rate)){float cap=dt*5.f;if(!(cap>change))change=cap;if(!(change>0.f-cap))change=0.f-cap;}
    *rate+=change;float result=*rate*dt+value;
    if(fabs((double)result-target)<.0010000000474974513){*rate=0;return target;}return result;
}
static float amplitude(const uint8_t*p,uint32_t count,float time){
    if(!p||time<=0)return 0;float x=time*20.f;int32_t first=(int32_t)x,last=(int32_t)ceil((double)x);
    float fraction=x-(float)first,remaining=1.f-fraction;
    float a=first>=0&&(uint32_t)first<count?(float)p[first]*.003921568859368563f:0;
    float b=last>=0&&(uint32_t)last<count?(float)p[last]*.003921568859368563f:0;
    return(float)((double)fraction*b+(double)a*remaining);
}
int uc_character_face_step(UCCharacterFace*s,UCGameRandom*r,const uint8_t*p,uint32_t count,
    float dt,float weight,UCCharacterFacePose output[8],uint32_t*emitted){
    if(!s||!r||!output||!emitted||r->first>=250||r->second>=250||count>1000000||(!p&&count)||
        !isfinite(dt)||dt<=0||dt>1||!isfinite(weight)||weight<0||weight>1||
        !isfinite(s->elapsed)||s->elapsed<0||s->elapsed>1e6f||!isfinite(s->phoneme_elapsed)||s->phoneme_elapsed<0||s->phoneme_elapsed>1e6f)return 1;
    for(unsigned i=0;i<4;i++){const UCCharacterFaceNoise*n=s->noise+i;
        if(n->axis<0||n->axis>2||n->secondary< -1||n->secondary>2||!isfinite(n->remaining)||
           !isfinite(n->magnitude)||!isfinite(n->chance)||!isfinite(n->minimum)||!isfinite(n->maximum))return 1;
        for(unsigned j=0;j<3;j++)if(!isfinite(n->value[j])||!isfinite(s->head[j])||!isfinite(s->rate[j]))return 1;
    }
    s->phoneme_elapsed+=dt;s->elapsed+=dt;UCCharacterFacePose result[8]={0};
    float a=amplitude(p,count,s->phoneme_elapsed),v=(a-.05f)*1.8181817531585693f;
    if(v>=1)v=1;else if(v<=0)v=0;float shape=(float)pow((double)v,.5),mouth=shape*.3f;
    //2AF81E overwrites the amplitude stack slot with the mapped mouth shape.
    result[0].bone=15;result[0].rotation[0]=0.f-(shape*.0890117958188057f-.019198622554540634f);
    static const int32_t bones[4]={18,19,20,21};
    for(unsigned i=0;i<4;i++){result[i+1].bone=bones[i];result[i+1].rotation[1]=i<2?mouth:0.f-mouth;}
    uint32_t n=5;
    if(s->elapsed>1&&weight>0){
        float eyes[3]={0},head[3]={0};
        for(unsigned i=0;i<4;i++){noise(s->noise+i,r,dt);for(unsigned j=0;j<3;j++)(i<2?eyes:head)[j]+=s->noise[i].value[j];}
        for(unsigned j=0;j<3;j++){s->head[j]=spring(dt,s->head[j],head[j],s->rate+j);head[j]=s->head[j]*weight;eyes[j]*=weight;}
        result[5].bone=13;result[6].bone=14;result[7].bone=8;
        memcpy(result[5].rotation,eyes,12);memcpy(result[6].rotation,eyes,12);memcpy(result[7].rotation,head,12);n=8;
    }
    memcpy(output,result,n*sizeof *result);*emitted=n;return 0;
}
int uc_character_face_apply(float(*local)[16],uint32_t bones,const UCCharacterFacePose*p,uint32_t count){
    if(!local||!p||count>24||!bones)return 1;
    for(uint32_t i=0;i<count;i++){if(p[i].bone< -1||(p[i].bone>=0&&(uint32_t)p[i].bone>=bones))return 1;
        for(unsigned j=0;j<3;j++)if(!isfinite(p[i].rotation[j])||!isfinite(p[i].translation[j]))return 1;}
    for(uint32_t i=0;i<count;i++)if(p[i].bone!=-1){
        float rotation[16],next[16]={0};float*old=local[p[i].bone];uc_euler_rotation(p[i].rotation[0],p[i].rotation[1],p[i].rotation[2],rotation);
        for(unsigned row=0;row<3;row++)for(unsigned col=0;col<4;col++)
            next[row*4+col]=(old[col]*rotation[row*4]+old[4+col]*rotation[row*4+1])+(old[8+col]*rotation[row*4+2]+old[12+col]*rotation[row*4+3]);
        for(unsigned j=0;j<3;j++)next[12+j]=old[12+j]+p[i].translation[j];next[15]=1;memcpy(old,next,sizeof next);
    }return 0;
}

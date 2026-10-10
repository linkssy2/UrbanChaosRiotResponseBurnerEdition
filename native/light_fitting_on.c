#include "light_fitting_on.h"
#include <math.h>
static int valid(const UCLightFittingOn*v){
    if(!v||!v->identity||!v->mode||!v->period||!v->clock||!v->delta||!v->position||
       !v->health||!v->spark_delay||!v->damage||!v->controller||!*v->controller||
       !v->bank||!v->random||v->random->first>=250||v->random->second>=250)return 0;
    if(*v->mode<2||*v->mode>4||!isfinite(*v->period)||*v->period<.001f||*v->period>100||
       !isfinite(*v->clock)||*v->clock<0||*v->clock>0x1p20f||!isfinite(*v->delta)||
       *v->delta<0||*v->delta>1||!isfinite(*v->health)||fabsf(*v->health)>1e6f||
       !isfinite(*v->spark_delay)||fabsf(*v->spark_delay)>4)return 0;
    for(unsigned i=0;i<3;i++)if(!isfinite(v->position[i])||fabsf(v->position[i])>1e6f)return 0;
    return 1;
}
static float power10(float q){float q2=q*q,q4=q2*q2;return(q4*q4)*q2;}
int uc_light_fitting_on(const UCLightFittingOn*v,const UCLightFittingOnServices*s){
    if(!valid(v)||!s||!s->publish||!s->factor||!s->state||!s->broken||!s->sparks||
       !s->player_position||!s->sound||!s->yield)return 1;
    if(s->publish(s->owner,*v->controller,1))return 2;
    for(;;){
        if(!valid(v))return 2;
        const uint32_t mode=*v->mode;
        const float phase=(float)((uintptr_t)v->identity&0xfff)+*v->clock,period=*v->period;
        float factor;
        if(mode==2){
            const double t=(double)phase/period;const float f=(float)t;
            double sum=sin(t*61.0)+sin((double)f*29.0);
            sum+=sin((double)f*23.0);sum+=sin((double)f*176.0);sum+=sin((double)f);
            factor=(float)((sum*(double).2f)*(double).25f+(double).6f);
            if(*v->spark_delay>0)*v->spark_delay=(float)((double)*v->spark_delay-*v->delta);
            if(*v->spark_delay<=0){
                if(s->sparks(s->owner))return 2;
                int32_t word;if(uc_game_random_next(v->random,&word))return 2;
                *v->spark_delay=(float)(((double)word*2.0)*0x1p-31+2.0);
            }
        }else if(mode==3){
            const float q=(float)(1.0-fmod((double)phase,period)/period);
            factor=power10(q)>.005f?1.f:0.f;
        }else{
            const float reciprocal=(float)(1.0/period);
            const float q=(float)(1.0-fmod((double)phase,period)*reciprocal);
            const double shifted=(double)period*(double).15f+(double)phase;
            const float other=(float)(1.0-fmod(shifted,period)*reciprocal);
            const float a=power10(q),b=power10(other);factor=b>a?b:a;
        }
        if(factor>.005f&&s->factor(s->owner,*v->controller)<=.005f){
            const float*p=s->player_position(s->owner);
            if(p){const float z=v->position[2]-p[2],y=v->position[1]-p[1],x=v->position[0]-p[0];
                if((z*z+y*y)+x*x<1e6f&&s->sound(s->owner,*v->bank))return 2;
            }
        }
        if(s->publish(s->owner,*v->controller,factor))return 2;
        if(*v->health<=0&&s->broken(s->owner,*v->controller,*v->damage))return 2;
        if(s->state(s->owner,*v->controller))return 0;
        if(s->yield(s->owner))return 2;
    }
}

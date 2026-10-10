#include "environment_history.h"
#include <math.h>
#include <xmmintrin.h>
static int arithmetic(void){return !(_mm_getcsr()&0xE040u);}
int uc_environment_history_reset(UCEnvironmentHistoryEffect *e){
    if(!e)return 0;e->amount=0;e->remaining=0;return 1;
}
int uc_environment_history_set(UCEnvironmentHistoryEffect *e,float amount,float duration){
    if(!e||!isfinite(amount)||!isfinite(duration))return 0;
    e->amount=amount;e->remaining=duration;return 1;
}
int uc_environment_history_tick(UCEnvironmentHistoryEffect *e,float dt){
    if(!e||!arithmetic()||!isfinite(e->amount)||!isfinite(e->remaining)||!isfinite(dt))return 0;
    UCEnvironmentHistoryEffect next=*e;
    if(next.remaining<=0)next.amount=0;
    else {next.remaining-=dt;if(!isfinite(next.remaining))return 0;}
    *e=next;return 1;
}
static int valid(const UCEnvironmentHistoryGroup *g){
    if(!g||!isfinite(g->minimum)||g->amount_count>1024||g->enabled_count>1024||
       (g->amount_count&&!g->amounts)||(g->enabled_count&&!g->enabled))return 0;
    for(uint32_t i=0;i<g->amount_count;i++)if(!g->amounts[i]||!isfinite(*g->amounts[i]))return 0;
    for(uint32_t i=0;i<g->enabled_count;i++)if(!g->enabled[i])return 0;
    return 1;
}
int uc_environment_history_select(const UCEnvironmentHistoryGroup *g,
    const UCEnvironmentHistoryGroup *parent,float *out){
    if(!out||!arithmetic()||!valid(g)||(parent&&!valid(parent)))return 0;
    const UCEnvironmentHistoryGroup *groups[2]={g,parent};float amount=0;
    for(unsigned k=0;k<2;k++)if(groups[k])
        for(uint32_t i=0;i<groups[k]->enabled_count;i++)if(!*groups[k]->enabled[i]){*out=0;return 1;}
    for(unsigned k=0;k<2;k++)if(groups[k])
        for(uint32_t i=0;i<groups[k]->amount_count;i++){
            amount+=*groups[k]->amounts[i];if(!isfinite(amount))return 0;
        }
    for(unsigned k=0;k<2;k++)if(groups[k]){
        if(!(0.8f>amount))amount=0.8f;
        if(!(amount>groups[k]->minimum))amount=groups[k]->minimum;
    }
    *out=amount>=1?1:amount>0?amount:0;return 1;
}

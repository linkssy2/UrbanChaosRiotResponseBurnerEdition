#include "mission_conditions.h"
#include <string.h>
static uint32_t word(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static int take(UCLevelBytes b,size_t *at,uint32_t n,uint32_t stride,const uint8_t **p){
    if(*at>b.size||n>(b.size-*at)/stride)return 0;
    *p=b.data+*at;*at+=(size_t)n*stride;return 1;
}
int uc_mission_conditions_load(const UCLevelEntity *e,UCMissionCondition *out,
    uint32_t capacity,uint32_t *count){
    if(!e||!count||!e->script_records.data)return 1;
    UCLevelBytes b=e->script_records;size_t at=0;const uint8_t*p;
    if(!take(b,&at,1,4,&p))return 1;
    uint32_t n=word(p);if(n>capacity||n>INT32_MAX||(n&&!out))return 1;
    for(uint32_t i=0;i<n;i++){
        UCMissionCondition c={0};
        if(!take(b,&at,1,4,&p))return 1;
        c.test_count=word(p);
        if(c.test_count>INT32_MAX||!take(b,&at,c.test_count,8,&c.tests)||!take(b,&at,1,4,&p))return 1;
        c.repeat=word(p);
        if(!take(b,&at,1,4,&p))return 1;
        c.scripts.count=word(p);
        if(!take(b,&at,c.scripts.count,12,&c.scripts.rows)||!take(b,&at,1,4,&p))return 1;
        memcpy(&c.scripts.priority,p,4);out[i]=c;
    }
    *count=n;return 0;
}
static int matches(const UCMissionCondition*c,const uint8_t*flags){
    if(!c->test_count)return 0;
    for(uint32_t j=0;j<c->test_count;j++)
        if((flags[word(c->tests+j*8)]!=0)!=(word(c->tests+j*8+4)!=0))return 0;
    return 1;
}
int uc_mission_conditions_update(UCMissionEvent*e,uint8_t*started,
    UCMissionCondition*c,uint32_t n,uint8_t*flags,size_t flag_count,
    void(*destroy)(void*,void*),void*owner){
    if(!uc_mission_event_valid(e)||!started)return 1;
    if(*e->secondary_flags&8)return 0;
    if(n>INT32_MAX||(n&&!c)||(*e->condition_index>=0&&(uint32_t)*e->condition_index>=n)||
       (*e->command&&!destroy))return 1;
    for(uint32_t i=0;i<n;i++){
        if(c[i].test_count>INT32_MAX||(c[i].test_count&&(!c[i].tests||!flags)))return 1;
        for(uint32_t j=0;j<c[i].test_count;j++)if(word(c[i].tests+j*8)>=flag_count)return 1;
    }
    if(*e->condition_index>=0&&!matches(c+*e->condition_index,flags)){
        *e->secondary_flags&=~4u;
        if(uc_mission_event_cancel(e,destroy,owner))return 1;
        *started=0; /* actual default217830 */
    }
    for(uint32_t i=0;i<n;i++){
        if((*e->priority>=0&&c[i].scripts.priority<*e->priority)||
           (!c[i].repeat&&c[i].used)||!matches(c+i,flags)||*e->condition_index==(int32_t)i)continue;
        int32_t priority=c[i].scripts.priority;*e->priority=priority;
        *e->secondary_flags&=0xffffffb7;
        *started=0;*e->flags&=0xbfffffff;
        if(*e->command)destroy(owner,*e->command);
        *e->command=NULL;*e->new_script=0;*started=1;
        *e->condition_index=(int32_t)i;*e->priority=priority;
        *e->secondary_flags|=4;c[i].used=1;
    }
    return 0;
}

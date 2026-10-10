#include "entity_properties.h"
#include <string.h>
static unsigned width(uint32_t t){return t==4?12:t==5?8:t==3?1:
    (t==1||t==2||t==6||t==8||t==9||t==10||t==11)?4:0;}
static void *field(UCEntityProperty*p,uint32_t t){return t==4?(void*)p->vector:
    t==5?(void*)p->pair:(t==6||t==8||t==9||t==10)?(void*)&p->reference:&p->scalar;}
static int name_valid(const char*p){if(!p)return 0;for(unsigned i=0;i<1024;i++)if(!p[i])return 1;return 0;}
static void initialize(UCEntityProperty*p,uint32_t t,const char*n,const void*v){
    p->reference=0;p->pair[0]=0;p->pair[1]=UINT32_MAX;p->type=t;p->name=n;
    unsigned size=width(t);unsigned char*d=field(p,t);const unsigned char*s=v;
    if(size==1)*d=*s;else for(unsigned i=0;i<size;i+=4){uint32_t word;memcpy(&word,s+i,4);memcpy(d+i,&word,4);}
}
int uc_entity_properties_begin(UCEntityProperties*p,int32_t source){
    if(!p)return 1;p->source_index=source;p->definition=NULL;p->count=0;
    for(unsigned i=0;i<15;i++)initialize(p->entries+i,0,NULL,NULL);
    p->flags=p->word2A0=p->placement_mode=p->word2A8=p->word2AC=0;return 0;
}
int uc_entity_properties_append(UCEntityProperties*p,UCEntityProperty*scratch,
    uint32_t t,const char*n,const void*v){
    if(!p||!scratch||p->count>=15||!name_valid(n)||(width(t)&&!v))return 1;
    initialize(scratch,t,n,v);p->entries[p->count]=*scratch;p->count++;return 0;
}
int uc_entity_properties_get(const UCEntityProperties*p,uint32_t t,const char*n,void*v,int*found){
    if(!p||p->count>15||!name_valid(n)||!found||(width(t)&&!v))return 1;
    for(unsigned i=0;i<p->count;i++)if(!name_valid(p->entries[i].name))return 1;
    *found=0;
    for(unsigned i=0;i<p->count;i++)if(!strcmp(p->entries[i].name,n)){
        unsigned size=width(t);if(size){memmove(v,field((UCEntityProperty*)(p->entries+i),t),size);*found=1;}break;
    }
    return 0;
}

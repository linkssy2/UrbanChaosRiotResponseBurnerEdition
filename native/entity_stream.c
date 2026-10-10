#include "entity_stream.h"
#include <string.h>
int uc_entity_stream_get(const UCLevelEntity *d,const uint8_t **cursor,
    uint32_t type,void *output,int *found){
    if(!d||!cursor||!found)return 1;
    if(type<1||type>11){*found=0;return 0;}
    if(type==7){*found=1;return 0;}
    int link=type==6||type>=8;
    if(!output){if(link)return 1;*found=1;return 0;}
    size_t size=type==4?12:type==5?8:4;
    uintptr_t first=(uintptr_t)d->properties.data,current=(uintptr_t)*cursor;
    if(!first||current<first||current-first>d->properties.size||
        size>d->properties.size-(size_t)(current-first))return 1;
    if(link){
        int32_t index;memcpy(&index,*cursor,4);
        if(index>=0&&(uint32_t)index<d->link_count&&(!d->links||!d->links[index]))return 1;
        *cursor+=4;
        *(const char**)output=index>=0&&(uint32_t)index<d->link_count?d->links[index]:"";
    }else{memcpy(output,*cursor,size);*cursor+=size;}
    *found=1;return 0;
}

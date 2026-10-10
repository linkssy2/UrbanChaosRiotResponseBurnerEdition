#include "mission_command.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

void uc_mission_command_destroy(void *unused,void *command){(void)unused;free(command);}
static uint32_t word(const uint8_t *p){uint32_t v;memcpy(&v,p,4);return v;}
static int span(const UCLevelEntity *e,const uint8_t *p,size_t n){
    uintptr_t at=(uintptr_t)p;
    for(uint32_t i=0;i<e->script_count;i++){
        UCLevelBytes b=e->scripts[i].code;uintptr_t start=(uintptr_t)b.data;
        if(b.data&&at>=start&&at-start<=b.size&&n<=b.size-(at-start))return 1;
    }
    return 0;
}
int uc_mission_command_decode(const UCMissionCommandContext *c,int32_t *out){
    if(!c||!out||!uc_mission_event_valid(c->event)||!c->wait_handle||*c->wait_handle||
       !c->level||!c->level->entities||c->entity>=c->level->entity_count)return 1;
    return uc_mission_command_decode_descriptor(c,c->level->entities+c->entity,c->level->field,out);
}
int uc_mission_command_decode_descriptor(const UCMissionCommandContext *c,
    const UCLevelEntity *v,uint32_t flag_limit,int32_t *out){
    if(!c||!out||!uc_mission_event_valid(c->event)||!c->wait_handle||*c->wait_handle||!v)return 1;
    UCMissionEvent *e=c->event;
    if(*e->command||(v->script_count&&!v->scripts))return 1;
    const uint8_t *p=*e->cursor;
    if(p&&!span(v,p,4))return 1;
    if(!p||word(p)==UINT32_MAX){*e->secondary_flags|=0x8000;*out=-1;return 0;}
    uint32_t op=word(p),arg;
    //216FA0/218BD0 ->55F00: named music cue. 55F00 explicitly succeeds
    //when the optional Sound_MusicManager (515010) is absent.
    if(op==57){
        if(!span(v,p,8))return 1;
        uint32_t length=word(p+4);
        if(length>=512||!span(v,p+8,length))return 1;
        char name[512];memcpy(name,p+8,length);name[length]=0;
        *e->cursor=p+8+length;
        if(c->music_event&&c->music_event(c->owner,name))return 2;
        *out=-2;return 0;
    }

    if((op!=5&&op!=6&&op!=10&&op!=11&&op!=16&&op!=17&&op!=21&&op!=22&&op!=23&&op!=43&&op!=52&&op!=53&&op!=61)||!span(v,p,8))return 1;
    arg=word(p+4);
    float remaining=0;
    if(op==6||op==21){memcpy(&remaining,&arg,4);if(!isfinite(remaining)||
        (remaining>0&&(!c->delta||!isfinite(*c->delta)||*c->delta<0||!c->yield)))return 1;}
    if((op==10||op==11)&&arg!=UINT32_MAX&&
       (!c->flags||arg>=flag_limit||arg>=c->flag_count))return 1;
    if(op==22&&arg!=UINT32_MAX&&!c->ensure)return 1;
    if(op==16&&arg!=UINT32_MAX&&!c->operate)return 1;
    if((op==17||op==23||op==43)&&arg!=UINT32_MAX&&!c->entity_action)return 1;
    if(op==61&&arg!=UINT32_MAX&&(!c->ensure_id||!c->alive||!c->yield))return 1;
    if((op==52||op==53)&&arg!=UINT32_MAX&&
       (!c->logic||!c->logic->triggers||arg>=c->logic->trigger_count))return 1;
    *e->cursor=p+4;
    if(op==22){*e->cursor+=4;if(arg!=UINT32_MAX&&c->ensure(c->owner,arg))return 2;*out=-2;return 0;}
    int32_t id=-1;
    if(op==61){*e->cursor+=4;if(arg==UINT32_MAX){*out=-2;return 0;}
        if(c->ensure_id(c->owner,arg,&id))return 2;}
    UCMissionCommand *command=(UCMissionCommand*)malloc(sizeof *command);
    if(!command)return 2;
    command->opcode=op;*e->command=command;
    if(op==61)command->argument=(uint32_t)id;
    else{command->argument=word(*e->cursor);*e->cursor+=4;}
    if(op==16){if(arg!=UINT32_MAX&&c->operate(c->owner,arg))return 2;}
    else if(op==17||op==23||op==43){if(arg!=UINT32_MAX&&c->entity_action(c->owner,arg,op==17?0:op==23?1:2))return 2;}
    else if(op==10||op==11){command->value=(uint8_t)(op==10);if(arg!=UINT32_MAX)c->flags[arg]=command->value;}
    else if(op==52||op==53){command->value=(uint8_t)(op==52);if(arg!=UINT32_MAX){
        uint8_t *flags=&c->logic->triggers[arg].flags;
        *flags=op==52?(uint8_t)(*flags|16):(uint8_t)(*flags&~16);
    }}
    else if(op==5){
        if((int32_t)arg>=0&&arg<v->script_count&&v->scripts[arg].code.data){
            *e->script_index=arg;*e->cursor=v->scripts[arg].code.data;
        }
        *e->secondary_flags|=0x1000;
    }else if(op==61){
        for(;;){uint8_t live=0;if(c->alive(c->owner,id,&live))return 2;
            if(!live)break;
            if(c->yield(c->owner))return 2;
        }
    }else while(remaining>0){
        if(!isfinite(*c->delta)||*c->delta<0)return 2;
        remaining=(float)((double)remaining-(double)*c->delta);
        if(c->yield(c->owner))return 2;
    }
    uc_mission_command_destroy(NULL,*e->command);*e->command=NULL;
    *out=op==5?-1:-2;return 0;
}

#include "checkpoint.h"
#include <math.h>
#include <string.h>
#include <limits.h>

int uc_checkpoint_pump(const UCCheckpointPump *p){
    if(!p||!p->ready||!p->reset||!p->number||!p->primary||!p->script||!p->current)return 1;
    for(;;){
        int32_t current;
        if(p->primary(p->owner)||p->script(p->owner))return 2;
        if(*p->reset||*p->ready)return 0;
        if(p->current(p->owner,&current))return 2;
        if(current!=*p->number)return 0;
    }
}
int uc_checkpoint_operate(const UCCheckpointOperation *p){
    if(!p||!p->pending||!p->parked||!p->save_pending||!p->number||!p->group||
       !p->player||!p->session||!p->dead||!p->objectives_block||!p->current||
       !p->select||!p->capture_flags||!p->capture_player||!p->sound||!p->message||!p->save||!p->event)return 1;
    if(*p->parked||!*p->pending)return 0;
    *p->pending=0;
    void *player;uint32_t session;uint8_t blocked=0;int32_t current;
    if(p->player(p->owner,&player)||p->session(p->owner,&session))return 2;
    if(!session){
        if(!player)goto event;
        if(p->dead(p->owner,player,&blocked))return 2;
        if(blocked)goto event;
    }
    if(p->objectives_block(p->owner,&blocked))return 2;
    if(!blocked){
        if(p->current(p->owner,&current))return 2;
        if(*p->number>current){
            if(p->select(p->owner,*p->number)||p->capture_flags(p->owner))return 2;
            *p->save_pending=1;
            if(p->capture_player(p->owner))return 2;
            if(*p->number>1&&(p->sound(p->owner)||p->message(p->owner)||p->save(p->owner)))return 2;
        }
    }
event:;
    int result=p->event(p->owner,p->entity,*p->group);
    if(result!=0&&result!=1)return 2;
    *p->parked=1;return 0;
}

static int snapshot_item(const UCCheckpointSnapshotInput *in,uint32_t i){
    const UCMainplayerState *s=in->inventory;const UCMainplayerItemSpec *d=in->specs+i;
    uint32_t left=d->clip_size-s->clip[i],reserve=s->ammo[d->ammo_category];
    uint32_t reload=d->category==0&&!d->reserve_factor?left:(reserve<left?reserve:left);
    return s->owned[i]&&(s->clip[i]||reload||in->item_extra[i]);
}
int uc_checkpoint_snapshot(const UCCheckpointSnapshotInput *in,UCCheckpointSnapshot *out){
    if(!in||!out||!in->inventory||!in->specs||!in->item_extra||!in->flag9||
       !in->number||!in->mission||!in->statistics)return 1;
    const UCMainplayerState *s=in->inventory;
    if(!isfinite(s->health)||s->health<0||s->active_item< -1||s->active_item>=27||
       s->requested_item< -1||s->requested_item>=27||s->previous_item< -1||s->previous_item>=27)return 1;
    for(unsigned i=0;i<9;i++)if(s->ammo[i]>INT_MAX)return 1;
    for(unsigned i=0;i<27;i++)if(!in->specs[i].available||in->specs[i].category>2||
       in->specs[i].ammo_category>=9||in->specs[i].clip_size>INT_MAX||
       s->clip[i]>in->specs[i].clip_size)return 1;
    unsigned maximum=0;
    for(unsigned c=0;c<3;c++){
        if(s->category_item[c]< -1||s->category_item[c]>=27||s->category_count[c]>27)return 1;
        for(unsigned j=0;j<s->category_count[c];j++){
            uint32_t i=s->category_order[c][j];if(i>=27)return 1;
            maximum+=(i!=25&&snapshot_item(in,i));
        }
    }
    if(maximum>27)return 1;
    out->number=*in->number;out->health=s->health;out->shield=(uint8_t)snapshot_item(in,0);
    out->flag9=*in->flag9;out->count=0;out->selected=-1;
    memcpy(out->mission,in->mission,sizeof out->mission);
    for(unsigned c=0;c<3;c++)for(unsigned j=0;j<s->category_count[c];j++){
        uint32_t i=s->category_order[c][j];if(i==25||!snapshot_item(in,i))continue;
        if(out->selected==-1)out->selected=(int32_t)i;
        UCCheckpointItem *row=out->items+out->count++;
        row->item=i;row->clip=(uint16_t)s->clip[i];
        row->reserve=(uint16_t)s->ammo[in->specs[i].ammo_category];row->seen=s->seen_items[i];
    }
    out->extra22=s->owned[22]?in->item_extra[22]:0;
    memcpy(out->category,s->category_item,sizeof out->category);
    int32_t selected=s->active_item==0||s->active_item==26?s->previous_item:s->requested_item;
    if(selected!=-1&&selected!=25)out->selected=selected;
    memcpy(out->statistics,in->statistics,sizeof out->statistics);return 0;
}

UCCheckpointStatus uc_checkpoint_begin(const UCLevel *level, int32_t number, UCCheckpoint *out) {
    UCLevelStatus result;
    if (!level || !out) return UC_CHECKPOINT_ARGUMENT;
    memset(out,0,sizeof(*out)); result=uc_level_restart(level,number,&out->restart);
    if (result!=UC_LEVEL_OK) return result==UC_LEVEL_NOT_FOUND?UC_CHECKPOINT_NOT_FOUND:UC_CHECKPOINT_ARGUMENT;
    out->remaining_delay=0.25f; out->initialized=1; return UC_CHECKPOINT_READY;
}
UCCheckpointStatus uc_checkpoint_resume(UCCheckpoint *c, float dt,
    UCCheckpointStartEvent start, UCCheckpointPollScript poll, void *context) {
    uint32_t group; int result;
    if (!c || !c->initialized || !isfinite(dt) || dt<0) return UC_CHECKPOINT_ARGUMENT;
    for (;;) switch(c->phase) {
    case UC_CHECKPOINT_OPERATED: case UC_CHECKPOINT_RESTART: case UC_CHECKPOINT_POST:
        group=c->phase==UC_CHECKPOINT_OPERATED?c->restart.on_operated[1]:
            c->phase==UC_CHECKPOINT_RESTART?c->restart.on_restart[1]:c->restart.on_post_restart[1];
        if (group!=UINT32_MAX) {
            if (!start) return UC_CHECKPOINT_NEEDS_EVENT;
            result=start(context,c->restart.entity_index,group);
            if (result==-1) return UC_CHECKPOINT_NEEDS_EVENT;
            if (result!=0 && result!=1) return UC_CHECKPOINT_FAILED;
        }
        if (c->phase==UC_CHECKPOINT_POST) { c->phase=UC_CHECKPOINT_PARK; return UC_CHECKPOINT_POST_STARTED; }
        c->phase=(UCCheckpointPhase)(c->phase+1); break;
    case UC_CHECKPOINT_WAIT_OPERATED: case UC_CHECKPOINT_WAIT_RESTART:
        if (!poll) return UC_CHECKPOINT_NEEDS_POLL;
        result=poll(context,c->restart.entity_index);
        if (result==-1) return UC_CHECKPOINT_NEEDS_POLL;
        if (result==1) return UC_CHECKPOINT_YIELD;
        if (result!=0) return UC_CHECKPOINT_FAILED;
        c->phase=(UCCheckpointPhase)(c->phase+1); break;
    case UC_CHECKPOINT_DELAY:
        if (c->remaining_delay>0) { c->remaining_delay-=dt; return UC_CHECKPOINT_YIELD; }
        c->restart_ready=1; c->reset_complete=1; c->phase=UC_CHECKPOINT_POST; break;
    case UC_CHECKPOINT_PARK: return UC_CHECKPOINT_PARKED;
    default:return UC_CHECKPOINT_ARGUMENT;
    }
}

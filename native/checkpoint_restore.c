#include "checkpoint.h"
#include <math.h>
#include <string.h>
#include <limits.h>

int uc_checkpoint_restore(const UCCheckpointSnapshot *in,const UCMainplayerItemSpec *specs,
    UCMainplayerState *out,uint32_t extra[27]){
    if(!in||!specs||!out||!extra||in->number<0||in->count>27||
       !isfinite(in->health)||in->health<0||in->health>65536||in->selected< -1||in->selected>=27)return 1;
    for(unsigned i=0;i<27;i++)if(!specs[i].available||specs[i].category>=3||
        specs[i].ammo_category>=9||specs[i].clip_size>INT_MAX||
        (uint64_t)specs[i].clip_size*specs[i].reserve_factor>INT_MAX)return 1;
    for(unsigned c=0;c<3;c++)if(in->category[c]< -1||in->category[c]>=27)return 1;
    for(unsigned n=0;n<in->count;n++)if(in->items[n].item>=27||
        in->items[n].clip>specs[in->items[n].item].clip_size)return 1;
    UCMainplayerState s={0};memcpy(s.seen_items,out->seen_items,sizeof s.seen_items);
    s.health=in->health>200.f?in->health:200.f;
    s.active_item=s.requested_item=s.previous_item=-1;
    for(unsigned c=0;c<3;c++)s.category_item[c]=-1;
    for(unsigned n=0;n<in->count;n++){
        const UCCheckpointItem *row=in->items+n;const UCMainplayerItemSpec *d=specs+row->item;
        uint32_t capacity=d->clip_size*d->reserve_factor;
        if(capacity>s.ammo_capacity[d->ammo_category])s.ammo_capacity[d->ammo_category]=capacity;
        s.owned[row->item]=1;s.clip[row->item]=row->clip;s.ammo[d->ammo_category]=row->reserve;
        if(uc_mainplayer_record_item(&s,row->item,specs,27))return 1;
        s.seen_items[row->item]=row->seen;
    }
    if(in->shield){
        const UCMainplayerItemSpec *d=specs;uint32_t capacity=d->clip_size*d->reserve_factor;
        if(capacity>s.ammo_capacity[d->ammo_category])s.ammo_capacity[d->ammo_category]=capacity;
        if(!s.owned[0]){s.clip[0]=d->clip_size;
            uint32_t available=s.ammo[d->ammo_category]+capacity;
            s.ammo[d->ammo_category]=available<s.ammo_capacity[d->ammo_category]?available:s.ammo_capacity[d->ammo_category];
            extra[0]=0;}
        s.owned[0]=1;
    }
    memcpy(s.category_item,in->category,sizeof s.category_item);
    s.requested_item=s.previous_item=in->selected;
    if(in->selected!=-1&&in->selected!=26)s.category_item[specs[in->selected].category]=in->selected;
    if(s.owned[22])extra[22]=in->extra22;
    *out=s;return 0;
}

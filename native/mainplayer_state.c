#include "mainplayer_state.h"
#include <limits.h>
#include <string.h>

static uint32_t equipment_tier(const UCMainplayerResolveInput *in, unsigned selector) {
    unsigned first, count, i;
    uint32_t tier;
    if (selector == 0) { first=4; count=4; tier=1; }
    else if (selector == 1) { first=3; count=1; tier=1; }
    else { first=selector == 3 ? 0 : 8; count=3; tier=0; }
    for (i=first; i<first+count; ++i) tier += in->upgrades[i]!=0;
    if (in->has_selected_record && in->selected_record_first==UINT32_MAX
        && in->progression_class<6
        && selector==((in->progression_class&1u) ? 3u : 4u)) ++tier;
    return tier;
}
UCMainplayerStatus uc_mainplayer_resolve_item(uint32_t requested,
    const UCMainplayerResolveInput *in, uint32_t *out) {
    uint32_t result=requested, tier;
    int32_t level;
    if (!in || !out) return UC_MAINPLAYER_ARGUMENT;
    if (!in->session_bypass) {
        if (!in->special_level) {
            level=in->parsed_level_number;
            switch(level) {
            case 12: level=0; break; case 22: level=1; break;
            case 21: level=2; break; case 23: level=3; break;
            case 24: level=6; break; case 26: level=11; break;
            case 25: level=12; break; default: break;
            }
            switch(requested) {
            case 1: if(level>=4) result=15; break;
            case 2: if(level>=8) result=16; break;
            case 13: if(level<3) result=UINT32_MAX; break;
            case 14: if(level<6) result=UINT32_MAX; break;
            case 17: result=level<1?UINT32_MAX:level<3?17:level<11?18:19; break;
            case 20: result=level<2?UINT32_MAX:level<6?20:level<12?21:22; break;
            default: break;
            }
        } else {
            switch(requested) {
            case 1: if(equipment_tier(in,1)==2) result=15; break;
            case 2:
                if(in->pistol_override) result=24;
                else if(in->pistol_extended) result=23;
                else {
                    tier=equipment_tier(in,0);
                    if(tier==2) result=16; else if(tier==3) result=7;
                    else if(tier==4) result=23; else if(tier==5) result=24;
                }
                break;
            case 13: if(!in->upgrades[47]) result=UINT32_MAX; break;
            case 14: if(!in->upgrades[46]) result=UINT32_MAX; break;
            case 17:
                tier=equipment_tier(in,4);
                result=tier>=1 && tier<=3 ? 16+tier : UINT32_MAX; break;
            case 20:
                if(in->rifle_override) result=22;
                else {
                    tier=equipment_tier(in,3);
                    result=tier>=1 && tier<=3 ? 19+tier : UINT32_MAX;
                }
                break;
            default: break;
            }
        }
    }
    *out=result; return UC_MAINPLAYER_OK;
}

UCMainplayerStatus uc_mainplayer_item_spec(uint32_t item, uint8_t mode,
    const uint8_t upgrades[36], UCMainplayerItemSpec *out) {
    /* Original4E55D0, strideF0, fields+8/+10/+24/+28 after3A35A0.
     * Keep this fixed data native; the original initializer runs only in tests. */
    static const uint8_t definitions[27][4]={
        {2,0,1,0},{2,0,1,0},{0,1,15,14},{0,2,30,5},{0,4,2,10},
        {0,6,60,4},{0,7,6,10},{0,1,30,7},{1,0,10,0},{2,0,1,0},
        {0,8,1,10},{1,0,10,0},{1,0,3,0},{1,0,3,0},{1,0,3,0},
        {2,0,1,0},{0,1,30,7},{0,5,6,4},{0,5,12,4},{0,5,20,4},
        {0,3,30,5},{0,3,45,5},{0,3,45,5},{0,1,60,4},{0,1,1,1},
        {0,1,1,1},{1,0,1,0}
    };
    static const uint8_t clips[]={3,5,8,10,12};
    UCMainplayerItemSpec s={0}; uint32_t i,count=0,start;
    if (!out || !upgrades || item>=27) return UC_MAINPLAYER_ARGUMENT;
    s.category=definitions[item][0]; s.ammo_category=definitions[item][1];
    s.clip_size=definitions[item][2]; s.reserve_factor=definitions[item][3]; s.available=1;
    if (!mode) {
        if(item>=12 && item<=14) {
            for(i=15;i<19;i++) count+=upgrades[i]!=0;
            s.clip_size=clips[count];
        }
        start=s.ammo_category==1?21:s.ammo_category==5?26:s.ammo_category==3?31:0;
        if(start) {
            for(count=0,i=start;i<start+5;i++) count+=upgrades[i]!=0;
            /* The original's sixth lookup entry is zero, not base+5. */
            s.reserve_factor=count==5?0:s.reserve_factor+count;
        }
    }
    *out=s; return UC_MAINPLAYER_OK;
}

UCMainplayerStatus uc_mainplayer_equipment_commit(UCMainplayerState *state,
    UCMainplayerEquipControl *control) {
    if(!state||!control||state->active_item < -1||state->active_item>=27||
        state->requested_item < -1||state->requested_item>=27||
        state->previous_item < -1||state->previous_item>=27)return UC_MAINPLAYER_ARGUMENT;
    if(control->select_empty){
        control->select_empty=0;state->previous_item=state->requested_item;state->active_item=0;
    }else if(control->select_previous){
        control->select_previous=0;state->active_item=state->previous_item;
    }else{
        state->previous_item=state->active_item;state->active_item=state->requested_item;
    }
    return UC_MAINPLAYER_OK;
}

static uint32_t word(const uint8_t *p) {
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
UCMainplayerStatus uc_mainplayer_properties(const UCLevel *level, uint32_t index,
    UCMainplayerProperties *out) {
    UCMainplayerProperties p={0}; const UCLevelEntity *e; const uint8_t *b;
    size_t at=0,n; uint32_t i,j,id;
    if (!level || !out || index>=level->entity_count || !level->entities) return UC_MAINPLAYER_ARGUMENT;
    e=&level->entities[index]; b=e->properties.data; n=e->properties.size;
    if (!e->class_name || strcmp(e->class_name,"Mainplayer")) return UC_MAINPLAYER_UNSUPPORTED;
    if (!b) return UC_MAINPLAYER_FORMAT;
    for(i=0;i<3;i++) {
        if (n-at<4) return UC_MAINPLAYER_FORMAT;
        id=word(b+at); at+=4;
        if (id>(n-at)/4) return UC_MAINPLAYER_FORMAT;
        p.flags[i].data=b+at; p.flags[i].size=id*4;
        for(j=0;j<id;j++) if(word(b+at+j*4)!=UINT32_MAX && word(b+at+j*4)>=level->field) return UC_MAINPLAYER_FORMAT;
        at+=(size_t)id*4;
    }
    if (n-at!=36) return UC_MAINPLAYER_FORMAT;
    for(i=0;i<7;i++) {
        id=word(b+at+(i?i+2:0)*4);
        if (id!=UINT32_MAX && (id>=e->link_count || !e->links || !e->links[id])) return UC_MAINPLAYER_FORMAT;
        if (!i) p.shield_animation=id==UINT32_MAX?NULL:e->links[id];
        else p.sound_banks[i-1]=id==UINT32_MAX?NULL:e->links[id];
    }
    p.start_with_weapons=(uint8_t)((word(b+at+4)&255)!=0);
    memcpy(&p.start_without_pistol_ammo,b+at+8,4);
    *out=p; return UC_MAINPLAYER_OK;
}
UCMainplayerStatus uc_mainplayer_record_item(UCMainplayerState *s,uint32_t id,
    const UCMainplayerItemSpec *items,uint32_t count) {
    uint32_t cat,ammo,j,k,at,n;int duplicate=0;
    if(!s||id>=27||!items||id>=count)return UC_MAINPLAYER_ARGUMENT;
    if(!items[id].available)return UC_MAINPLAYER_MISSING_DEFINITION;
    cat=items[id].category;ammo=items[id].ammo_category;
    if(cat>=3||ammo>=9)return UC_MAINPLAYER_ARGUMENT;
    if(!id||id==26){s->seen_items[id]=1;return UC_MAINPLAYER_OK;}
    n=s->category_count[cat];at=n;if(n>27)return UC_MAINPLAYER_ARGUMENT;
    for(j=0;j<n;j++){
        k=s->category_order[cat][j];
        if(k>=27||k>=count||!items[k].available||items[k].ammo_category>=9)return UC_MAINPLAYER_ARGUMENT;
        if(k==id)duplicate=1;
        if(items[k].ammo_category==ammo)at=j+1;
    }
    if(!duplicate&&n==27)return UC_MAINPLAYER_ARGUMENT;
    s->seen_items[id]=1;
    if(s->category_item[cat]==-1)s->category_item[cat]=(int32_t)id;
    if(s->requested_item==-1)s->active_item=s->requested_item=(int32_t)id;
    if(!duplicate){
        for(j=n;j>at;j--)s->category_order[cat][j]=s->category_order[cat][j-1];
        s->category_order[cat][at]=id;s->category_count[cat]++;
    }
    return UC_MAINPLAYER_OK;
}
UCMainplayerStatus uc_mainplayer_reset(const UCMainplayerProperties *p,
    const UCMainplayerResetInput *in, UCMainplayerState *out) {
    static const uint8_t priority[]={24,23,22,19,7,21,18,16,20,17,2};
    UCMainplayerState s={0}; uint32_t i,cat,ammo,capacity,remaining,tier=0;
    int32_t id; const UCMainplayerItemSpec *d;
    if (!p || !in || !out) return UC_MAINPLAYER_ARGUMENT;
    if (!in->no_saved_override || (p->start_with_weapons && p->start_without_pistol_ammo)) return UC_MAINPLAYER_UNSUPPORTED;
    s.active_item=s.requested_item=s.previous_item=-1;
    for(i=0;i<3;i++) { s.category_item[i]=-1; tier+=in->health_upgrades[i]!=0; }
    s.health=in->alternate_mode?500.0f:1000.0f+200.0f*(float)tier;
    if (!p->start_with_weapons) { *out=s; return UC_MAINPLAYER_OK; }
    for(i=0;i<7;i++) {
        id=i?in->resolved_items[i-1]:0;
        if (id==-1) continue;
        if (id<0 || id>=27) return UC_MAINPLAYER_ARGUMENT;
        if (!in->items || (uint32_t)id>=in->item_count || !in->items[id].available) return UC_MAINPLAYER_MISSING_DEFINITION;
        d=&in->items[id]; cat=d->category; ammo=d->ammo_category;
        if(cat>=3 || ammo>=9 || (uint64_t)d->clip_size*((uint64_t)d->reserve_factor+1)>INT32_MAX) return UC_MAINPLAYER_ARGUMENT;
        capacity=d->clip_size*d->reserve_factor;
        if(capacity>s.ammo_capacity[ammo]) s.ammo_capacity[ammo]=capacity;
        if(!s.owned[id]) {
            s.clip[id]=d->clip_size;
            remaining=s.ammo_capacity[ammo]-s.ammo[ammo];
            s.ammo[ammo]+=capacity<remaining?capacity:remaining;
        }
        s.owned[id]=1;
        { UCMainplayerStatus status=uc_mainplayer_record_item(&s,(uint32_t)id,in->items,in->item_count);
          if(status!=UC_MAINPLAYER_OK)return status; }
    }
    for(i=0;i<sizeof(priority);i++) if(s.owned[priority[i]]) {
        id=priority[i]; s.requested_item=s.previous_item=id;
        s.category_item[in->items[id].category]=id; break;
    }
    *out=s; return UC_MAINPLAYER_OK;
}

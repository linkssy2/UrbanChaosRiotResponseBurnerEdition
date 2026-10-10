#include "npc_thrown_weapon.h"
#include <string.h>
int uc_npc_thrown_construct(UCNpcThrownWeapon*s,UCBulletState*b,const UCEntityFreshView*v,const UCNpcThrownServices*h){
    UCNpcThrownProperty p;unsigned found;int32_t weapon;float damage;const char*name=NULL;
    if(!s||!b||!v||!v->id||!v->target.parent_id||!v->transform||!v->actor||!h||!h->base||!h->listener_reset
        ||!h->coroutine_initialize||!h->property||!h->bank||!h->physics_construct||!h->coroutine_schedule
        ||!h->damage||!h->listener_register||!h->listener_bind||!h->find_template||!h->name_add)return 1;
    if(h->base(h->owner))return 2;s->asset1D0=NULL;
    if(h->listener_reset(h->owner)||h->coroutine_initialize(h->owner))return 2;s->bank238=NULL;
    if(h->property(h->owner,1,"WeaponType",&p,&found))return 2;weapon=found?p.integer:-1;
    if(h->property(h->owner,2,"WeaponDamage",&p,&found))return 2;damage=found?p.number:50;
    if(h->property(h->owner,1,"DamageInfoTeam",&p,&found))return 2;b->team=found?(uint32_t)p.integer:0;
    p.string=NULL;if(h->property(h->owner,6,"SoundBank",&p,&found))return 2;s->bank238=p.string;
    if(h->bank(h->owner,s->bank238,&s->bank23C))return 2;
    if(h->property(h->owner,3,"Secondary Weapon",&p,&found))return 2;s->secondary218=found?(uint8_t)((p.integer&255)!=0):0;
    if(h->property(h->owner,1,"Owner",&p,&found))return 2;b->actor=found?(uint32_t)p.integer:UINT32_MAX;
    if(h->property(h->owner,1,"Texture variation",&p,&found))return 2;s->variation240=found?p.integer:0;
    if(b->actor==UINT32_MAX)b->actor=(uint32_t)*v->target.parent_id;
    s->weapon1EC=weapon;s->import254=s->import255=0;s->physics250=NULL;s->group280=s->group284=-1;
    if(h->physics_construct(h->owner)||h->coroutine_schedule(h->owner))return 2;
    s->delay1FC=.01f;memset(s->angular228,0,sizeof(s->angular228));memset(s->velocity21C,0,sizeof(s->velocity21C));
    s->armed1F0=s->launched1F8=s->exploded200=s->hit201=s->settled202=s->hidden203=s->flag204=0;
    s->word208=0;s->effect248=NULL;s->dropped210=0;s->material214=0;s->elapsed27C=0;
    if(h->damage(h->owner,damage))return 2;v->actor->word98=0x4400;b->item=(uint32_t)s->weapon1EC;b->definition_kind=10;
    if(h->listener_register(h->owner,*v->id)||h->listener_bind(h->owner,0)||h->listener_bind(h->owner,1)||h->listener_bind(h->owner,2))return 2;
    memset(s->vector264,0,sizeof(s->vector264));memset(s->vector258,0,sizeof(s->vector258));s->flag219=1;
    switch(s->weapon1EC){case 8:name="TEMPLATE_CLEAVER";break;case 11:name="TEMPLATE_MOLOTOV";break;
        case 12:name="TEMPLATE_PIPEBOMB";break;case 31:case 32:case 33:name="TEMPLATE_LAUNCHGRENADE";break;}
    s->template244=NULL;if(name&&h->find_template(h->owner,name,&s->template244))return 2;
    if(s->weapon1EC==8&&h->name_add(h->owner,"EXTREMELY_INTERESTING_LOOK",*v->id))return 2;
    memcpy(s->previous270,v->transform+12,sizeof(s->previous270));return 0;
}
float uc_npc_thrown_fuse(const UCNpcThrownWeapon*s){return s->armed1F0&&!s->exploded200?(s->fuse1F4<0?0:s->fuse1F4):-1;}
uint8_t uc_npc_thrown_settled(const UCNpcThrownWeapon*s){return s->settled202;}
uint8_t uc_npc_thrown_exploded(const UCNpcThrownWeapon*s){return s->exploded200;}
void uc_npc_thrown_set_fuse(UCNpcThrownWeapon*s,float value){if(value>=0)s->fuse1F4=value;}
int uc_npc_thrown_ignite(UCNpcThrownWeapon*s,float seconds,const UCNpcThrownServices*h){
    if(!s||!h)return 1;if(!s->launched1F8&&!s->armed1F0){s->armed1F0=1;s->fuse1F4=seconds;
        if(s->physics250&&(!h->physics_ignite||h->physics_ignite(h->owner,s->physics250,seconds)))return 2;}return 0;
}
int uc_npc_thrown_launch(UCNpcThrownWeapon*s,const float velocity[3],float delay,float spin,const UCNpcThrownServices*h){
    unsigned held;if(!s||!h||!velocity)return 1;if(!s->launched1F8&&!s->exploded200){
        memcpy(s->velocity21C,velocity,12);s->delay1FC=delay;s->launched1F8=1;s->spin20C=spin;
        if(s->physics250){if(!h->physics_source_owned||h->physics_source_owned(h->owner,s->physics250,&held))return 2;
            if(held&&(!h->physics_launch||h->physics_launch(h->owner,s->physics250,s->velocity21C,s->delay1FC)))return 2;}}
    return 0;
}
int uc_npc_thrown_drop(UCNpcThrownWeapon*s,const UCNpcThrownServices*h){
    static const float zero[3]={0,0,0};if(!s||!h)return 1;s->dropped210=1;
    if(s->armed1F0)return uc_npc_thrown_launch(s,zero,0,.1f,h);
    switch(s->weapon1EC){case 3:case 4:case 5:case 6:case 8:case 10:case 11:case 12:case 13:case 14:
        if(s->template244&&(!h->dropped_template||h->dropped_template(h->owner)))return 2;return 0;
        case 30:return 0;default:return !h->set_flags||h->set_flags(h->owner,1)?2:0;}
}

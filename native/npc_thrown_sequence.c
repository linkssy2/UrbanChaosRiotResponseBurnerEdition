#include "npc_thrown_sequence.h"
int uc_npc_thrown_model_start(UCNpcThrownWeapon*s,UCBulletState*b,const UCNpcThrownSequenceServices*h){
    if(!s||!b||!h||!h->visible)return 1;
    if(s->template244){const char*asset=0;unsigned found;
        if(!h->template_asset||h->template_asset(h->owner,s->template244,s->secondary218?"secondary model":"model",&asset,&found))return 2;
        if(found){s->asset1D0=asset;if(!h->model_load||h->model_load(h->owner,asset))return 2;}}
    if(h->visible(h->owner,1))return 2;b->motion.mass=.2f;return 0;
}
int uc_npc_thrown_sequence(UCNpcThrownWeapon*s,UCBulletState*b,const UCNpcThrownSequenceServices*h){
    unsigned active;double delta;int r=uc_npc_thrown_model_start(s,b,h);if(r)return r;
    if(!h->yield)return 2;
    while(!s->armed1F0&&!s->launched1F8)if(h->yield(h->owner))return 2;
    if(s->armed1F0&&(!h->ignition||h->ignition(h->owner)))return 2;
    while(!s->launched1F8&&!s->exploded200)if(h->yield(h->owner))return 2;
    if(s->launched1F8&&(!h->flight||h->flight(h->owner)))return 2;
    while(!s->exploded200&&!s->hidden203){if(!h->delta||h->delta(h->owner,&delta))return 2;
        double elapsed=delta+s->elapsed27C;s->elapsed27C=(float)elapsed;
        if(elapsed>8&&(!h->flags||h->flags(h->owner,1)))return 2;
        if(h->yield(h->owner))return 2;}
    if(s->exploded200&&(!h->explosion||h->explosion(h->owner)))return 2;
    if(s->dropped210&&!s->exploded200){if(!h->pickup||h->pickup(h->owner)||!h->land_sound||h->land_sound(h->owner)||h->visible(h->owner,0))return 2;return 0;}
    if(!h->sound_active)return 2;for(;;){if(h->sound_active(h->owner,&active))return 2;if(!active)break;if(h->yield(h->owner))return 2;}
    return !h->remove||h->remove(h->owner)?2:0;
}

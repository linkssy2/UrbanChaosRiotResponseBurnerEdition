#include "npc_gun.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>

static int fp(void){return (_mm_getcsr()&0xE040u)==0;}
static int matrix(const float *m){if(!m)return 0;for(unsigned i=0;i<16;++i)if(!isfinite(m[i])||fabsf(m[i])>1e6f)return 0;return 1;}
static int view(const UCEntityFreshView*v){return v&&v->target.identity&&v->target.flags&&v->target.parent_id&&v->flags60&&v->id&&v->model74&&v->transform;}
static void multiply(const float*a,const float*b,float*out){
    for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)
        out[r*4+c]=(a[r*4]*b[c]+a[r*4+1]*b[c+4])+(a[r*4+2]*b[c+8]+a[r*4+3]*b[c+12]);
}
static void swapped_compose(const float*bone,const float*world,float*out){
    static const float swap[16]={1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,1};
    float local[16];multiply(swap,bone,local);multiply(local,world,out);
}
int uc_npc_gun_construct(UCNpcGun*g,const UCEntityFreshView*v,const UCNpcGunServices*s){
    UCNpcGunProperty p;unsigned found;const UCEntityFreshView*parent=NULL;
    if(!g||!view(v)||!s||!s->health_base||!s->listener_reset||!s->property||!s->bank||!s->resolve
        ||!s->asset||!s->model_load||!s->flare||!s->tag||!s->listener_register||!s->listener_bind||!s->priority)return 1;
    if(s->health_base(s->owner))return 2;
    if(s->listener_reset(s->owner))return 2;
    g->assetC8=NULL;g->bankD0=NULL;
    if(s->property(s->owner,1,"WeaponType",&p,&found))return 2;g->weaponD8=found?p.integer:21;
    if(s->property(s->owner,2,"DamageAmount",&p,&found))return 2;g->damageDC=found?p.number:100;
    if(s->property(s->owner,1,"DamageTeam",&p,&found))return 2;g->teamE0=found?p.integer:5;
    if(s->property(s->owner,3,"Hand",&p,&found))return 2;g->handE4=found?(uint8_t)((p.integer&255)!=0):1;
    if(s->property(s->owner,1,"Owner",&p,&found))return 2;g->owner11C=found?p.integer:-1;
    p.string=NULL;if(s->property(s->owner,6,"SoundBank",&p,&found))return 2;g->bankD0=p.string;
    if(s->bank(s->owner,g->bankD0,&g->bankD4)||s->resolve(s->owner,*v->target.parent_id,&parent))return 2;
    if(parent){if(!view(parent))return 2;*v->target.flags|=*parent->target.flags&0x400000;
        if(*parent->flags60&0x10)*v->flags60|=0x10;
        if(*parent->flags60&2)*v->flags60|=2;}
    g->removeF0=g->droppedF4=g->wordF8=g->wordFC=0;
    g->old_y118=g->deadline110=g->unattached114=0;g->elapsedE8=1000;g->heatEC=0;
    if(s->asset(s->owner,&g->assetC8)||s->model_load(s->owner,g->assetC8)
        ||s->resolve(s->owner,*v->target.parent_id,&parent)||s->flare(s->owner,g->weaponD8,&g->flareCC))return 2;
    if(parent){if(!view(parent)||s->tag(s->owner,parent,"gundummy",&g->tags[0])
        ||s->tag(s->owner,parent,"gundummy2",&g->tags[1]))return 2;}
    else g->tags[0]=g->tags[1]=-1;
    if(s->tag(s->owner,v,"FirePos",&g->tags[2])||s->listener_register(s->owner,*v->id)
        ||s->listener_bind(s->owner,0)||s->listener_bind(s->owner,1)||s->priority(s->owner,*v->id,4))return 2;
    return 0;
}
int uc_npc_gun_update(UCNpcGun*g,const UCEntityFreshView*v,const UCNpcGunServices*s){
    float dt;
    if(!g||!view(v)||!s||!s->delta||!s->model_heat||!s->health_update||!fp()
        ||!isfinite(g->elapsedE8)||!isfinite(g->heatEC))return 1;
    if(s->delta(s->owner,&dt)||!isfinite(dt)||dt<0||dt>1)return 2;
    g->elapsedE8=(float)((double)dt+g->elapsedE8);
    if(s->delta(s->owner,&dt)||!isfinite(dt)||dt<0||dt>1)return 2;
    g->heatEC=(float)((double)g->heatEC-(double)dt*.1f);
    if(g->heatEC>=1)g->heatEC=1;else if(!(g->heatEC>.05f))g->heatEC=.05f;
    if(*v->model74&&s->model_heat(s->owner,*v->model74,g->heatEC))return 2;
    return s->health_update(s->owner)?2:0;
}
int uc_npc_gun_attached(UCNpcGun*g,const UCEntityFreshView*v,const UCNpcGunServices*s){
    const UCEntityFreshView *parent=NULL,*flare=NULL;float old_y,bone[16],world[16],now;unsigned found;
    if(!g||!view(v)||!s||!s->resolve||!s->pose_tag||!s->set_world||!s->time||!fp()
        ||g->removeF0||g->droppedF4||!matrix(v->transform))return 1;
    if(s->resolve(s->owner,*v->target.parent_id,&parent))return 2;
    old_y=v->transform[13];
    if(!parent){
        if(!s->player_count||!s->player_id||!s->remove||!isfinite(g->unattached114))return 2;
        if(s->player_count(s->owner)!=1)g->unattached114=6.f;
        else{const UCEntityFreshView*player=NULL;
            if(s->resolve(s->owner,s->player_id(s->owner),&player))return 2;
            if(player){if(!matrix(player->transform))return 2;
                float x=v->transform[12]-player->transform[12],y=v->transform[13]-player->transform[13],z=v->transform[14]-player->transform[14];
                if((z*z+y*y)+x*x<=4000000.f)g->unattached114=0;
                else{float dt;if(!s->delta||s->delta(s->owner,&dt)||!isfinite(dt))return 2;g->unattached114=(float)((double)dt+g->unattached114);}}
        }
        return s->remove(s->owner)?2:0;
    }
    if(!view(parent))return 2;
    g->unattached114=0;
    if(s->resolve(s->owner,*v->target.parent_id,&parent)||!view(parent)
        ||s->pose_tag(s->owner,parent,g->tags[g->handE4?0:1],bone,&found)||!found
        ||!matrix(bone)||!matrix(parent->transform))return 2;
    swapped_compose(bone,parent->transform,world);
    if(s->set_world(s->owner,world)||s->time(s->owner,&now)||!isfinite(now))return 2;
    g->deadline110=now;g->old_y118=old_y;
    if(s->resolve(s->owner,g->flareCC,&flare))return 2;
    if(!flare)return 0;
    if(!view(flare)||s->pose_tag(s->owner,v,g->tags[2],bone,&found)||!matrix(v->transform))return 2;
    if(found){if(!matrix(bone))return 2;swapped_compose(bone,v->transform,world);memcpy(flare->transform,world,sizeof(world));}
    else memcpy(flare->transform,v->transform,sizeof(world));
    return 0;
}
int uc_npc_gun_descriptor(const float*dummy,const float*world,int32_t owner,UCGameRandom*rng,UCNpcGunDescriptor*out){
    static const char*banks[]={"Banks\\NPC\\Cops\\Cop_WeaponsA.amp","Banks\\NPC\\Cops\\Cop_WeaponsB.amp","Banks\\NPC\\Cops\\Cop_WeaponsC.amp"};
    float swapped[16];int32_t choice;UCNpcGunDescriptor result;
    if(!out||!rng||!matrix(dummy)||!matrix(world)||!fp())return 1;
    memcpy(swapped,dummy,sizeof(swapped));memcpy(swapped,dummy+8,16);memcpy(swapped+8,dummy,16);
    multiply(swapped,world,result.world);result.weapon=2;result.damage=100;result.team=6;result.hand=1;result.owner=owner;
    if(uc_game_random_range(rng,0,3,&choice)||choice<0||choice>=3)return 2;
    result.bank=banks[choice];result.asset="Assets_Characters\\CHA_Cops\\CHA_Cop_Handgun.amf";
    *out=result;return 0;
}

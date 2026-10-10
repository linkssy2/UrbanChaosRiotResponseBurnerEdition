#include "npc_gun_shot.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int floats(const float*p,unsigned n){if(!p)return 0;for(unsigned i=0;i<n;++i)if(!isfinite(p[i])||fabsf(p[i])>1e6f)return 0;return 1;}
static int fp(void){return (_mm_getcsr()&0xE040u)==0;}
static void mul(const float*a,const float*b,float*out){for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)out[4*r+c]=(a[4*r]*b[c]+a[4*r+1]*b[4+c])+(a[4*r+2]*b[8+c]+a[4*r+3]*b[12+c]);}
int uc_npc_gun_shoot(UCNpcGun*g,const UCEntityFreshView*v,const float*d,unsigned secondary,unsigned effects,unsigned bullet,float offset,float scale,unsigned hit,const UCNpcGunServices*h,const UCNpcGunShotServices*s){
    float bone[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},world[16];unsigned found;
    if(!g||!v||!v->id||!floats(v->transform,16)||!floats(d,3)||!fp()||secondary>1||effects>1||bullet>1||hit>1
       ||!isfinite(offset)||!isfinite(scale)||!h||!h->pose_tag||!s||(bullet&&!s->bullet)
       ||(effects&&(!s->sound||!s->flare||!s->lightflash||!s->flash_position)))return 1;
    if(h->pose_tag(h->owner,v,g->tags[2],bone,&found)||!floats(bone,16)||!floats(v->transform,16))return 2;
    mul(bone,v->transform,world);
    if(bullet){UCWeaponBulletRequest r={0};r.item=(uint32_t)g->weaponD8;r.definition_kind=hit?3:4;r.team=(uint32_t)g->teamE0;r.actor=(uint32_t)g->owner11C;
        memcpy(r.direction,d,12);r.sentinel=-1;r.value20=g->damageDC;r.scale24=1;r.value28=scale;r.flag2c=1;
        for(unsigned i=0;i<3;++i)r.origin[i]=world[12+i]-world[4+i]*offset;
        if(s->bullet(s->owner,&r,*v->id))return 2;}
    if(effects){unsigned sound=0;if(secondary&&s->sound(s->owner,g->bankD4,"SecondaryFire",&sound))return 2;
        if(!sound&&s->sound(s->owner,g->bankD4,"Fire",&sound))return 2;
        if(g->flareCC!=-1&&s->flare(s->owner,g->flareCC,*v->id,g->weaponD8,(uint8_t)secondary))return 2;
        UCShotLightFlashRequest r={0};r.parameters=(UCLightFlashParameters){.18f,256,256,1,0,{2,1.5f,1},{.5f,.25f,0},.4f,0,0};
        if(!floats(s->flash_position,3))return 2;memcpy(r.position,s->flash_position,12);r.parent_id=(uint32_t)*v->id;r.mode=2;
        if(s->lightflash(s->owner,&r))return 2;}
    g->elapsedE8=0;g->heatEC+=.2f;return 0;
}
int uc_npc_gun_matrix(const UCNpcGun*g,const UCEntityFreshView*v,const UCNpcGunServices*h,float*out,unsigned*found){
    static const float swap[16]={1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,1};
    const UCEntityFreshView*p;float bone[16],local[16];unsigned posed;
    if(!g||!v||!v->target.parent_id||!h||!h->resolve||!h->pose_tag||!out||!found||!fp())return 1;
    if(h->resolve(h->owner,*v->target.parent_id,&p))return 2;
    if(!p){*found=0;return 0;}
    if(h->pose_tag(h->owner,p,g->tags[g->handE4?0:1],bone,&posed)||!posed||!floats(bone,16)||!floats(p->transform,16))return 2;
    mul(swap,bone,local);mul(local,p->transform,out);*found=1;return 0;
}
int uc_npc_gun_fire_position(const float*w,const float*p,unsigned found,float*out){
    if(!floats(w,16)||found>1||!out||!fp()||(found&&!floats(p,3)))return 1;
    for(unsigned i=0;i<3;++i)out[i]=found?(w[i]*p[0]+w[i+4]*p[1])+(w[i+8]*p[2]+w[i+12]):w[i+12];return 0;
}
int uc_npc_gun_reload_sound(const UCNpcGun*g,const UCNpcGunShotServices*s){unsigned found;if(!g||!s||!s->sound)return 1;return s->sound(s->owner,g->bankD4,"Load",&found)?2:0;}

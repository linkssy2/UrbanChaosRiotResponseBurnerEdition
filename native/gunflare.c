#include "gunflare.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
#include <xmmintrin.h>
static int valid(const UCGunFlare *s,const UCGunFlareModels *m,unsigned n){
    if(!s||n>16||(n&&!m)||s->first_person>1||s->pending>1||s->visible>1||s->sprite_enabled>1
       ||fegetround()!=FE_TONEAREST||(_mm_getcsr()&0xE040u))return 0;
    for(unsigned i=0;i<16;i++)if(!isfinite(s->world[i])||fabsf(s->world[i])>65536)return 0;
    for(unsigned i=0;i<n;i++){
        if(!m[i].name||m[i].count>256||(m[i].count&&!m[i].instances))return 0;
        unsigned k=0;while(k<1024&&m[i].name[k])k++;if(k==1024)return 0;
        for(unsigned j=0;j<m[i].count;j++)if(!m[i].instances[j])return 0;
    }
    return s->selected==UINT32_MAX||(s->selected<n&&s->variant<m[s->selected].count);
}
static int random_valid(const UCGameRandom *r){return r&&r->first<250&&r->second<250;}
static float sample(UCGameRandom *r,float low,float high){
    int32_t word;uc_game_random_next(r,&word);
    return (float)(((double)word*((double)high-low))*0x1p-31+low);
}
int uc_gunflare_shot(UCGunFlare *s,const UCGunFlareModels *m,unsigned n,unsigned item,
                    UCGameRandom *r,const UCGunFlareServices *h){
    static const char *model[]={"Assets_PlayerWeaponsAndEquipment\\EQP_AssaultRifle\\EQP_AssaultRifle_Muzzleflash_TP.mtxt",
        "Assets_PlayerWeaponsAndEquipment\\EQP_HandGun\\EQP_HandGun_muzzleflash_FP_01_Skin.AMF"};
    static const char *pistol[]={"GunFlare\\Pistol2.simpleparticlesystem","GunFlare\\Pistol3.simpleparticlesystem","GunFlare\\Pistol1.simpleparticlesystem"};
    static const char *shotgun[]={"GunFlare\\Shotgun3.simpleparticlesystem","GunFlare\\policeshotgunsmoke.simpleparticlesystem","GunFlare\\Pistol1.simpleparticlesystem"};
    if(!valid(s,m,n)||(item!=2&&item!=17)||!random_valid(r)||!h||!h->particles||!h->tint)return 1;
    unsigned selected=UINT32_MAX;
    for(unsigned i=0;i<n;i++)if(!strcmp(m[i].name,model[s->first_person])){selected=i;break;}
    if(selected!=UINT32_MAX&&!m[selected].count)return 1;
    s->counter=1;s->sprite_enabled=0;
    float matrix[16];memcpy(matrix,s->world+8,16);memcpy(matrix+4,s->world+4,16);
    memcpy(matrix+8,s->world,16);memcpy(matrix+12,s->world+12,16);
    for(unsigned i=0;i<3;i++)if(h->particles(h->user,(item==2?pistol:shotgun)[i],matrix,r))return 2;
    s->selected=selected;
    if(selected!=UINT32_MAX){
        s->variant=0;
        if(m[selected].count>1){int32_t word;uc_game_random_next(r,&word);
            int32_t index=word%(int32_t)m[selected].count;if(index<0)return 3;s->variant=(uint32_t)index;}
        float color[4]={1,1,1,sample(r,.9f,1)};
        if(h->tint(h->user,m[selected].instances[s->variant],color))return 2;
    }
    s->pending=1;return 0;
}
int uc_gunflare_tick(UCGunFlare *s,const UCGunFlareModels *m,unsigned n,const UCGunFlareServices *h){
    if(!valid(s,m,n)||!h||!h->visible||!h->parent_exists||!h->remove)return 1;
    int32_t counter;memcpy(&counter,&s->counter,4);
    if(counter>0&&s->selected==UINT32_MAX)return 1;
    if(s->parent!=-1){unsigned exists;
        if(h->parent_exists(h->user,s->parent,&exists))return 2;
        if(!exists)return h->remove(h->user)?2:0;
    }
    for(unsigned i=0;i<n;i++)for(unsigned j=0;j<m[i].count;j++)
        if(h->visible(h->user,m[i].instances[j],0))return 2;
    if(counter>0&&h->visible(h->user,m[s->selected].instances[s->variant],1))return 2;
    s->visible=s->pending;--s->counter;s->pending=0;return 0;
}
int uc_gunflare_transform(UCGunFlare *s,const UCGunFlareModels *m,unsigned n,UCGameRandom *r,const UCGunFlareServices *h){
    if(!valid(s,m,n)||!random_valid(r)||!h||!h->transform)return 1;
    if(s->selected!=UINT32_MAX){
        float angle=sample(r,0,0x1.921fb6p+1f),sn=(float)sin((double)angle),cs=(float)cos((double)angle);
        float rotation[16]={cs,-sn,0,0,sn,cs,-0.f,0,0,0,1,0,0,0,0,1},matrix[16];
        for(unsigned i=0;i<4;i++)for(unsigned j=0;j<4;j++)
            matrix[i*4+j]=(rotation[i*4]*s->world[j]+rotation[i*4+1]*s->world[4+j])
                         +(rotation[i*4+2]*s->world[8+j]+rotation[i*4+3]*s->world[12+j]);
        if(h->transform(h->user,m[s->selected].instances[s->variant],matrix))return 2;
    }
    memcpy(s->sprite_position,s->world+12,12);return 0;
}

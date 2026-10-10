#include "medic_weapon.h"
#include <math.h>
#include <string.h>
static int matrix(const float*m){if(!m)return 0;for(unsigned i=0;i<16;i++)if(!isfinite(m[i]))return 0;return 1;}
static int view(const UCEntityFreshView*v){return v&&v->id&&v->target.parent_id&&v->target.flags&&v->flags60&&v->transform&&v->model74;}
static void mul(const float*a,const float*b,float*out){for(unsigned r=0;r<4;r++)for(unsigned c=0;c<4;c++)out[r*4+c]=(a[r*4]*b[c]+a[r*4+1]*b[c+4])+(a[r*4+2]*b[c+8]+a[r*4+3]*b[c+12]);}
int uc_medic_weapon_matrix(const float*b,const float*w,float*out){if(!out||!matrix(b)||!matrix(w))return 1;float swap[16];memcpy(swap,b,64);memcpy(swap,b+8,16);memcpy(swap+8,b,16);mul(swap,w,out);return 0;}
#define CALL(n,...)do{if(!h->n||h->n(h->owner,__VA_ARGS__))return 2;}while(0)
#define CALL0(n)do{if(!h->n||h->n(h->owner))return 2;}while(0)
int uc_medic_weapon_construct(UCMedicWeapon*s,const UCEntityFreshView*v,const UCMedicWeaponServices*h){
 if(!s||!view(v)||!h)return 1;CALL0(health_base);CALL0(listener_reset);s->asset=0;CALL0(material_reset);s->bank_name=0;
 UCMedicWeaponProperty p;unsigned found;CALL(property,1,"WeaponType",&p,&found);s->weapon=found?p.integer:23;
 CALL(property,1,"Bone",&p,&found);s->bone=found?p.integer:0;
 CALL(property,3,"Collision",&p,&found);s->collision=found?(uint8_t)((p.integer&255)!=0):0;
 CALL(property,3,"HasModel",&p,&found);s->has_model=found?(uint8_t)((p.integer&255)!=0):1;
 p.string=0;CALL(property,6,"SoundBank",&p,&found);s->bank_name=p.string;CALL(bank,s->bank_name,&s->bank);
 CALL(property,1,"Texture variation",&p,&found);s->variation=found?p.integer:0;
 if((s->weapon!=34&&s->weapon!=8)||(s->bone!=0&&(s->weapon!=34||s->bone!=1))||s->collision||!s->has_model||s->variation!=(s->weapon==8?1:0))return 2;
 const UCEntityFreshView*parent=0;CALL(resolve,*v->target.parent_id,&parent);
 if(parent){if(!view(parent))return 2;*v->target.flags|=*parent->target.flags&0x400000;*v->flags60|=*parent->flags60&0x12;}
 s->blade=-1;CALL(asset,&s->asset);CALL(model_load,s->asset,&s->blade);if(s->blade!=-1)return 2;
 s->fading=s->dropped=s->grounded=s->landed=s->skin_applied=s->paused=0;
 s->time=s->old_y=s->far_time=s->sound_rate=0;s->sound_level=s->sound_target=1;
 CALL(resolve,*v->target.parent_id,&parent);static const char*tags[]={"gundummy","Bip01 L Hand","Bip01 L Forearm","gundummy"};
 for(unsigned i=0;i<4;i++){if(parent){CALL(tag,parent,tags[i],s->tags+i);}else s->tags[i]=-1;}
 CALL(listener_register,*v->id);for(unsigned i=0;i<3;i++){CALL(listener_bind,i);}CALL(priority,*v->id,4);return 0;
}
int uc_medic_weapon_update(UCMedicWeapon*s,const UCEntityFreshView*v,const UCMedicWeaponServices*h){
 if(!s||!view(v)||!h||(s->weapon!=34&&s->weapon!=8)||s->collision||!s->has_model||s->blade!=-1)return 1;
 if(!s->skin_applied){if(*v->model74){CALL(skin,s->variation);}s->skin_applied=1;}
 if(*v->model74){unsigned dirty=0;CALL(material_dirty,&dirty);if(dirty){CALL(skin,s->variation);}}
 CALL0(health_update);return 0;
}
int uc_medic_weapon_late(UCMedicWeapon*s,const UCEntityFreshView*v,const UCMedicWeaponServices*h){
 if(!s||!view(v)||!h||(s->weapon!=34&&s->weapon!=8)||s->dropped||s->fading||s->bone<0||s->bone>1||(s->weapon==8&&s->bone)||s->blade!=-1||!matrix(v->transform))return 1;
 const UCEntityFreshView*parent=0;CALL(resolve,*v->target.parent_id,&parent);if(s->paused)return 0;float old_y=v->transform[13];
 if(parent)s->far_time=0;else{int32_t count;CALL(player_count,&count);if(count!=1)s->far_time=6;
  else{int32_t id;CALL(player,&id);const UCEntityFreshView*player=0;CALL(resolve,id,&player);if(player){if(!matrix(player->transform))return 2;
   float x=v->transform[12]-player->transform[12],y=v->transform[13]-player->transform[13],z=v->transform[14]-player->transform[14];
   if((x*x+z*z)+y*y<=4000000.f)s->far_time=0;else{float dt;CALL(delta,&dt);s->far_time=(float)((double)dt+s->far_time);}}}}
 if(s->far_time>5||!parent){CALL0(remove);return 0;}
 CALL(resolve,*v->target.parent_id,&parent);if(parent){if(!view(parent)||!matrix(parent->transform))return 2;
  float bone[16],local[16],world[16];static const float swap[16]={1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,1};
  CALL(pose,parent,s->tags[s->bone],bone);if(!matrix(bone))return 2;mul(swap,bone,local);mul(local,parent->transform,world);CALL(set_world,world);}
 CALL(time,&s->time);s->old_y=old_y;return 0;
}
void uc_medic_weapon_pause(UCMedicWeapon*s,uint8_t value){if(s)s->paused=value!=0;}
int uc_medic_weapon_property(const UCMedicWeapon*s,const char*name,int32_t*out){if(!s||!name||!out)return -1;if(strcmp(name,"weapon type"))return 0;*out=s->weapon;return 1;}

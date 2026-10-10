#include "npc_gun_controller.h"
#include <math.h>
#include <string.h>
typedef struct Profile {int32_t type,capacity;float delay,min,max;unsigned automatic;} Profile;
static const Profile profiles[8]={{4,2,.33f,600,3500,0},{2,15,.33f,600,2500,0},
 {5,12,.115f,600,4000,1},{3,24,.1f,600,4000,1},{6,6,1.1f,600,2500,0},
 {10,1,.5f,600,5000,0},{1,100,.33f,600,2500,0},{7,30,.25f,600,2500,1}};
static int valid(const UCNpcGunController*s){return s&&s->profile>=0&&s->profile<8;}
static int capacity(const UCNpcGunController*s){return s->capacity_override?s->capacity_override:s->profile==-1?0:profiles[s->profile].capacity;}
static int random_float(const UCNpcGunControllerServices*h,float a,float b,double*out){int32_t n;
 if(!h||uc_game_random_next(h->random,&n))return 2;*out=((double)n*((double)b-a))*0x1p-31+a;return 0;}
static int cadence(UCNpcGunController*s,const UCNpcGunControllerServices*h){double v;
 if(!h||!h->difficulty)return 2;s->elapsed=0;s->enabled=1;s->held=0;
 if(random_float(h,.25f,.7f,&v))return 2;s->delay=(float)v;
 if(random_float(h,.25f,.5f,&v))return 2;s->interval=(float)v;
 if(h->difficulty(h->owner,&v)||!isfinite(v))return 2;s->interval=(float)((2.-v*1.25)*s->interval);return 0;}
static int resolve(const UCNpcGunControllerServices*h,int32_t id,const UCEntityFreshView**out){return !h||!h->resolve||h->resolve(h->owner,id,out)?2:0;}
int uc_npc_gun_controller_construct(UCNpcGunController*s,int32_t actor,void*body,int32_t cap,float range,const UCNpcGunControllerServices*h){
 if(!s||!h||!h->random||!h->difficulty||!isfinite(range))return 1;
 s->weapon=s->type=s->selected=s->last_target=-1;s->detached=s->hidden=0;
 s->actor=actor;s->body=body;s->capacity_override=cap;s->range_scale=range;s->cooldown=s->unused34=0;
 s->profile=s->target=s->obstruction=-1;s->burst_finished=0;s->idle=1000;s->offset[0]=s->offset[1]=0;s->offset[2]=1;
 s->shooting=s->reloading=0;s->scale=1;return cadence(s,h);}
int uc_npc_gun_controller_bind(UCNpcGunController*s,int32_t weapon,const UCNpcGunControllerServices*h){
 if(!s||!h||!h->resolve)return 1;const UCEntityFreshView*v;
 if(s->weapon!=-1){if(resolve(h,s->weapon,&v))return 2;if(v&&(!h->remove_weapon||h->remove_weapon(h->owner,s->weapon)))return 2;
  s->weapon=s->selected=s->type=-1;s->detached=0;}
 int32_t type=-1;if(weapon!=-1&&(!h->weapon_type||h->weapon_type(h->owner,weapon,&type)))return 2;
 s->profile=-1;for(int i=0;i<8;i++)if(profiles[i].type==type){s->profile=i;break;}
 if(resolve(h,s->actor,&v))return 2;if(v){s->type=type;s->selected=0;s->weapon=weapon;s->ammo=capacity(s);}return 0;}
int uc_npc_gun_controller_status(const UCNpcGunController*s,unsigned slot,int32_t*out){
 if(!s||!out||s->profile < -1||s->profile>7)return 1;switch(slot){
 case 0x18:*out=s->last_target;break;case 0x34:*out=s->shooting;break;case 0x38:*out=s->ammo<=0||s->ammo>capacity(s);break;
 case 0x3C:*out=s->reloading;break;case 0x40:*out=s->ammo!=capacity(s);break;case 0x48:*out=s->ammo;break;
 case 0x4C:*out=capacity(s);break;case 0x54:*out=s->burst_finished;break;case 0x58:if(!valid(s))return 1;*out=(int)profiles[s->profile].automatic;break;default:return 1;}return 0;}
int uc_npc_gun_controller_update(UCNpcGunController*s,const UCNpcGunControllerServices*h){
 if(!s||!h)return 1;if(s->weapon==-1)return 0;if(s->detached){const UCEntityFreshView*v;if(resolve(h,s->weapon,&v))return 2;if(!v)s->weapon=-1;return 0;}
 if(!valid(s)||!h->delta)return 2;double d=h->delta(h->owner);if(!isfinite(d))return 2;s->cooldown=(float)(s->cooldown-d);
 d=h->delta(h->owner);if(!isfinite(d))return 2;s->idle=(float)(d+s->idle);s->burst_finished=0;
 if(profiles[s->profile].automatic&&s->held){d=h->delta(h->owner);if(!isfinite(d))return 2;float t=(float)(d+s->elapsed);s->elapsed=t;
  if(t>s->delay){if(s->enabled){s->burst_finished=1;s->enabled=0;s->idle=10.f;}
   if(t>s->interval+s->delay)return cadence(s,h);}}return 0;}
int uc_npc_gun_controller_eligible(UCNpcGunController*s,int32_t target,uint32_t flags,float y,const UCNpcGunControllerServices*h,unsigned*out){
 if(!s||!h||!out||!isfinite(y))return 1;s->obstruction=-1;*out=0;
 if(s->weapon==-1||s->detached||(!(flags&0x80)&&s->cooldown>0))return 0;if(!valid(s))return 2;
 if(!(flags&1)&&profiles[s->profile].automatic&&!s->enabled)return 0;unsigned b=0;
 if(!(flags&0x40)){if(!h->boolean||h->boolean(h->owner,target,"avoid targetting",&b))return 2;if(b)return 0;}
 const UCEntityFreshView*t,*gun,*actor;if(resolve(h,target,&t)||!t||!t->transform)return 2;float point[3];b=0;
 if(!(flags&2)&&(!h->vector||h->vector(h->owner,t,"target position",point,&b)))return 2;if(!b)memcpy(point,t->transform+12,12);
 if(resolve(h,s->weapon,&gun)||!gun||!h->fire_position||!h->matrix)return 2;float from[3],matrix[16],dir[3],forward[3];
 if(h->fire_position(h->owner,s->weapon,from))return 2;from[1]+=y;for(unsigned i=0;i<3;i++)dir[i]=point[i]-from[i];
 float square=dir[0]*dir[0]+dir[1]*dir[1];square+=dir[2]*dir[2];if(!(square>0)||!isfinite(square))return 2;
 float inv=(float)(1./sqrt(square));for(unsigned i=0;i<3;i++)dir[i]*=inv;
 if(h->matrix(h->owner,s->weapon,matrix))return 2;double length=sqrt(((double)matrix[10]*matrix[10]+(double)matrix[9]*matrix[9])+(double)matrix[8]*matrix[8]);
 if(!(length>0)||!isfinite(length))return 2;for(unsigned i=0;i<3;i++)forward[i]=(float)((double)matrix[8+i]*(1./length));
 if(!(flags&4)){float dot=forward[2]*dir[2]+forward[1]*dir[1];dot+=forward[0]*dir[0];if(.98f>dot)return 0;}
 for(unsigned i=0;i<3;i++)from[i]-=forward[i]*30.f;
 if(resolve(h,s->actor,&actor)||!actor||!actor->actor||!h->query)return 2;const UCEntityFreshView*hit=NULL;b=0;
 if(h->query(h->owner,from,point,actor->actor->group94,&hit,&b))return 2;
 if(b){if(!hit)return 0;if(hit!=t){if(!hit->id||!hit->target.parent_id||!hit->target.flags||!h->boolean||h->boolean(h->owner,*hit->id,"is character",&b))return 2;
   if(b)s->obstruction=*hit->id;if(*hit->target.parent_id!=*t->id&&!(*hit->target.flags&2))return 0;}}
 if(!(flags&0x100)){if(!h->segment_clear||h->segment_clear(h->owner,from,point,&b))return 2;if(!b)return 0;}*out=1;return 0;}
static int shot(UCNpcGunController*s,const UCNpcGunControllerServices*h,unsigned*out){
 *out=0;if(s->hidden)return 0;const UCEntityFreshView*gun,*target;
 if(resolve(h,s->weapon,&gun)||!gun||!h->fire_position)return 2;float from[3];if(h->fire_position(h->owner,s->weapon,from))return 2;
 if(resolve(h,s->target,&target))return 2;if(!target)return 0;if(!h->vector||!target->transform)return 2;
 float point[3],d[3];unsigned found=0;if(h->vector(h->owner,target,s->head?"head position":"target position",point,&found))return 2;
 if(!found)memcpy(point,target->transform+12,12);for(unsigned i=0;i<3;i++)d[i]=(float)((double)point[i]-from[i]);
 const Profile*p=profiles+s->profile;float minimum=p->min*s->range_scale,maximum=p->max*s->range_scale,span=maximum-minimum;
 if(!(span>0)||!isfinite(span))return 2;float over=(float)(sqrt(((double)d[1]*d[1]+(double)d[2]*d[2])+(double)d[0]*d[0])-minimum);
 float fraction=over/span;if(over<0)over=0;float multiplier;if(fraction<0)multiplier=1;else if(fraction>1){multiplier=.01f;over=maximum-p->min*s->range_scale;}else multiplier=1-fraction*.99f;
 float threshold=s->accuracy*multiplier;if(s->force_miss){threshold=0;over=maximum-p->min*s->range_scale;}if(s->force_hit){threshold=1;over=0;}
 double random;if(random_float(h,0,1,&random))return 2;float off[3]={0};unsigned hit=random<threshold;
 if(hit){memset(s->offset,0,sizeof s->offset);}else{float x,z;
  /*6BA40 is a source RET: NearMissFromEnemy has no runtime side effect. */
  if(.3f>s->idle&&s->offset[0]!=0&&s->offset[1]!=0){x=s->offset[0];z=s->offset[1];if(random_float(h,3,6,&random))return 2;x=(float)(s->offset[2]>0?random+x:x-random);
   if((double)over*.025f>fabs((double)z)){if(random_float(h,3,6,&random))return 2;z=(float)(random+z);}}
  else{float end=over*.015f+20.f;if(random_float(h,20,end,&random))return 2;x=(float)random;int32_t coin;if(uc_game_random_boolean(h->random,&coin))return 2;if(coin)x=0.f-x;
   if(random_float(h,0,over*.025f,&random))return 2;z=(float)random;int32_t n;if(uc_game_random_next(h->random,&n))return 2;
   if((float)n*0x1p-31f<.25f){if(random_float(h,200,300,&random))return 2;z=(float)random;}z=0.f-z;}
  float perpendicular[3]={d[2],0,0.f-d[0]};double sum=((double)perpendicular[2]*perpendicular[2]+(double)perpendicular[1]*perpendicular[1])+(double)perpendicular[0]*perpendicular[0];
  if(!(sum>0)||!isfinite(sum))return 2;float inv=(float)(1./sqrt(sum));s->offset[0]=x;s->offset[1]=z;s->offset[2]=x;
  off[0]=(inv*perpendicular[0])*x;off[1]=z;off[2]=(inv*perpendicular[2])*x;}
 float direction[3];for(unsigned i=0;i<3;i++)direction[i]=d[i]+off[i];double square=((double)direction[2]*direction[2]+(double)direction[1]*direction[1])+(double)direction[0]*direction[0];
 if(!(square>0)||!isfinite(square))return 2;float inv=(float)(1./sqrt(square));for(unsigned i=0;i<3;i++)direction[i]*=inv;
 if(resolve(h,s->target,&target))return 2;if(target&&target->target.flags&&(*target->target.flags&0x08000000)){if(!target->actor)return 2;target->actor->word90&=~2u;}
 if(!h->shot||h->shot(h->owner,s->weapon,direction,s->scale,hit))return 2;
 if(resolve(h,s->target,&target))return 2;if(target&&target->target.flags&&(*target->target.flags&0x08000000)){if(!target->actor)return 2;*out=!!(target->actor->word90&2);}
 if(profiles[s->profile].automatic)s->held=1;if(s->target!=-1)s->last_target=s->target;s->idle=0;return 0;}
int uc_npc_gun_controller_fire(UCNpcGunController*s,int32_t target,uint32_t flags,float accuracy,float scale,const UCNpcGunControllerServices*h,unsigned*out){
 if(!s||!h||!out||!isfinite(accuracy)||!isfinite(scale))return 1;*out=0;if(s->hidden||(!(flags&0x80)&&s->cooldown>0))return 0;if(!valid(s))return 2;
 s->target=target;s->accuracy=accuracy;s->scale=scale;s->force_hit=(uint8_t)!!(flags&8);s->force_miss=0;s->position_mode=(uint8_t)((flags>>1)&1);s->head=(uint8_t)((flags>>5)&1);
 if(flags&16)s->force_miss=1;else if(!(flags&1)&&profiles[s->profile].automatic){double r;if(s->delay==0||random_float(h,0,1,&r))return 2;s->force_miss=(uint8_t)(((double)s->elapsed/s->delay)*.75>r);}
 int r=shot(s,h,out);if(r)return r;s->ammo=(int32_t)((uint32_t)s->ammo-1);s->cooldown=profiles[s->profile].delay;return 0;}
int uc_npc_gun_controller_reload(UCNpcGunController*s,unsigned speak,unsigned alternate,const UCNpcGunControllerServices*h){
 if(!valid(s)||!h)return 1;const UCEntityFreshView*v;if(resolve(h,s->actor,&v))return 2;
 if(speak&&alternate){int32_t bit;if(uc_game_random_boolean(h->random,&bit)||!v||!h->emote||h->emote(h->owner,s->actor,bit?68u:22u,25))return 2;}
 if(resolve(h,s->weapon,&v))return 2;if(v&&(!h->reload_sound||h->reload_sound(h->owner,s->weapon)))return 2;s->ammo=capacity(s);
 return profiles[s->profile].automatic?cadence(s,h):0;}

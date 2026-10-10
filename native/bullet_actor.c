#include "bullet_actor.h"
#include <fenv.h>
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int finite3(const float *p){return p&&isfinite(p[0])&&isfinite(p[1])&&isfinite(p[2]);}
static double length3(const float *p){return sqrt(((double)p[2]*p[2]+(double)p[1]*p[1])+(double)p[0]*p[0]);}
static double distance(const float *a,const float *b){double x=(double)a[0]-b[0],y=(double)a[1]-b[1],z=(double)a[2]-b[2];return sqrt((z*z+y*y)+x*x);}
int uc_bullet_actor_ignored(const UCBulletState *b,int32_t target,int32_t player){return b&&((uint32_t)target==b->actor||(b->definition_kind==4&&target==player));}
int uc_bullet_actor_contact(const UCEntityFreshView *v,UCBulletState *b,
    const UCEntityFreshView *t,const UCBulletImpact *hit,const UCBulletActorServices *s){
    if(!v||!v->actor||!v->id||!v->transform||!b||!t||!t->actor||!t->target.flags||!hit||!s||!s->boolean||!s->damage||!s->react||
       !finite3(b->motion.velocity)||!finite3(b->vectorE4)||!finite3(hit->point)||!isfinite(v->actor->valueA4)||
       !isfinite(b->valueDC)||!isfinite(b->motion.mass)||b->definition_kind==8||b->definition_kind==9||
       (_mm_getcsr()&0xE040u)||fegetround()!=FE_TONEAREST)return 1;
    if(!(v->actor->word90&0x10))return 0;
    uint8_t invulnerable=0,shield=0;
    const int eligible=b->item!=UINT32_MAX&&v->actor->valueA4>0;
    if(s->boolean(s->owner,"invulnerable",&invulnerable)||invulnerable>1)return 2;
    if(!eligible||invulnerable)return 0;
    if(s->boolean(s->owner,"shield blocks damage",&shield)||shield)return 2;
    const double speed=length3(b->motion.velocity);if(!(speed>0)||!isfinite(speed))return 2;
    *t->target.flags|=0x80;t->actor->word90|=2;v->actor->word90|=8;
    UCPlayerFireDamage packet={0};packet.source=(int32_t)b->item;packet.kind=(int32_t)b->definition_kind;packet.zero=(int32_t)b->team;
    const float inverse=(float)(1/speed);for(unsigned i=0;i<3;i++)packet.direction[i]=b->motion.velocity[i]*inverse;
    packet.owner=(int32_t)b->actor;memcpy(packet.center,hit->point,12);packet.other=*v->id;packet.session=packet.unique=-1;
    float amount=v->actor->valueA4;
    if(b->valueDC>0){
        const double fraction=1-distance(hit->point,b->vectorE4)/b->valueDC;float clamped=(float)fraction;
        if((double).05f>fraction)clamped=.05f;else if(clamped>1)clamped=1;
        amount=(float)((double)v->actor->valueA4*clamped);
    }
    if(s->damage(s->owner,amount,&packet))return 2;
    UCCharacterBullet bullet={(int32_t)b->item,(int32_t)b->definition_kind,(int32_t)b->actor,{0}};
    memcpy(bullet.position,v->transform+12,12);
    UCCharacterBulletImpact impact={0};impact.source=*v->id;impact.material=hit->material;impact.kind=2;
    memcpy(impact.point,hit->point,12);for(unsigned i=0;i<3;i++)impact.momentum[i]=b->motion.velocity[i]*b->motion.mass;
    impact.distance=(float)distance(hit->point,b->vectorE4);memcpy(impact.parameters,b->parameters,sizeof impact.parameters);
    return s->react(s->owner,&bullet,&impact)?2:0;
}

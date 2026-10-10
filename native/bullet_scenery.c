#include "bullet_scenery.h"
#include <fenv.h>
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int bounded3(const float *v) {
    return v&&isfinite(v[0])&&fabsf(v[0])<=1e6f&&isfinite(v[1])&&fabsf(v[1])<=1e6f
        &&isfinite(v[2])&&fabsf(v[2])<=1e6f;
}
static double length3(const float *v) {
    return sqrt(((double)v[2]*v[2]+(double)v[1]*v[1])+(double)v[0]*v[0]);
}
static int different(const float *a,const float *b) {
    return a[0]!=b[0]||a[1]!=b[1]||a[2]!=b[2];
}
int uc_bullet_scenery(const UCBulletScenery *o,const UCEntityFreshView *v,
                     UCBulletState *b,float out[3]) {
    float full[3],desired[3],correction[3]={0};const UCCollisionResult *hit=NULL;
    if(!o||!v||!b||!out||!v->transform||!v->target.flags||!v->id||!v->actor
       ||!o->delta||!o->history||o->history->count>200||!o->preflight||!o->impact
       ||!o->segment||(!o->query&&(!o->trace.queries||!o->trace.hits))
       ||!bounded3(v->transform+12)||!bounded3(b->vectorE4)||!bounded3(b->motion.velocity)
       ||!isfinite(b->motion.mass)||b->motion.mass<=0||b->motion.mass>1e4f
       ||!isfinite(b->impact)||!isfinite(*o->delta)||fabsf(*o->delta)>1000
       ||(_mm_getcsr()&0xE040u)||fegetround()!=FE_TONEAREST
       ||!o->preflight(o->context,v,b))return 1;
    float *position=v->transform+12;
    if(b->vectorE4[0]==0&&b->vectorE4[1]==0&&b->vectorE4[2]==0)
        memcpy(b->vectorE4,position,12);
    if(!(*v->target.flags&0x800)){memcpy(out,correction,12);return 0;}
    for(unsigned i=0;i<3;++i)full[i]=desired[i]=position[i]+b->motion.velocity[i];
    int result=o->query?o->query(o->context,position,full,9,v->actor->group94,v->actor->word98,b->impact,&hit):
        uc_collision_scenery_trace(&o->trace,position,full,9,v->actor->group94,v->actor->word98,b->impact,&hit);
    if(result<0)return 2;
    if(result) {
        UCBulletImpact impact;double d[3];
        const float defaults[9]={1200,900,500,350,.6f,.4f,1,1.5f,20};
        if(result!=1||!hit||hit->property45)return 2;
        impact.material=hit->material;memcpy(impact.parameters,defaults,sizeof defaults);
        memcpy(impact.point,hit->point,12);memcpy(impact.normal,hit->normal,12);
        for(unsigned i=0;i<3;++i) {
            impact.momentum[i]=b->motion.velocity[i]*b->motion.mass;
            d[i]=(double)hit->point[i]-b->vectorE4[i];
        }
        impact.distance=(float)(sqrt((d[2]*d[2]+d[1]*d[1])+d[0]*d[0])*(double).01f);
        if(o->contact&&o->contact(o->context,v,b,&impact))return 2;
        *v->target.flags|=0x80;
        for(unsigned i=0;i<3;++i){desired[i]=impact.normal[i]+impact.point[i];correction[i]=desired[i]-full[i];}
        if(o->impact(o->context,v,b,&impact))return 2;
    }
    if(different(desired,position)&&different(full,position)) {
        UCBulletSegment segment;float delta_desired[3],delta_full[3];
        memcpy(segment.from,position,12);memcpy(segment.to,desired,12);
        for(unsigned i=0;i<3;++i){delta_desired[i]=desired[i]-position[i];delta_full[i]=full[i]-position[i];}
        segment.delta=(float)(length3(delta_desired)/length3(delta_full)*(double)*o->delta);
        segment.mass=b->motion.mass;
        if(o->segment(o->context,&segment))return 2;
        if(b->item==11||b->item==12||b->item==33){
            UCBulletFlyingSegment flying;memcpy(flying.from,position,12);memcpy(flying.to,desired,12);flying.id=*v->id;
            if(!o->flying_segment||o->flying_segment(o->context,&flying))return 2;
        }
    }
    if(b->enabled1C8) {
        UCBulletHistoryEntry entry;entry.owner_id=b->actor;entry.bullet_id=(uint32_t)*v->id;
        memcpy(entry.from,position,12);memcpy(entry.to,desired,12);
        if(b->definition_kind!=6&&uc_bullet_history_append(o->history,&entry,o->local_player_id))return 2;
        b->enabled1C8=0;
    }
    memcpy(out,correction,12);return 0;
}

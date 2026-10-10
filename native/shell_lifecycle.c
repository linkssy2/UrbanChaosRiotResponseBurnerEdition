#include "shell_lifecycle.h"
#include <fenv.h>
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int values(const float *v,unsigned n,float bound){
    for(unsigned i=0;i<n;i++)if(!isfinite(v[i])||fabsf(v[i])>bound)return 0;
    return 1;
}
static int services(const UCShellServices *h,int update){
    return h&&h->preflight&&h->construct_base&&h->parent_position&&
        h->group&&(!update||(h->bone_position&&h->attach_model&&h->configure_model&&h->bounce_sound&&h->delta&&h->remove))&&
        fegetround()==FE_TONEAREST&&!(_mm_getcsr()&0xE040u);
}
int uc_shell_construct(UCShellState *s,const UCShellProperties *p,UCGameRandom *r,const UCShellServices *h){
    if(!s||!p||!r||!services(h,0)||r->first>=250||r->second>=250||p->present>15||
       !(p->present&1)||(p->weapon_type!=2&&p->weapon_type!=17)||
       !values(p->velocity,3,1e6f)||!values(&p->ground_y,1,1e6f)||
       !h->preflight(h->context,s,p))return 1;
    if(h->construct_base(h->context,s))return 2;
    s->weapon_type=p->weapon_type;
    for(unsigned j=0;j<3;j++)s->velocity[j]=(p->present&2)?p->velocity[j]:0.f;
    s->start_bone=(p->present&4)?p->start_bone:-1;
    s->ground_y=(p->present&8)?p->ground_y:0.f;
    s->family=p->weapon_type==2?0u:2u;
    int32_t word;uc_game_random_next(r,&word);
    s->bounce=(uint8_t)((float)word*0x1p-31f>.5f);s->age=0.f;
    uc_game_random_next(r,&word);
    float angle=(float)((((double)word*60.)*0x1p-31-30.)*(double)0x1.1df46ap-6f);
    /* Full2D3DC0(0,angle,0) specialization, retaining signed-zero operations. */
    float sn=(float)sin((double)angle),cs=(float)cos((double)angle);
    float rotation[12]={cs,-0.f,sn,0.f,
        (1.f*sn)*0.f+1.f*0.f,1.f*1.f-(0.f*sn)*0.f,-(cs*0.f),0.f,
        0.f*0.f-(1.f*1.f)*sn,(1.f*0.f)*sn+1.f*0.f,cs*1.f,0.f};
    float base[16];memcpy(base,s->transform,sizeof base);
    base[3]=base[7]=base[11]=0.f;base[12]=base[13]=base[14]=0.f;base[15]=1.f;
    for(unsigned col=0;col<3;col++)for(unsigned row=0;row<4;row++){
        const float *v=rotation+col*4;
        s->transform[col*4+row]=(base[row]*v[0]+base[4+row]*v[1])+
            (base[8+row]*v[2]+base[12+row]*v[3]);
    }
    const float *parent=h->parent_position(h->context,s->parent_id);
    if(parent)memcpy(s->parent_position,parent,sizeof s->parent_position);
    h->group(h->context,s,1);return 0;
}
static int remove_shell(UCShellState *s,const UCShellServices *h){
    h->group(h->context,s,0);h->remove(h->context,s);return 2;
}
int uc_shell_update(UCShellState *s,const UCShellServices *h){
    if(!s||!services(h,1)||(s->weapon_type!=2&&s->weapon_type!=17)||
       s->family!=(s->weapon_type==2?0u:2u)||s->bounce>1||
       !values(s->transform,16,1e9f)||!values(s->velocity,3,1e9f)||
       !values(&s->ground_y,1,1e9f)||!values(s->parent_position,3,1e9f)||
       !values(&s->age,1,1e9f)||!h->preflight(h->context,s,0))return 1;
    if(s->age==0.f){
        h->attach_model(h->context,s,s->family==0?
            "Assets_PlayerWeaponsAndEquipment\\EQP_HandGun\\eqp_handgun_shell_skin.amf":
            "Assets_PlayerWeaponsAndEquipment\\EQP_Police_shotgun\\eqp_shotgun_shell_skin.amf");
        if(s->model)h->configure_model(h->context,s);
        if(s->start_bone!=-1){
            /* The source resolves this parent even though the tag service
             * itself receives its identity in this typed interface. */
            (void)h->parent_position(h->context,s->parent_id);
            float position[3];h->bone_position(h->context,s->parent_id,s->start_bone,position);
            s->start_bone=-1;memcpy(s->transform+12,position,sizeof position);s->transform[15]=1.f;
        }
        for(unsigned j=0;j<3;j++)s->velocity[j]*=.2f;
    }
    const float *parent=h->parent_position(h->context,s->parent_id);
    if(parent){
        for(unsigned j=0;j<3;j++)s->transform[12+j]+=parent[j]-s->parent_position[j];
        s->transform[15]=1.f;memcpy(s->parent_position,parent,sizeof s->parent_position);
    }
    if(s->age>1.f)return remove_shell(s,h);
    if(s->ground_y+5.f>=s->transform[13]){
        if(!s->bounce||s->velocity[1]>=-1.f)return remove_shell(s,h);
        s->velocity[1]=(float)(fabs((double)s->velocity[1])*(double).4f);
        h->bounce_sound(h->context,s);s->bounce=0;
    }
    float dt=(float)h->delta(h->context),zero=dt*0.f;
    s->velocity[0]=zero+s->velocity[0];s->velocity[1]+=dt*-980.f;s->velocity[2]+=zero;
    for(unsigned j=0;j<3;j++)s->velocity[j]*=.97f;
    dt=(float)h->delta(h->context);
    for(unsigned j=0;j<3;j++)s->transform[12+j]+=s->velocity[j]*dt;
    s->transform[15]=1.f;
    double age_delta=h->delta(h->context);s->age=(float)(age_delta+s->age);return 0;
}

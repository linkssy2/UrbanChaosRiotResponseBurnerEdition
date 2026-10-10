#include "scene_camera.h"
#include <string.h>
#include <math.h>
static int get(const UCEntityProperties*p,unsigned t,const char*n,void*out){int found;return uc_entity_properties_get(p,t,n,out,&found);}
int uc_scene_camera_construct(const UCEntityFreshContext*c,const UCEntityFreshView*v,
 UCSceneProjection*p,UCBulletMotion*m,UCSceneCamera*s,const UCEntityProperties*properties,
 const UCSceneCameraServices*h){
 if(!p||!m||!s||!properties||!h||!h->preflight||!h->current_viewport||!h->register_listener||!h->append||!h->auxiliary||!h->equipped_fov||!h->event_ids)return 1;
 float fov=65,fd=.2f,from[3]={0},fromd=.2f,to[3]={0},tod=.2f;int32_t target=-1;
 if(get(properties,2,"fovCurrent",&fov)||get(properties,2,"fovDamper",&fd)||get(properties,4,"fromOffset",from)||get(properties,2,"fromDamper",&fromd)||get(properties,4,"toOffset",to)||get(properties,2,"toDamper",&tod)||get(properties,1,"TargetEntity",&target))return 1;
 if(!isfinite(fov)||!isfinite(fd)||!isfinite(fromd)||!isfinite(tod))return 1;
 for(unsigned i=0;i<3;i++)if(!isfinite(from[i])||!isfinite(to[i]))return 1;
 if(!h->preflight(h->owner))return 1;
 int e=uc_scene_camera_base_construct(c,v,p,m,h->current_viewport,h->owner);if(e)return e;
 s->listener_count=s->listener_capacity=0;s->listener_owner=NULL;
 s->source_id=s->target_id=-1;s->source_enabled=s->target_enabled=0;s->source_tag=s->target_tag=s->target_entity=-1;s->mode=0;
 memset(s->target,0,sizeof s->target);memset(s->extra,0,sizeof s->extra);for(unsigned i=0;i<4;i++)s->extra[i*5]=1;
 s->effect_amplitude=s->effect_timer=0;
 p->fov=fov;s->fov_damper=fd;memcpy(s->from_offset,from,12);s->from_damper=fromd;memcpy(s->to_offset,to,12);s->to_damper=tod;s->target_entity=target;
 for(unsigned j=0;j<3;j++){float z=v->transform[8+j]*from[2]+v->transform[12+j];float x=v->transform[j]*from[0],y=v->transform[4+j]*from[1];v->transform[12+j]=(x+y)+z;}
 v->transform[15]=1;s->source_id=s->target_id=*v->target.parent_id;s->target_fov=p->fov;
 for(unsigned i=0;i<3;i++)if(!isfinite(v->transform[12+i]))return 2;
 uint32_t reserved;memcpy(&reserved,v->transform+13,4);
 s->listener_owner=v->target.identity;h->register_listener(h->owner,s,*v->id);
 for(unsigned i=0;i<7;i++){UCSceneCameraSubscription sub={h->event_ids[i],reserved,i,{0,0,0}};h->append(h->owner,s,&sub);}
 s->auxiliary=h->auxiliary(h->owner);const float*alternate=h->equipped_fov(h->owner);
 if(alternate){if(!isfinite(*alternate))return 2;p->alternate_fov=*alternate;p->alternate_enabled=1;}return 0;
}

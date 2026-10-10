#include "scene_camera_events.h"
#include <stdlib.h>
#include <string.h>
struct UCSceneCameraEvents {
    UCEntitySubscriptions *subscriptions;
    UCSceneCamera *camera;
    UCSceneProjection *projection;
    UCSceneCameraActivation activation;
};
static int source_tag(void *p,const void *data,uint32_t *sender){
    (void)sender;if(!data)return 1;UCSceneCamera*c=((UCSceneCameraEvents*)p)->camera;
    memcpy(&c->source_tag,data,4);c->source_enabled=1;return 0;
}
static int activate(void*p,const void*data,uint32_t*sender){
    (void)sender;if(!data)return 1;UCSceneCameraEvents*e=p;int32_t value;memcpy(&value,data,4);
    return e->activation.activate(e->activation.owner,e->camera,value);
}
static int target_offset(void*p,const void*data,uint32_t*sender){
    (void)sender;if(!data)return 1;memcpy(((UCSceneCameraEvents*)p)->camera->to_offset,data,12);return 0;
}
static int remove_camera(void*p,const void*data,uint32_t*sender){
    (void)data;(void)sender;UCSceneCameraEvents*e=p;return e->activation.remove(e->activation.owner,e->camera);
}
static int target_tag(void*p,const void*data,uint32_t*sender){
    (void)sender;if(!data)return 1;UCSceneCamera*c=((UCSceneCameraEvents*)p)->camera;
    memcpy(&c->target_tag,data,4);c->target_enabled=1;return 0;
}
static int fov(void*p,const void*data,uint32_t*sender){
    (void)sender;if(!data)return 1;UCSceneCameraEvents*e=p;
    memcpy(&e->camera->target_fov,data,4);memcpy(&e->projection->fov,data,4);return 0;
}
static int extra(void*p,const void*data,uint32_t*sender){
    (void)sender;if(!data)return 1;memcpy(((UCSceneCameraEvents*)p)->camera->extra,data,64);return 0;
}
int uc_scene_camera_events_create(UCSceneCamera*c,UCSceneProjection*p,
    const UCSceneCameraActivation*h,UCSceneCameraEvents**out){
    if(!c||!p||!h||!h->activate||!h->remove||!out)return 1;
    UCSceneCameraEvents*e=malloc(sizeof *e);if(!e)return 1;
    e->camera=c;e->projection=p;e->activation=*h;
    if(uc_entity_subscriptions_create(e,&e->subscriptions)){free(e);return 1;}
    *out=e;return 0;
}
UCEntityEventListener *uc_scene_camera_events_listener(UCSceneCameraEvents*e){return e?uc_entity_subscriptions_listener(e->subscriptions):NULL;}
int uc_scene_camera_events_append(UCSceneCameraEvents*e,const UCSceneCameraSubscription*s){
    static int(*const receiver[7])(void*,const void*,uint32_t*)={source_tag,activate,target_offset,remove_camera,target_tag,fov,extra};
    if(!e||!s||s->handler>=7||s->zero[0]||s->zero[1]||s->zero[2])return 1;
    UCEntityEventListener*l=uc_entity_subscriptions_listener(e->subscriptions);
    if(e->camera->listener_count!=l->count||e->camera->listener_capacity!=uc_entity_subscriptions_capacity(e->subscriptions))return 1;
    UCEntityEventSubscription entry={s->event,receiver[s->handler]};
    if(uc_entity_subscriptions_append(e->subscriptions,&entry))return 1;
    e->camera->listener_count=l->count;e->camera->listener_capacity=uc_entity_subscriptions_capacity(e->subscriptions);return 0;
}
void uc_scene_camera_events_close(UCSceneCameraEvents*e){if(e){uc_entity_subscriptions_close(e->subscriptions);free(e);}}

#include "glass_entity.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
int uc_glass_entity_properties(const UCLevelEntity*e,UCGlassEntityProperties*out){
    UCGlassEntityProperties p;uint32_t words[11];
    if(!e||!out||!e->class_name||strcmp(e->class_name,"GlassPane")||
       e->properties.size!=44||!e->properties.data)return 1;
    memcpy(words,e->properties.data,44);memcpy(&p.left,words,24);
    for(unsigned i=0;i<6;i++){float v;memcpy(&v,words+i,4);if(!isfinite(v)||fabsf(v)>1e6f)return 1;}
    p.auto_extents=(words[6]&255)!=0;p.on_broken[0]=words[7];p.on_broken[1]=words[8];
    p.npc_smash=(uint8_t)((words[9]&255)!=0);
    if(words[10]==UINT32_MAX)p.shader="";
    else{if(words[10]>=e->link_count||!e->links||!e->links[words[10]])return 1;p.shader=e->links[words[10]];}
    *out=p;return 0;
}
static int receive(void*c,const UCBulletSegment*s){
    UCGlassContainer*p=c;UCGlassBullet b=uc_glass_pane_bullet(p->pane);
    return uc_glass_bullet_receive(&b,s);
}
int uc_glass_entity_start(UCGlassEntity*p,const UCGlassEntityStart*c,
                         const char*name,const float world[16],const UCGlassEntityProperties*e){
    const void*found;void*bank;uint8_t inserted;
    if(!p||!c||!world||!e||p->model74||p->container11C||!c->factory||
       !c->current_resources||!c->current_observers||!c->bank_preflight||!c->load_bank||
       !c->air_pool||!c->ground_pool||!c->vectors||!c->random||!c->uv||!e->shader||
       uc_model_support_find(c->collision,".glass_panehk",&found)||found||
       uc_model_support_find(c->entity,".ENTSupp",&found)||found)return 1;
    for(unsigned i=0;i<16;i++)if(!isfinite(world[i])||fabsf(world[i])>1e6f)return 1;
    const float extents[]={e->left,e->right,e->up,e->down,e->collision_height};
    for(unsigned i=0;i<5;i++)if(!isfinite(extents[i])||fabsf(extents[i])>1e6f)return 1;
    if(c->bank_preflight(c->context,"Banks\\Glass.amp"))return 1;
    UCGlassProxy*proxy=calloc(1,sizeof *proxy);if(!proxy)return 1;
    int r=uc_glass_proxy_init(proxy,c->factory,name,c->collision,c->entity);
    p->model74=proxy;if(r)return 2;
    if(proxy->model){
        const float quad[4][2]={{0-e->left,0-e->down},{0-e->left,e->up},
            {e->right,e->up},{e->right,0-e->down}};
        UCGlassContainerInput in={proxy->model,c->air_pool,c->ground_pool,c->vectors,
            c->random,c->uv,world,quad,e->collision_height,e->shader,
            "FX\\FX_Glass\\FX_Glass_Shard.shader"};
        if(uc_glass_container_create(&in,c->current_resources,c->context,&p->container11C))return 2;
        p->observer.owner=p->container11C;p->observer.segment=receive;
        UCBulletObservers*observers=c->current_observers(c->context);
        if(uc_bullet_observers_add(observers,&p->observer,&inserted)||!inserted)return 2;
    }
    if(c->load_bank(c->context,"Banks\\Glass.amp",&bank))return 2;
    p->bank114=bank;p->state118=0;return 0;
}

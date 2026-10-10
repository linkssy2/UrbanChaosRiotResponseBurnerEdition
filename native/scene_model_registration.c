#include "scene_model_registration.h"
#include <stddef.h>

static int valid(const UCSceneModelGroup *g){
    UCSceneGroupListNode *n,*previous;uint32_t count=0;
    if(!g||!g->active||!g->memory.allocate||!g->memory.release||
       !g->graph.list||g->count>4096)return 0;
    previous=g->graph.list;n=previous->next;
    while(n!=g->graph.list){
        if(!n||!n->next||n->previous!=previous||++count>4096)return 0;
        previous=n;n=n->next;
    }
    return previous==g->graph.list->previous&&count==g->count;
}
int uc_scene_model_group_begin(UCSceneModelGroup *g,const UCSceneModelMemory *m){
    UCSceneModelNode *n;
    if(!g||g->active||!m||!m->allocate||!m->release)return 1;
    n=m->allocate(m->owner);if(!n)return 2;
    n->link.next=n->link.previous=&n->link;n->link.target=NULL;n->owned=0;
    g->empty_tree=(UCSceneGroupTreeNode){&g->empty_tree,&g->empty_tree,&g->empty_tree,NULL,1};
    g->graph=(UCSceneGroup){&g->empty_tree,&n->link};g->count=0;g->memory=*m;g->active=1;
    return 0;
}
int uc_scene_model_group_append(UCSceneModelGroup *g,void *target,uint8_t owned){
    UCSceneGroupListNode *head,*previous;UCSceneModelNode *n;
    if(!valid(g)||g->count==4096)return 1;
    head=g->graph.list;previous=head->previous;
    n=g->memory.allocate(g->memory.owner);if(!n)return 2;
    n->link=(UCSceneGroupListNode){head,previous,target};n->owned=owned;
    g->count++;
    head->previous=&n->link;previous->next=&n->link;
    return 0;
}
int uc_scene_model_group_remove(UCSceneModelGroup *g,const void *target,uint32_t *removed){
    UCSceneGroupListNode *n,*next;uint32_t count=0;
    if(!removed||!valid(g))return 1;
    n=g->graph.list->next;
    while(n!=g->graph.list){
        next=n->next;
        if(n->target==target){
            n->previous->next=next;next->previous=n->previous;
            g->memory.release(g->memory.owner,(UCSceneModelNode *)n);
            g->count--;count++;
        }
        n=next;
    }
    *removed=count;return 0;
}
int uc_scene_model_group_end(UCSceneModelGroup *g){
    if(!valid(g)||g->count)return 1;
    g->memory.release(g->memory.owner,(UCSceneModelNode *)g->graph.list);
    g->graph.list=NULL;g->active=0;return 0;
}
int uc_scene_model_create(void *const *factory,const void *resource,
                          void *initialization,const char *name,
                          const UCSceneModelCreateHooks *h,void *owner,void **out){
    void *model;UCSceneModelGroup *g;
    if(!factory||!h||!out||!h->preflight||!h->create||!h->initialize||!h->current_primary||!h->name||
       !h->preflight(owner,resource,initialization,name))return 1;
    model=h->create(owner,*factory,resource);*out=model;
    if(!model)return 0;
    h->initialize(owner,model,initialization);
    g=h->current_primary(owner);
    if(uc_scene_model_group_append(g,model,0))return 2;
    h->name(owner,model,name?name:"");
    return 0;
}

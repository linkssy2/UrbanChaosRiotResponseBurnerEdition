#include "scene_group_collect.h"

static UCSceneGroupTreeNode *next(UCSceneGroupTreeNode *n){
    UCSceneGroupTreeNode *p;
    if(n->nil)return n;
    if(!n->right->nil){
        n=n->right;
        while(!n->left->nil)n=n->left;
        return n;
    }
    p=n->parent;
    while(!p->nil&&n==p->right){n=p;p=p->parent;}
    return p;
}
/* Walk each edge once without recursion or a second allocated graph. */
int uc_scene_group_validate(const UCSceneGroup *g){
    UCSceneGroupTreeNode *h,*n,*last;
    UCSceneGroupListNode *l,*previous;
    uint32_t count=0,steps=0;
    if(!g||!(h=g->tree)||!h->nil||!h->left||!h->right||!h->parent||!g->list)return 1;
    if(h->parent==h){if(h->left!=h||h->right!=h)return 1;}
    else{
        if(h->parent->nil||h->parent->parent!=h)return 1;
        n=h->parent;
        while(!n->left||!n->right||!n->parent||n->nil||n->left!=h){
            if(!n->left||!n->right||!n->parent||n->nil||++steps>4096)return 1;
            if(n->left->parent!=n)return 1;
            n=n->left;
        }
        if(n!=h->left)return 1;
        last=h;
        while(n!=h){
            UCSceneGroupTreeNode *p;
            if(!n||n->nil||!n->left||!n->right||!n->parent||++count>4096)return 1;
            if(n->left!=h&&(n->left->nil||n->left->parent!=n))return 1;
            if(n->right!=h&&(n->right->nil||n->right->parent!=n))return 1;
            last=n;
            if(n->right!=h){
                n=n->right;
                while(n->left!=h){
                    if(!n->left||++steps>8192||n->left->nil||n->left->parent!=n)return 1;
                    n=n->left;
                }
            }else{
                p=n->parent;
                while(p!=h&&n==p->right){
                    if(!p->parent||++steps>8192||p->nil)return 1;
                    n=p;p=p->parent;
                }
                if(p!=h&&p->left!=n)return 1;
                n=p;
            }
        }
        if(last!=h->right)return 1;
    }
    previous=g->list;l=previous->next;count=0;
    if(!l||!previous->previous)return 1;
    while(l!=g->list){
        if(!l||!l->next||l->previous!=previous||!l->target||++count>4096)return 1;
        previous=l;l=l->next;
    }
    return g->list->previous!=previous;
}
int uc_scene_group_collect(UCSceneGroup *g,uint32_t slot,void *collector,
                           void *manager,UCSceneGroupCollect call,void *owner){
    UCSceneGroupTreeNode *t;UCSceneGroupListNode *l;
    if(!call||(slot!=0x40&&slot!=0x44)||uc_scene_group_validate(g))return 1;
    t=g->tree->left;
    while(t!=g->tree){
        if(t->target)call(owner,t->target,slot,collector,manager);
        t=next(t);
    }
    l=g->list->next;
    while(l!=g->list){
        call(owner,l->target,slot,collector,manager);
        l=l->next;
    }
    return 0;
}

#include "scene_geometry_tree.h"

typedef UCSceneGroupTreeNode Link;
static UCSceneGeometryNode *node(Link *n){return (UCSceneGeometryNode *)n;}
static int valid(const UCSceneGeometryTree *t){
    return t&&t->active==1&&t->header&&t->count<=4096&&t->memory.allocate&&t->memory.release;
}
static void rotate(UCSceneGeometryTree *t,Link *p,int left){
    Link *q=left?p->right:p->left,*child=left?q->left:q->right;
    if(left)p->right=child;else p->left=child;
    if(!child->nil)child->parent=p;
    q->parent=p->parent;
    if(p==t->header->parent)t->header->parent=q;
    else if(p==p->parent->left)p->parent->left=q;
    else p->parent->right=q;
    if(left)q->left=p;else q->right=p;
    p->parent=q;
}
int uc_scene_geometry_tree_begin(UCSceneGeometryTree *t,const UCSceneGeometryMemory *m){
    UCSceneGeometryNode *n;
    if(!t||!m||!m->allocate||!m->release||t->active||t->header||t->count)return 1;
    n=m->allocate(m->owner);if(!n)return 2;
    n->link.left=n->link.parent=n->link.right=&n->link;
    n->link.target=0;n->link.nil=1;n->color=1;n->key=0;
    t->memory=*m;t->header=&n->link;t->count=0;t->active=1;return 0;
}
int uc_scene_geometry_tree_set(UCSceneGeometryTree *t,uint32_t key,void *target){
    Link *h,*p,*q,*fresh;int left=1;
    if(!valid(t))return 1;
    h=t->header;p=h;q=h->parent;
    while(!q->nil){
        if(key==node(q)->key){q->target=target;return 0;}
        p=q;left=key<node(q)->key;q=left?q->left:q->right;
    }
    if(t->count==4096)return 1;
    UCSceneGeometryNode *n=t->memory.allocate(t->memory.owner);if(!n)return 2;
    q=fresh=&n->link;q->left=q->right=h;q->parent=p;q->target=target;
    q->nil=0;n->key=key;n->color=0;t->count++;
    if(p==h)h->parent=h->left=h->right=q;
    else if(left){p->left=q;if(p==h->left)h->left=q;}
    else{p->right=q;if(p==h->right)h->right=q;}
    while(node(q->parent)->color==0){
        p=q->parent;Link *g=p->parent;int side=p==g->left;Link *uncle=side?g->right:g->left;
        if(node(uncle)->color==0){node(p)->color=node(uncle)->color=1;node(g)->color=0;q=g;}
        else{
            if(q==(side?p->right:p->left)){q=p;rotate(t,q,side);}
            node(q->parent)->color=1;node(q->parent->parent)->color=0;
            rotate(t,q->parent->parent,!side);
        }
    }
    node(h->parent)->color=1;fresh->target=target;return 0;
}
int uc_scene_geometry_tree_find(const UCSceneGeometryTree *t,uint32_t key,void **target){
    Link *p;if(!valid(t)||!target)return 1;p=t->header->parent;
    while(!p->nil){if(key==node(p)->key){*target=p->target;return 0;}p=key<node(p)->key?p->left:p->right;}
    return 1;
}
static void clear_nodes(UCSceneGeometryTree *t,Link *n){
    while(!n->nil){Link *old=n;clear_nodes(t,n->right);n=n->left;t->memory.release(t->memory.owner,node(old));}
}
int uc_scene_geometry_tree_clear(UCSceneGeometryTree *t){
    Link *h;if(!valid(t))return 1;h=t->header;clear_nodes(t,h->parent);
    h->parent=h;t->count=0;h->left=h;h->right=h;return 0;
}
int uc_scene_geometry_tree_end(UCSceneGeometryTree *t){
    if(uc_scene_geometry_tree_clear(t))return 1;
    t->memory.release(t->memory.owner,node(t->header));t->header=0;t->count=0;t->active=0;return 0;
}

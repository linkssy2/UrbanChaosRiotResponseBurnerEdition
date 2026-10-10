#include "collision_sap.h"
#include <fenv.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <xmmintrin.h>

struct UCCollisionSAP {
    UCCollisionSAPView view;
    uint32_t capacity;
    UCCollisionSAPNode *nodes;
    UCCollisionSAPEndpoint *axis[3];
};

static int float_mode(void) {
    return fegetround()==FE_TONEAREST && (_mm_getcsr()&0xE040u)==0;
}
static int finite4(const float *p) {
    return p && isfinite(p[0]) && isfinite(p[1]) && isfinite(p[2]) && isfinite(p[3]);
}
static void quantize(const float p[4],const float offset[4],const float scale[4],uint16_t out[3]) {
    uint32_t bits[4];
    __m128 v=_mm_mul_ps(_mm_add_ps(_mm_loadu_ps(p),_mm_loadu_ps(offset)),_mm_loadu_ps(scale));
    v=_mm_min_ps(v,_mm_set1_ps(65532.0f));
    v=_mm_max_ps(v,_mm_setzero_ps());
    v=_mm_add_ps(v,_mm_set1_ps(65536.0f));
    memcpy(bits,&v,sizeof(bits));
    for(unsigned a=0;a<3;a++)out[a]=(uint16_t)(bits[a]>>7);
}
static float calibration(void) {
    float low=10,high=11;
    const float zero[4]={0},one[4]={1,1,1,1};
    for(unsigned i=0;i<23;i++) {
        double mid=((double)high+low)*0.5;
        float p[4]={(float)(mid+1),0,0,0};uint16_t q[3];
        quantize(p,zero,one,q);
        if((q[0]|1u)<12)low=(float)mid;else high=(float)mid;
    }
    return (float)(((double)high+low)*0.5-11.0);
}
UCCollisionStatus uc_collision_sap_create(const float lower[4],const float upper[4],
    uint32_t capacity,UCCollisionSAP **out) {
    UCCollisionSAPView v={0};float range[4],reciprocal[4]={0};uint32_t magic=0x37800200;float margin;
    if(!out||!finite4(lower)||!finite4(upper))return UC_COLLISION_ARGUMENT;
    if(!float_mode())return UC_COLLISION_UNSUPPORTED;
    if(!capacity||capacity>UC_COLLISION_SAP_MAX_BODIES)return UC_COLLISION_LIMIT;
    _mm_storeu_ps(range,_mm_sub_ps(_mm_loadu_ps(upper),_mm_loadu_ps(lower)));
    for(unsigned a=0;a<3;a++) {
        if(!(lower[a]<upper[a])||!isfinite(range[a]))return UC_COLLISION_ARGUMENT;
        reciprocal[a]=(float)(1.0/(double)range[a]);
    }
    _mm_storeu_ps(v.scale,_mm_mul_ps(_mm_set1_ps(65532),_mm_loadu_ps(reciprocal)));
    memcpy(&margin,&magic,4);
    _mm_storeu_ps(v.offset_low,_mm_xor_ps(_mm_loadu_ps(lower),_mm_set1_ps(-0.0f)));
    _mm_storeu_ps(v.offset_high,_mm_add_ps(_mm_loadu_ps(v.offset_low),_mm_mul_ps(_mm_set1_ps(margin),_mm_loadu_ps(range))));
    v.scale[3]=v.offset_low[3]=v.offset_high[3]=0;
    if(!finite4(v.scale)||!finite4(v.offset_high))return UC_COLLISION_ARGUMENT;
    v.bias=calibration();v.node_count=1;v.endpoint_count=2;
    UCCollisionSAP *s=(UCCollisionSAP*)calloc(1,sizeof(*s));
    if(!s)return UC_COLLISION_MEMORY;
    s->capacity=capacity;s->view=v;
    s->nodes=(UCCollisionSAPNode*)calloc((size_t)capacity+1,sizeof(*s->nodes));
    for(unsigned a=0;a<3;a++)s->axis[a]=(UCCollisionSAPEndpoint*)malloc(((size_t)capacity+1)*2*sizeof(*s->axis[a]));
    if(!s->nodes||!s->axis[0]||!s->axis[1]||!s->axis[2]) {
        free(s->nodes);for(unsigned a=0;a<3;a++)free(s->axis[a]);free(s);return UC_COLLISION_MEMORY;
    }
    s->view.nodes=s->nodes;
    for(unsigned a=0;a<3;a++) {
        s->view.axis[a]=s->axis[a];s->nodes[0].max[a]=1;
        s->axis[a][0]=(UCCollisionSAPEndpoint){0,0};
        s->axis[a][1]=(UCCollisionSAPEndpoint){0xFFFD,0};
    }
    *out=s;return UC_COLLISION_OK;
}
void uc_collision_sap_free(UCCollisionSAP *s) {
    if(!s)return;
    for(uint32_t i=1;i<s->view.node_count;i++)s->nodes[i].handle->index=UC_COLLISION_SAP_DETACHED;
    free(s->nodes);for(unsigned a=0;a<3;a++)free(s->axis[a]);free(s);
}
const UCCollisionSAPView *uc_collision_sap_view(const UCCollisionSAP *s) {return s?&s->view:NULL;}
static int pairs_valid(const UCCollisionSAPPairs *p) {
    return p && p->count<=p->capacity && (!p->capacity||p->data);
}
static int overlaps(const UCCollisionSAP *s,uint32_t i,const uint16_t low[3],const uint16_t high[3]) {
    for(unsigned a=0;a<3;a++) {
        const UCCollisionSAPNode *n=s->nodes+i;
        if(high[a]<s->axis[a][n->min[a]].value||low[a]>s->axis[a][n->max[a]].value)return 0;
    }
    return 1;
}
static uint32_t pair_count(const UCCollisionSAP *s,uint32_t skip,const uint16_t low[3],const uint16_t high[3]) {
    uint32_t count=0;
    for(uint32_t i=1;i<s->view.node_count;i++)if(i!=skip&&overlaps(s,i,low,high))count++;
    return count;
}
static void append_pairs(const UCCollisionSAP *s,UCCollisionSAPHandle *h,uint32_t skip,
    const uint16_t low[3],const uint16_t high[3],UCCollisionSAPPairs *p) {
    for(uint32_t i=1;i<s->view.node_count;i++)if(i!=skip&&overlaps(s,i,low,high))
        p->data[p->count++]=(UCCollisionSAPPair){h,s->nodes[i].handle};
}
static void index_axis(UCCollisionSAP *s,unsigned a) {
    for(uint32_t i=0;i<s->view.endpoint_count;i++) {
        UCCollisionSAPEndpoint e=s->axis[a][i];
        if(e.value&1)s->nodes[e.node].max[a]=(uint16_t)i;else s->nodes[e.node].min[a]=(uint16_t)i;
    }
}
UCCollisionStatus uc_collision_sap_add(UCCollisionSAP *s,UCCollisionSAPHandle *h,
    const float lower[4],const float upper[4],UCCollisionSAPPairs *pairs) {
    uint16_t low[3],high[3];
    if(!s||!h||!h->identity||h->index!=UC_COLLISION_SAP_DETACHED||!pairs_valid(pairs)||!finite4(lower)||!finite4(upper))return UC_COLLISION_ARGUMENT;
    if(!float_mode())return UC_COLLISION_UNSUPPORTED;
    if(s->view.node_count>s->capacity)return UC_COLLISION_LIMIT;
    for(unsigned a=0;a<3;a++)if(lower[a]>upper[a])return UC_COLLISION_ARGUMENT;
    quantize(lower,s->view.offset_low,s->view.scale,low);quantize(upper,s->view.offset_high,s->view.scale,high);
    for(unsigned a=0;a<3;a++){low[a]&=0xFFFE;high[a]|=1;}
    if(pair_count(s,0,low,high)>pairs->capacity-pairs->count)return UC_COLLISION_LIMIT;
    append_pairs(s,h,0,low,high,pairs);
    uint32_t n=s->view.node_count++,count=s->view.endpoint_count;
    s->nodes[n].handle=h;h->index=n;s->view.endpoint_count+=2;
    for(unsigned a=0;a<3;a++) {
        UCCollisionSAPEndpoint *e=s->axis[a];uint32_t i=count;
        /* 325140: insert maximum before equal maxima, minimum after equal minima. */
        while(i&&e[i-1].value>=high[a]){e[i]=e[i-1];i--;}
        e[i]=(UCCollisionSAPEndpoint){high[a],(uint16_t)n};i=count+1;
        while(i&&e[i-1].value>low[a]){e[i]=e[i-1];i--;}
        e[i]=(UCCollisionSAPEndpoint){low[a],(uint16_t)n};index_axis(s,a);
    }
    return UC_COLLISION_OK;
}
UCCollisionStatus uc_collision_sap_remove(UCCollisionSAP *s,UCCollisionSAPHandle *h,UCCollisionSAPPairs *pairs) {
    uint16_t low[3],high[3];
    if(!s||!h||!pairs_valid(pairs)||!h->index||h->index>=s->view.node_count||s->nodes[h->index].handle!=h)return UC_COLLISION_ARGUMENT;
    if(!float_mode())return UC_COLLISION_UNSUPPORTED;
    uint32_t n=h->index,last=s->view.node_count-1;
    for(unsigned a=0;a<3;a++){low[a]=s->axis[a][s->nodes[n].min[a]].value;high[a]=s->axis[a][s->nodes[n].max[a]].value;}
    if(pair_count(s,n,low,high)>pairs->capacity-pairs->count)return UC_COLLISION_LIMIT;
    append_pairs(s,h,n,low,high,pairs);
    for(unsigned a=0;a<3;a++) {
        UCCollisionSAPEndpoint *e=s->axis[a];uint32_t j=0;
        for(uint32_t i=0;i<s->view.endpoint_count;i++)if(e[i].node!=n) {
            e[j]=e[i];if(e[j].node==last)e[j].node=(uint16_t)n;j++;
        }
    }
    s->nodes[n]=s->nodes[last];s->view.node_count--;s->view.endpoint_count-=2;
    if(n!=last)s->nodes[n].handle->index=n;
    h->index=UC_COLLISION_SAP_DETACHED;
    for(unsigned a=0;a<3;a++)index_axis(s,a);
    return UC_COLLISION_OK;
}

typedef struct BatchWork {
    void *allocation;
    UCCollisionSAPNode *nodes;
    UCCollisionSAPEndpoint *axis[3],*merge;
    uintptr_t *identities;
    uint16_t *active[2];
    uint8_t *selected;
} BatchWork;
static int identity_order(const void *a,const void *b) {
    uintptr_t x=*(const uintptr_t*)a,y=*(const uintptr_t*)b;return (x>y)-(x<y);
}
static UCCollisionStatus batch_work(UCCollisionSAP *s,UCCollisionSAPHandle *const *handles,
    const UCCollisionSAPBounds *bounds,uint32_t count,int adding,BatchWork *w) {
    uint32_t old=s->view.node_count,n=old+(adding?count:0);
    if(count>(adding?s->capacity+1-old:old-1))return UC_COLLISION_LIMIT;
    for(uint32_t i=0;i<count;i++) {
        const UCCollisionSAPHandle *h=handles[i];
        if(!h||!h->identity)return UC_COLLISION_ARGUMENT;
        if(adding) {
            if(h->index!=UC_COLLISION_SAP_DETACHED||!finite4(bounds[i].lower)||!finite4(bounds[i].upper))return UC_COLLISION_ARGUMENT;
            for(unsigned a=0;a<3;a++)if(bounds[i].lower[a]>bounds[i].upper[a])return UC_COLLISION_ARGUMENT;
        } else if(!h->index||h->index>=old||s->nodes[h->index].handle!=h)return UC_COLLISION_ARGUMENT;
    }
    size_t bytes=(size_t)count*sizeof(uintptr_t)+(size_t)n*(2*sizeof(uint16_t)+sizeof(uint8_t));
    if(adding)bytes+=(size_t)n*(sizeof(UCCollisionSAPNode)+8*sizeof(UCCollisionSAPEndpoint));
    uint8_t *p=malloc(bytes);if(!p)return UC_COLLISION_MEMORY;w->allocation=p;
    if(adding){w->nodes=(UCCollisionSAPNode*)p;p+=(size_t)n*sizeof(*w->nodes);}
    w->identities=(uintptr_t*)p;p+=(size_t)count*sizeof(*w->identities);
    if(adding) {
        for(unsigned a=0;a<3;a++){w->axis[a]=(UCCollisionSAPEndpoint*)p;p+=(size_t)2*n*sizeof(*w->axis[a]);}
        w->merge=(UCCollisionSAPEndpoint*)p;p+=(size_t)2*n*sizeof(*w->merge);
    }
    for(unsigned k=0;k<2;k++){w->active[k]=(uint16_t*)p;p+=(size_t)n*sizeof(*w->active[k]);}
    w->selected=p;memset(w->selected,0,n);
    for(uint32_t i=0;i<count;i++)w->identities[i]=(uintptr_t)handles[i]->identity;
    qsort(w->identities,count,sizeof(*w->identities),identity_order);
    for(uint32_t i=1;i<count;i++)if(w->identities[i-1]==w->identities[i])return UC_COLLISION_ARGUMENT;
    if(adding)for(uint32_t i=1;i<old;i++) {
        uintptr_t identity=(uintptr_t)s->nodes[i].handle->identity;
        if(bsearch(&identity,w->identities,count,sizeof(identity),identity_order))return UC_COLLISION_ARGUMENT;
    }
    for(uint32_t i=0;i<count;i++)w->selected[adding?old+i:handles[i]->index]=1;
    return UC_COLLISION_OK;
}
/* 324470 pivot and equal-value swaps. Disjoint partitions can be visited in
 * either order; recurse into the smaller one to bound native stack use. */
static void batch_sort(UCCollisionSAPEndpoint *e,int low,int high) {
    while(low<high) {
        int left=low,right=high;uint16_t pivot=e[(low+high)/2].value;
        do {
            while(e[left].value<pivot)++left;
            while(pivot<e[right].value)--right;
            if(right<left)break;
            if(left!=right){UCCollisionSAPEndpoint swap=e[left];e[left]=e[right];e[right]=swap;}
            --right;++left;
        }while(left<=right);
        if(right-low<high-left) {if(low<right)batch_sort(e,low,right);low=left;}
        else {if(left<high)batch_sort(e,left,high);high=right;}
    }
}
static void batch_index(UCCollisionSAPNode *nodes,const UCCollisionSAPEndpoint *e,uint32_t count,unsigned axis) {
    for(uint32_t i=0;i<count;i++) {
        if(e[i].value&1)nodes[e[i].node].max[axis]=(uint16_t)i;
        else nodes[e[i].node].min[axis]=(uint16_t)i;
    }
}
static int batch_yz(const UCCollisionSAPNode *a,const UCCollisionSAPNode *b) {
    return a->min[1]<=b->max[1]&&b->min[1]<=a->max[1]&&
           a->min[2]<=b->max[2]&&b->min[2]<=a->max[2];
}
/* 327BF0 unpartitioned slice. NULL output counts without publishing anything. */
static uint32_t batch_pairs(const UCCollisionSAPNode *nodes,const UCCollisionSAPEndpoint *axis,
    uint32_t endpoints,BatchWork *w,UCCollisionSAPPairs *out) {
    uint32_t active_count[2]={0},count=0;
    for(uint32_t i=1;i+1<endpoints;i++) {
        uint16_t node=axis[i].node;unsigned selected=w->selected[node];
        uint16_t *active=w->active[selected];
        if(axis[i].value&1) {
            uint32_t at=0;while(active[at]!=node)++at;
            active[at]=active[--active_count[selected]];
        } else {
            for(unsigned pass=0;pass<=selected;pass++) {
                unsigned list=1-pass;
                for(uint32_t j=0;j<active_count[list];j++) {
                    uint16_t other=w->active[list][j];
                    if(batch_yz(nodes+node,nodes+other)) {
                        if(out)out->data[out->count++]=(UCCollisionSAPPair){nodes[other].handle,nodes[node].handle};
                        ++count;
                    }
                }
            }
            active[active_count[selected]++]=node;
        }
    }
    return count;
}
UCCollisionStatus uc_collision_sap_add_batch(UCCollisionSAP *s,UCCollisionSAPHandle *const *handles,
    const UCCollisionSAPBounds *bounds,uint32_t count,UCCollisionSAPPairs *pairs) {
    if(!s||!pairs_valid(pairs)||(count&&(!handles||!bounds)))return UC_COLLISION_ARGUMENT;
    if(!float_mode())return UC_COLLISION_UNSUPPORTED;
    if(!count)return UC_COLLISION_OK;
    BatchWork w={0};UCCollisionStatus status=batch_work(s,handles,bounds,count,1,&w);
    if(status!=UC_COLLISION_OK){free(w.allocation);return status;}
    uint32_t old=s->view.node_count,n=old+count,old_end=2*old,end=2*n;
    memcpy(w.nodes,s->nodes,(size_t)old*sizeof(*w.nodes));
    for(unsigned a=0;a<3;a++)memcpy(w.axis[a],s->axis[a],(size_t)old_end*sizeof(*w.axis[a]));
    for(uint32_t i=0;i<count;i++) {
        uint16_t low[3],high[3];quantize(bounds[i].lower,s->view.offset_low,s->view.scale,low);
        quantize(bounds[i].upper,s->view.offset_high,s->view.scale,high);w.nodes[old+i].handle=handles[i];
        for(unsigned a=0;a<3;a++) {
            w.axis[a][old_end+i*2]=(UCCollisionSAPEndpoint){(uint16_t)(low[a]&0xFFFE),(uint16_t)(old+i)};
            w.axis[a][old_end+i*2+1]=(UCCollisionSAPEndpoint){(uint16_t)(high[a]|1),(uint16_t)(old+i)};
        }
    }
    for(unsigned a=0;a<3;a++) {
        UCCollisionSAPEndpoint *e=w.axis[a];batch_sort(e+old_end,0,(int)(count*2)-1);
        uint32_t before=1,added=old_end,dest=1;w.merge[0]=e[0];
        while(before<old_end&&added<end)w.merge[dest++]=e[e[before].value<e[added].value?before++:added++];
        while(before<old_end)w.merge[dest++]=e[before++];
        while(added<end)w.merge[dest++]=e[added++];
        memcpy(e,w.merge,(size_t)end*sizeof(*e));batch_index(w.nodes,e,end,a);
    }
    if(batch_pairs(w.nodes,w.axis[0],end,&w,NULL)>pairs->capacity-pairs->count)status=UC_COLLISION_LIMIT;
    else {
        memcpy(s->nodes,w.nodes,(size_t)n*sizeof(*s->nodes));
        for(unsigned a=0;a<3;a++)memcpy(s->axis[a],w.axis[a],(size_t)end*sizeof(*s->axis[a]));
        s->view.node_count=n;s->view.endpoint_count=end;
        for(uint32_t i=0;i<count;i++)handles[i]->index=old+i;
        batch_pairs(w.nodes,w.axis[0],end,&w,pairs);
    }
    free(w.allocation);return status;
}
UCCollisionStatus uc_collision_sap_remove_batch(UCCollisionSAP *s,UCCollisionSAPHandle *const *handles,
    uint32_t count,UCCollisionSAPPairs *pairs) {
    if(!s||!pairs_valid(pairs)||(count&&!handles))return UC_COLLISION_ARGUMENT;
    if(!float_mode())return UC_COLLISION_UNSUPPORTED;
    if(!count)return UC_COLLISION_OK;
    BatchWork w={0};UCCollisionStatus status=batch_work(s,handles,NULL,count,0,&w);
    if(status!=UC_COLLISION_OK){free(w.allocation);return status;}
    uint32_t old_end=s->view.endpoint_count;
    if(batch_pairs(s->nodes,s->axis[0],old_end,&w,NULL)>pairs->capacity-pairs->count)status=UC_COLLISION_LIMIT;
    else {
        batch_pairs(s->nodes,s->axis[0],old_end,&w,pairs);
        uint16_t *map=w.active[0];uint32_t low=0,high=s->view.node_count-1;
        for(uint32_t i=0;i<count;i++)handles[i]->index=UC_COLLISION_SAP_DETACHED;
        while(low<=high) {
            if(w.selected[high]){map[high--]=UINT16_MAX;continue;}
            if(w.selected[low]) {
                s->nodes[low]=s->nodes[high];map[high--]=(uint16_t)low;map[low]=UINT16_MAX;
                s->nodes[low].handle->index=low;
            } else map[low]=(uint16_t)low;
            ++low;
        }
        s->view.node_count=high+1;s->view.endpoint_count=2*(high+1);
        for(unsigned a=0;a<3;a++) {
            uint32_t dest=0;UCCollisionSAPEndpoint *e=s->axis[a];
            for(uint32_t i=0;i<old_end;i++)if(map[e[i].node]!=UINT16_MAX) {
                UCCollisionSAPEndpoint value=e[i];value.node=map[value.node];e[dest++]=value;
            }
            batch_index(s->nodes,e,s->view.endpoint_count,a);
        }
    }
    free(w.allocation);return status;
}

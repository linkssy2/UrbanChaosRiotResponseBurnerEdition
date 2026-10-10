#include "scene_asset.h"
#include "scene_models.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define BYTE_LIMIT (256u * 1024u * 1024u)
#define VERTEX_LIMIT 1000000u
#define INDEX_LIMIT 8000000u
#define RECORD_LIMIT 100000u
static const float identity[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
typedef struct Reader {
    const uint8_t *data;
    size_t size, pos, allocated;
    UCScene *scene;
    UCSceneModels *models;
    UCSceneStatus status;
} Reader;
static uint16_t u16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static uint32_t u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
}
static const uint8_t *take(Reader *r, size_t n) {
    const uint8_t *p;
    if (n > r->size-r->pos) { r->status=UC_SCENE_FORMAT; return NULL; }
    p=r->data+r->pos; r->pos+=n; return p;
}
static int grow(Reader *r, void **p, size_t old_count, size_t count, size_t stride) {
    size_t extra=(count-old_count)*stride;
    void *next;
    if (extra > BYTE_LIMIT-r->allocated) { r->status=UC_SCENE_LIMIT; return 0; }
    if (!extra) return 1;
    next=realloc(*p, count*stride);
    if (!next) { r->status=UC_SCENE_MEMORY; return 0; }
    *p=next; r->allocated+=extra; return 1;
}
#define NEED(p) do { if (!(p)) return 0; } while (0)
#define VALID(c) do { if (!(c)) { r->status=UC_SCENE_FORMAT; return 0; } } while (0)
#define SUPPORTED(c) do { if (!(c)) { r->status=UC_SCENE_UNSUPPORTED; return 0; } } while (0)
#define LIMITED(c) do { if (!(c)) { r->status=UC_SCENE_LIMIT; return 0; } } while (0)
static int names(Reader *r, uint32_t count) {
    UCScene *s=r->scene;
    uint32_t first=s->material_count, i;
    LIMITED(count <= RECORD_LIMIT-first);
    NEED(grow(r,(void **)&s->material_names,first,first+count,sizeof(char *)));
    if (count) memset(s->material_names+first,0,count*sizeof(char *));
    s->material_count+=count;
    for (i=first; i<s->material_count; ++i) {
        const uint8_t *p=take(r,2); uint16_t n;
        NEED(p); n=u16(p); SUPPORTED(n>0 && n<256);
        p=take(r,n); NEED(p); SUPPORTED(memchr(p,0,n)==NULL);
        NEED(grow(r,(void **)&s->material_names[i],0,(size_t)n+1,1));
        memcpy(s->material_names[i],p,n); s->material_names[i][n]=0;
    }
    return 1;
}
static int node(Reader *r, uint32_t materials, uint32_t material_base, uint32_t sector) {
    UCScene *s=r->scene;
    const uint8_t *h=take(r,112), *position, *basis=NULL, *color=NULL, *uv, *uv1=NULL, *draw, *indices, *p;
    uint32_t nv, nd, ni, flags, i, j, output_indices=0, vertex_base=s->vertex_count, batch_base=s->batch_count;
    NEED(h); nv=u32(h+8); nd=u32(h+12); ni=u32(h+16); flags=u32(h+28);
    SUPPORTED(u32(h+4)==0 && u32(h+20)==0 && !(flags&~56u));
    LIMITED(u32(h+24)<=RECORD_LIMIT);
    if (!nv) { VALID(nd==0 && ni==0); return 1; }
    LIMITED(nv <= VERTEX_LIMIT-vertex_base && nd <= RECORD_LIMIT-batch_base && ni<=INDEX_LIMIT);
    position=take(r,(size_t)nv*16); NEED(position);
    if (!(flags&16)) { basis=take(r,(size_t)nv*8); NEED(basis); }
    if (!(flags&8)) { color=take(r,(size_t)nv*4); NEED(color); }
    uv=take(r,(size_t)nv*8); NEED(uv);
    if (!(flags&32)) { uv1=take(r,(size_t)nv*8); NEED(uv1); }
    draw=take(r,(size_t)nd*20); NEED(draw);
    NEED(take(r,(size_t)u32(h+24)*20)); /* Auxiliary descriptors, not base draw list. */
    indices=take(r,(size_t)ni*2); NEED(indices);
    p=take(r,4); NEED(p); SUPPORTED(u32(p)==0);
    for (i=0; i<nd; ++i) {
        const uint8_t *d=draw+i*20;
        uint32_t start=u32(d), triangles=u16(d+12), count;
        SUPPORTED(d[14]==4 || d[14]==5);
        count=d[14]==4 ? triangles*3 : triangles+2;
        VALID(u16(d+4)<materials && (u16(d+6)==65535 || u16(d+6)<s->light_count));
        VALID(start<=ni && count<=ni-start);
        for (j=0; j<count; ++j) VALID(u16(indices+((size_t)start+j)*2)<nv);
        LIMITED(triangles*3<=INDEX_LIMIT-output_indices);
        output_indices+=triangles*3;
    }
    LIMITED(output_indices<=INDEX_LIMIT-s->index_count);
    NEED(grow(r,(void **)&s->vertices,vertex_base,vertex_base+nv,sizeof(*s->vertices)));
    NEED(grow(r,(void **)&s->indices,s->index_count,s->index_count+output_indices,sizeof(*s->indices)));
    NEED(grow(r,(void **)&s->batches,batch_base,batch_base+nd,sizeof(*s->batches)));
    s->vertex_count+=nv;
    for (i=0; i<nv; ++i) {
        UCSceneVertex *v=&s->vertices[vertex_base+i];
        memset(v,0,sizeof(*v));
        memcpy(v->position,position+(size_t)i*16,12);
        v->packed_normal=u32(position+(size_t)i*16+12);
        memcpy(v->uv,uv+(size_t)i*8,8);
        v->source_color=color ? u32(color+(size_t)i*4) : 0xffffffffu;
        if (uv1) memcpy(v->uv1,uv1+(size_t)i*8,8);
        if (basis) { v->packed_basis[0]=u32(basis+(size_t)i*8); v->packed_basis[1]=u32(basis+(size_t)i*8+4); }
        for (j=0; j<3; ++j) {
            VALID(isfinite(v->position[j]));
            if (v->position[j]<s->bounds_min[j]) s->bounds_min[j]=v->position[j];
            if (v->position[j]>s->bounds_max[j]) s->bounds_max[j]=v->position[j];
        }
        VALID(isfinite(v->uv[0]) && isfinite(v->uv[1]) && isfinite(v->uv1[0]) && isfinite(v->uv1[1]));
    }
    for (i=0; i<nd; ++i) {
        const uint8_t *d=draw+i*20;
        uint32_t start=u32(d), triangles=u16(d+12);
        UCSceneBatch *b=&s->batches[s->batch_count++];
        memset(b,0,sizeof(*b)); b->first_index=s->index_count; b->index_count=triangles*3;
        b->material_index=material_base+u16(d+4); b->light_index=u16(d+6);
        b->sector_index=sector; b->node_flags=flags; b->flags[0]=d[15]; b->flags[1]=d[16];
        memcpy(b->local_to_world,identity,sizeof(identity));
        for (j=0; j<triangles; ++j) {
            uint32_t first=start+(d[14]==4 ? j*3 : j), flip=d[14]==5 ? (j&1) : 0;
            s->indices[s->index_count++]=vertex_base+u16(indices+((size_t)first+flip)*2);
            s->indices[s->index_count++]=vertex_base+u16(indices+((size_t)first+(1-flip))*2);
            s->indices[s->index_count++]=vertex_base+u16(indices+((size_t)first+2)*2);
        }
    }
    return 1;
}
static int model(Reader *r, uint32_t sector) {
    size_t source_offset=r->pos;
    const uint8_t *h=take(r,76), *p;
    UCSceneModel record={0};
    uint32_t materials, material_base=r->scene->material_count;
    if(r->models){
        record.source_offset=source_offset; record.sector_index=sector;
        record.first_vertex=r->scene->vertex_count; record.first_index=r->scene->index_count;
        record.first_batch=r->scene->batch_count; record.first_material=material_base;
    }
    NEED(h); VALID(memcmp(h,"AMF\n",4)==0);
    materials=u32(h+4); SUPPORTED(u32(h+8)==1 && u32(h+12)==1);
    p=take(r,64); NEED(p); SUPPORTED(memcmp(p,identity,64)==0);
    NEED(take(r,160)); /* Original bone metadata; static identity palette above. */
    NEED(names(r,materials));
    if(r->models)record.group_offset=r->pos;
    p=take(r,56); NEED(p); SUPPORTED(u32(p)==1);
    if(r->models){
        memcpy(record.bounds.minimum,p+4,12); memcpy(record.bounds.maximum,p+16,12);
        for(unsigned i=0;i<3;++i){
            float low=record.bounds.minimum[i],high=record.bounds.maximum[i];
            SUPPORTED(isfinite(low)&&isfinite(high));
            record.bounds.center[i]=(low+high)*.5f;
            record.bounds.extent[i]=(high-low)*.5f;
            SUPPORTED(isfinite(record.bounds.center[i])&&isfinite(record.bounds.extent[i]));
        }
    }
    NEED(node(r,materials,material_base,sector));
    p=take(r,2); NEED(p); SUPPORTED(u16(p)==0); /* No auxiliary animation resource. */
    if(r->models){
        UCSceneModels *m=r->models;
        record.source_bytes=r->pos-source_offset;
        record.vertex_count=r->scene->vertex_count-record.first_vertex;
        record.index_count=r->scene->index_count-record.first_index;
        record.batch_count=r->scene->batch_count-record.first_batch;
        record.material_count=r->scene->material_count-material_base;
        NEED(grow(r,(void **)&m->items,m->count,m->count+1,sizeof(*m->items)));
        m->items[m->count++]=record;
    }
    ++r->scene->model_count;
    return 1;
}
static int counted_skip(Reader *r, size_t stride) {
    const uint8_t *p=take(r,4); uint32_t count;
    NEED(p); count=u32(p); LIMITED(count<=RECORD_LIMIT);
    NEED(take(r,(size_t)count*stride)); return 1;
}
static int parse(Reader *r) {
    UCScene *s=r->scene;
    const uint8_t *h=take(r,464), *p;
    uint32_t i,j,count;
    NEED(h); VALID(memcmp(h,"GEO\n",4)==0 && memcmp(h+460,"GEO\n",4)==0);
    s->light_count=u32(h+32); LIMITED(s->light_count<65535);
    NEED(take(r,(size_t)s->light_count*176));
    p=take(r,8); NEED(p); s->sector_count=u32(p+4); LIMITED(s->sector_count<=RECORD_LIMIT);
    for (i=0; i<s->sector_count; ++i) {
        p=take(r,4); NEED(p); count=u32(p); LIMITED(count<=RECORD_LIMIT-s->model_count);
        for (j=0; j<count; ++j) NEED(model(r,i));
    }
    s->geometry_end=r->pos;
    /* AA040/B70C0 tail: validate complete record spans. Visibility/collision
     * behavior is deliberately not synthesized by this mesh importer. */
    p=take(r,4); NEED(p); SUPPORTED(u32(p)<=1);
    if (u32(p)) {
        p=take(r,4); NEED(p); s->room_count=u32(p); LIMITED(s->room_count<=RECORD_LIMIT);
        for (i=0; i<s->room_count; ++i) {
            NEED(take(r,24)); NEED(counted_skip(r,4)); NEED(counted_skip(r,16));
        }
        p=take(r,4); NEED(p); s->portal_count=u32(p); LIMITED(s->portal_count<=RECORD_LIMIT);
        for (i=0; i<s->portal_count; ++i) { NEED(take(r,48)); NEED(counted_skip(r,4)); }
    }
    VALID(r->pos==r->size);
    return 1;
}
void uc_scene_free(UCScene *s) {
    uint32_t i;
    if (!s) return;
    for (i=0; i<s->material_count; ++i) free(s->material_names[i]);
    free(s->vertices); free(s->indices); free(s->batches); free(s->material_names);
    memset(s,0,sizeof(*s));
}
static UCSceneStatus load(const void *data, size_t size, UCScene *out,UCSceneModels *models) {
    Reader r; unsigned i;
    if (!out) return UC_SCENE_ARGUMENT;
    memset(out,0,sizeof(*out));
    if (!data) return UC_SCENE_ARGUMENT;
    if (size>BYTE_LIMIT) return UC_SCENE_LIMIT;
    memset(&r,0,sizeof(r)); r.data=(const uint8_t *)data; r.size=size; r.scene=out; r.models=models;
    for (i=0; i<3; ++i) { out->bounds_min[i]=FLT_MAX; out->bounds_max[i]=-FLT_MAX; }
    if (!parse(&r)) { uc_scene_models_free(models); uc_scene_free(out); return r.status; }
    if (!out->vertex_count) {
        memset(out->bounds_min,0,sizeof(out->bounds_min));
        memset(out->bounds_max,0,sizeof(out->bounds_max));
    }
    return UC_SCENE_OK;
}
UCSceneStatus uc_scene_load(const void *data,size_t size,UCScene *out){
    return load(data,size,out,NULL);
}
UCSceneStatus uc_scene_load_models(const void *data,size_t size,UCScene *out,UCSceneModels *models){
    UCSceneStatus status;
    if(!models){if(out)memset(out,0,sizeof(*out));return UC_SCENE_ARGUMENT;}
    memset(models,0,sizeof(*models));
    status=load(data,size,out,models);
    if(status==UC_SCENE_OK)models->scene=out;
    return status;
}
void uc_scene_models_free(UCSceneModels *models){
    if(models){free(models->items);memset(models,0,sizeof(*models));}
}
const char *uc_scene_status(UCSceneStatus s) {
    static const char *names_[]={"ok","argument","format","unsupported layout","limit","memory"};
    return (unsigned)s<sizeof(names_)/sizeof(*names_) ? names_[s] : "unknown";
}

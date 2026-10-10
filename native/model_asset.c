#include "model_asset.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define SOURCE_LIMIT (64u*1024u*1024u)
#define ALLOCATION_LIMIT (256u*1024u*1024u)
typedef struct Reader { const uint8_t *data;size_t size,at,allocated;UCModelStatus status; } Reader;
static uint16_t u16(const uint8_t *p){return (uint16_t)(p[0]|(p[1]<<8));}
static uint32_t u32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static int16_t i16(const uint8_t *p){int16_t n;memcpy(&n,p,2);return n;}
static const uint8_t *take(Reader *r,size_t n){
    const uint8_t *p;if(n>r->size-r->at){r->status=UC_MODEL_FORMAT;return NULL;}
    p=r->data+r->at;r->at+=n;return p;
}
static void *allocate(Reader *r,size_t count,size_t stride){
    void *p;if(count>ALLOCATION_LIMIT/stride||count*stride>ALLOCATION_LIMIT-r->allocated){r->status=UC_MODEL_LIMIT;return NULL;}
    p=calloc(count?count:1,stride);if(!p){r->status=UC_MODEL_MEMORY;return NULL;}
    r->allocated+=count*stride;return p;
}
static char *name(Reader *r,int empty){
    const uint8_t *p=take(r,2);size_t n;char *result;if(!p)return NULL;n=u16(p);
    if(n>255||(!empty&&!n)){r->status=UC_MODEL_UNSUPPORTED;return NULL;}
    p=take(r,n);if(!p)return NULL;
    if(memchr(p,0,n)){r->status=UC_MODEL_UNSUPPORTED;return NULL;}
    result=(char*)allocate(r,n+1,1);if(result)memcpy(result,p,n);return result;
}
static int finite_matrix(const float m[16]){for(unsigned i=0;i<16;++i)if(!isfinite(m[i]))return 0;return 1;}
#define NEED(p) do{if(!(p))return 0;}while(0)
#define VALID(c) do{if(!(c)){r->status=UC_MODEL_FORMAT;return 0;}}while(0)
#define SUPPORTED(c) do{if(!(c)){r->status=UC_MODEL_UNSUPPORTED;return 0;}}while(0)
#define LIMITED(c) do{if(!(c)){r->status=UC_MODEL_LIMIT;return 0;}}while(0)
static int group(Reader *r,UCModel *m){
    const uint8_t *g,*node,*position,*color,*uv,*uv1,*draw,*indices,*skin,*p;
    uint32_t nv,nd,ni,extra,arrays,total=0;size_t palette_at;
    g=take(r,56);NEED(g);SUPPORTED(u32(g)==1);memcpy(m->source_group,g,56);
    node=take(r,112);NEED(node);memcpy(m->source_node,node,112);
    nv=u32(node+8);nd=u32(node+12);ni=u32(node+16);extra=u32(node+20);arrays=u32(node+24);
    SUPPORTED(u32(node+4)==0&&u32(node+28)==17&&nv>0);
    LIMITED(nv<=1000000&&nd<=100000&&ni<=8000000&&extra<=100000&&arrays<=100000);
    position=take(r,(size_t)nv*10);NEED(position);color=take(r,(size_t)nv*4);NEED(color);
    uv=take(r,(size_t)nv*8);NEED(uv);uv1=take(r,(size_t)nv*8);NEED(uv1);
    draw=take(r,(size_t)nd*20);NEED(draw);NEED(take(r,(size_t)arrays*20));
    indices=take(r,(size_t)ni*2);NEED(indices);
    m->bounds=(UCModelBound*)allocate(r,extra,sizeof(*m->bounds));NEED(m->bounds);m->bound_count=extra;
    for(uint32_t i=0;i<extra;++i){
        UCModelBound *bound=&m->bounds[i];p=take(r,56);NEED(p);
        memcpy(bound->limits,p,24);bound->bone=u16(p+48);
        for(unsigned j=0;j<6;++j)VALID(isfinite(bound->limits[j]));
        VALID(bound->bone<m->bone_count);NEED(take(r,(size_t)u16(p+50)*4));
    }
    palette_at=r->at;
    /* A65B0 reads each8-byte header; A65D5 replaces word+4 with the newly
     * allocated payload pointer before any use. Serialized word+4 is ignored. */
    for(uint32_t i=0;i<nd;++i){
        uint32_t n;p=take(r,8);NEED(p);n=u32(p);LIMITED(n<=192&&n<=100000-m->palette_update_count);
        m->palette_update_count+=n;NEED(take(r,(size_t)n*4));
    }
    skin=take(r,(size_t)nv*8);NEED(skin);p=take(r,4);NEED(p);SUPPORTED(u32(p)==0);
    for(uint32_t i=0;i<nd;++i){
        const uint8_t *d=draw+(size_t)i*20;uint32_t start=u32(d),triangles=u16(d+12),count;
        SUPPORTED(d[14]==4||d[14]==5);count=d[14]==4?triangles*3:triangles+2;
        VALID(u16(d+4)<m->material_count&&start<=ni&&count<=ni-start);
        for(uint32_t j=0;j<count;++j)VALID(u16(indices+((size_t)start+j)*2)<nv);
        LIMITED(triangles*3<=8000000-total);total+=triangles*3;
    }
    m->vertices=(UCSceneVertex*)allocate(r,nv,sizeof(*m->vertices));NEED(m->vertices);
    m->skin=(UCModelSkin*)allocate(r,nv,sizeof(*m->skin));NEED(m->skin);
    m->indices=(uint32_t*)allocate(r,total,sizeof(*m->indices));NEED(m->indices);
    m->batches=(UCModelBatch*)allocate(r,nd,sizeof(*m->batches));NEED(m->batches);
    m->palette_updates=(UCModelPaletteUpdate*)allocate(r,m->palette_update_count,sizeof(*m->palette_updates));NEED(m->palette_updates);
    m->vertex_count=nv;m->batch_count=nd;
    for(uint32_t i=0;i<nv;++i){
        UCSceneVertex *v=&m->vertices[i];UCModelSkin *s=&m->skin[i];
        for(unsigned k=0;k<3;++k){
            int16_t n=i16(position+(size_t)i*10+k*2);
            v->position[k]=n==INT16_MIN?-1.0f:n/32767.0f;
            s->palette_rows[k]=i16(skin+(size_t)i*8+k*2);
            VALID(s->palette_rows[k]>=-96&&s->palette_rows[k]<=93);
        }
        v->packed_normal=u32(position+(size_t)i*10+6);v->source_color=u32(color+(size_t)i*4);
        memcpy(v->uv,uv+(size_t)i*8,8);memcpy(v->uv1,uv1+(size_t)i*8,8);
        VALID(isfinite(v->uv[0])&&isfinite(v->uv[1])&&isfinite(v->uv1[0])&&isfinite(v->uv1[1]));
        memcpy(s->weight_bytes,skin+(size_t)i*8+6,2);
    }
    total=0;
    for(uint32_t i=0;i<nd;++i){
        const uint8_t *d=draw+(size_t)i*20;UCModelBatch *b=&m->batches[i];
        uint32_t start=u32(d),triangles=u16(d+12),count=u32(r->data+palette_at);
        b->first_index=m->index_count;b->index_count=triangles*3;b->material_index=u16(d+4);
        b->light_index=u16(d+6);b->flags[0]=d[15];b->flags[1]=d[16];
        b->first_palette_update=total;b->palette_update_count=count;palette_at+=8;
        for(uint32_t j=0;j<count;++j){
            UCModelPaletteUpdate *update=&m->palette_updates[total++];p=r->data+palette_at;palette_at+=4;
            update->bone=u16(p);memcpy(&update->row_offset,p+2,1);update->source_byte3=p[3];
            VALID(update->bone<m->bone_count&&update->row_offset>=-96&&update->row_offset<=93);
        }
        for(uint32_t j=0;j<triangles;++j){
            uint32_t first=start+(d[14]==4?j*3:j),flip=d[14]==5?(j&1):0;
            m->indices[m->index_count++]=u16(indices+((size_t)first+flip)*2);
            m->indices[m->index_count++]=u16(indices+((size_t)first+1-flip)*2);
            m->indices[m->index_count++]=u16(indices+((size_t)first+2)*2);
        }
    }
    return 1;
}
static int parse(Reader *r,UCModel *m,uint32_t selected,int prefix){
    const uint8_t *h=take(r,76),*matrices,*metadata,*p;uint32_t groups;
    NEED(h);VALID(!memcmp(h,"AMF\n",4));memcpy(m->source_header,h,76);
    m->material_count=u32(h+4);m->bone_count=u32(h+12);groups=u32(h+8);
    LIMITED(m->material_count<=10000&&m->bone_count<=4096&&groups<=4096);
    SUPPORTED(m->bone_count>0&&groups>0);
    if(selected==UINT32_MAX){SUPPORTED(groups==1);selected=0;}
    VALID(selected<groups);
    memcpy(&m->position_scale,h+68,4);VALID(isfinite(m->position_scale));
    matrices=take(r,(size_t)m->bone_count*64);NEED(matrices);
    metadata=take(r,(size_t)m->bone_count*160);NEED(metadata);
    m->bones=(UCModelBone*)allocate(r,m->bone_count,sizeof(*m->bones));NEED(m->bones);
    for(uint32_t i=0;i<m->bone_count;++i){
        UCModelBone *b=&m->bones[i];p=metadata+(size_t)i*160;
        memcpy(b->initial_local,matrices+(size_t)i*64,64);memcpy(b->bind_correction,p+96,64);
        memcpy(b->source_metadata,p,160);b->parent=u16(p+22);b->first_child=u16(p+24);b->next_sibling=u16(p+26);
        VALID(finite_matrix(b->initial_local)&&finite_matrix(b->bind_correction));
        VALID(b->parent<m->bone_count&&b->first_child<m->bone_count&&b->next_sibling<m->bone_count);
    }
    m->material_names=(char**)allocate(r,m->material_count,sizeof(char*));NEED(m->material_names);
    for(uint32_t i=0;i<m->material_count;++i){m->material_names[i]=name(r,0);NEED(m->material_names[i]);}
    for(uint32_t i=0;i<groups;++i){
        if(i==selected){NEED(group(r,m));}
        else {
            UCModel skipped={0};int ok;skipped.bone_count=m->bone_count;skipped.material_count=m->material_count;
            ok=group(r,&skipped);uc_model_free(&skipped);NEED(ok);
        }
    }
    m->animation_template=name(r,1);NEED(m->animation_template);SUPPORTED(prefix||r->at==r->size);
    return 1;
}
void uc_model_free(UCModel *m){
    if(!m)return;
    if(m->material_names)for(uint32_t i=0;i<m->material_count;++i)free(m->material_names[i]);
    free(m->vertices);free(m->skin);free(m->indices);free(m->batches);free(m->bones);
    free(m->palette_updates);free(m->material_names);free(m->animation_template);free(m->bounds);memset(m,0,sizeof(*m));
}
static UCModelStatus load(const void *data,size_t size,uint32_t selected,UCModel *out,size_t *consumed){
    Reader r={0};if(!out)return UC_MODEL_ARGUMENT;memset(out,0,sizeof(*out));
    if(!data)return UC_MODEL_ARGUMENT;if(size>SOURCE_LIMIT)return UC_MODEL_LIMIT;
    r.data=(const uint8_t*)data;r.size=size;if(!parse(&r,out,selected,consumed!=NULL)){uc_model_free(out);return r.status;}if(consumed)*consumed=r.at;return UC_MODEL_OK;
}
UCModelStatus uc_model_load(const void *data,size_t size,UCModel *out){return load(data,size,UINT32_MAX,out,NULL);}
UCModelStatus uc_model_load_group(const void *data,size_t size,uint32_t selected,UCModel *out){
    if(selected==UINT32_MAX){if(out)memset(out,0,sizeof(*out));return UC_MODEL_ARGUMENT;}
    return load(data,size,selected,out,NULL);
}
UCModelStatus uc_model_load_prefix(const void *data,size_t size,UCModel *out,size_t *consumed){
    if(!consumed){if(out)memset(out,0,sizeof(*out));return UC_MODEL_ARGUMENT;}
    return load(data,size,UINT32_MAX,out,consumed);
}
const char *uc_model_status(UCModelStatus s){
    static const char *const names[]={"ok","argument","format","unsupported layout","limit","memory"};
    return (unsigned)s<sizeof(names)/sizeof(*names)?names[s]:"unknown";
}
void uc_model_skin_position(const float position[3],const uint8_t weights[2],const float rows[3][12],float out[3]){
    float result[3],w0=weights[0]/255.0f,w1=weights[1]/255.0f,w2=1.0f-(w0+w1);
    for(unsigned r=0;r<3;++r){
        float row[4];for(unsigned c=0;c<4;++c){unsigned i=r*4+c;row[c]=(w1*rows[1][i]+w0*rows[0][i])+w2*rows[2][i];}
        result[r]=position[0]*row[0]+position[1]*row[1]+position[2]*row[2]+row[3];
    }
    memcpy(out,result,sizeof(result));
}

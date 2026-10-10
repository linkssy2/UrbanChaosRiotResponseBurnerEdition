#include "level_asset.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define LIMIT_BYTES (256u * 1024u * 1024u)
#define LIMIT_RECORDS 100000u
typedef struct Reader {
    const uint8_t *data; size_t size, pos, allocated;
    UCLevelStatus error;
} Reader;
static const uint8_t *take(Reader *r, size_t n) {
    if (r->error) return NULL;
    if (n > r->size-r->pos) { r->error=UC_LEVEL_FORMAT; return NULL; }
    r->pos+=n; return r->data+r->pos-n;
}
static uint32_t le32(const uint8_t *p) {
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static uint32_t word(Reader *r) { const uint8_t *p=take(r,4); return p?le32(p):0; }
static uint32_t count(Reader *r) {
    uint32_t n=word(r);
    if (n>LIMIT_RECORDS) r->error=UC_LEVEL_LIMIT;
    return r->error?0:n;
}
static void *array(Reader *r, uint32_t n, size_t stride) {
    void *p; size_t bytes=(size_t)n*stride;
    if (r->error || !n) return NULL;
    if (bytes>LIMIT_BYTES-r->allocated) { r->error=UC_LEVEL_LIMIT; return NULL; }
    p=calloc(n,stride);
    if (!p) r->error=UC_LEVEL_MEMORY; else r->allocated+=bytes;
    return p;
}
static UCLevelBytes blob(Reader *r) {
    UCLevelBytes b; b.size=word(r); b.data=take(r,b.size); return b;
}
static const char *text(Reader *r) {
    UCLevelBytes b=blob(r);
    if (!b.data) return NULL;
    if (!b.size) return "";
    if (b.data[b.size-1] || memchr(b.data,0,b.size-1)) { r->error=UC_LEVEL_FORMAT; return NULL; }
    return (const char *)b.data;
}
static void scripts(Reader *r,UCLevelEntity *e) {
    uint32_t n,i; size_t start;
    e->script_count=count(r); e->scripts=array(r,e->script_count,sizeof(*e->scripts));
    for (i=0;i<e->script_count && !r->error;i++) {
        e->scripts[i].name=text(r); e->scripts[i].code=blob(r);
    }
    start=r->pos; n=count(r);
    for (i=0;i<n && !r->error;i++) {
        uint32_t c=count(r); take(r,(size_t)c*8); word(r);
        c=count(r); take(r,(size_t)c*12); word(r);
    }
    n=count(r);
    for (i=0;i<n && !r->error;i++) { uint32_t c=count(r); take(r,(size_t)c*12); word(r); }
    e->script_records.data=r->data+start; e->script_records.size=(uint32_t)(r->pos-start);
}
static void entity(Reader *r, UCLevelEntity *e) {
    uint32_t i; size_t start; const uint8_t *p;
    e->source_offset=(uint32_t)r->pos;
    if (word(r)!=11) { if (!r->error) r->error=UC_LEVEL_UNSUPPORTED; return; }
    e->class_name=text(r); e->asset_name=text(r); e->field50=text(r); e->flags=word(r);
    p=take(r,64); if (p) memcpy(e->transform,p,64);
    for (i=0;i<16;i++) if (!isfinite(e->transform[i])) r->error=UC_LEVEL_FORMAT;
    e->active=word(r); e->field40=word(r);
    scripts(r,e);
    e->name=text(r); e->link_count=count(r); e->links=array(r,e->link_count,sizeof(*e->links));
    for (i=0;i<e->link_count && !r->error;i++) e->links[i]=text(r);
    start=r->pos;
    if (word(r)) {
        p=take(r,28);
        if (p) take(r,(size_t)((uint32_t)p[2]|(uint32_t)p[3]<<8)*12);
    }
    e->extra.data=r->data+start; e->extra.size=(uint32_t)(r->pos-start);
    e->properties=blob(r); p=take(r,4);
    if (p && memcmp(p,"[--]",4)) r->error=UC_LEVEL_FORMAT;
}
void uc_level_free(UCLevel *l) {
    uint32_t i; if (!l) return;
    if (l->entities) for (i=0;i<l->entity_count;i++) { free(l->entities[i].scripts); free(l->entities[i].links); }
    free(l->entities); free(l->storage); memset(l,0,sizeof(*l));
}
static void floats(Reader *r,float *out,uint32_t n) {
    const uint8_t *p=take(r,(size_t)n*4);
    if(!p)return;
    memcpy(out,p,(size_t)n*4);
    for(uint32_t i=0;i<n;i++)if(!isfinite(out[i]))r->error=UC_LEVEL_FORMAT;
}
static UCLevelIndices indices(Reader *r,int filter) {
    UCLevelIndices list={0};uint32_t n=count(r);
    list.data=array(r,n,4);
    for(uint32_t i=0;i<n&&!r->error;i++){
        uint32_t v=word(r);
        if(!filter||(int32_t)v>=0)list.data[list.count++]=v;
    }
    return list;
}
static UCLevelBytes pairs(Reader *r) {
    UCLevelBytes b={0};uint32_t n=count(r);b.size=n*8;b.data=take(r,b.size);return b;
}
static void trigger(Reader *r,UCLevelTrigger *t) {
    uint32_t once,custom,group_count;const uint8_t *p;
    t->source_offset=(uint32_t)r->pos;t->packed=word(r);
    floats(r,t->position,3);floats(r,&t->delay,1);once=word(r);
    word(r);word(r); /* Version11 editor fields are read but not retained. */
    floats(r,t->transform,16);custom=word(r);
    t->flags=(uint8_t)(16|(once?8:0)|(custom?32:0)|((t->packed&64)?1:0));
    if(custom){
        scripts(r,&t->script);
        /* The last collection starts after the conditional records. */
        Reader s={0};s.data=t->script.script_records.data;s.size=t->script.script_records.size;
        uint32_t n=count(&s);
        for(uint32_t i=0;i<n&&!s.error;i++){
            uint32_t c=count(&s);take(&s,(size_t)c*8);word(&s);
            c=count(&s);take(&s,(size_t)c*12);word(&s);
        }
        group_count=count(&s);
        if(s.error)r->error=s.error;
        if(group_count)t->script.properties=blob(r);
    }
    t->kind=t->packed&63;t->shape=(t->packed>>7)&7;
    t->number=(int16_t)(t->packed>>11);
    if(t->shape==3){
        t->shape_field=word(r);uint32_t n=count(r);t->boxes.size=n*24;t->boxes.data=take(r,t->boxes.size);
        for(uint32_t i=0;i<n*6&&!r->error;i++){
            float value;memcpy(&value,t->boxes.data+i*4,4);if(!isfinite(value))r->error=UC_LEVEL_FORMAT;
        }
    }else if(t->shape<=4)floats(r,t->dimensions,t->shape==4?3:t->shape==2?2:1);
    else r->error=UC_LEVEL_UNSUPPORTED;
    switch(t->kind){
    case 1:case 6:case 14:case 25:t->targets=indices(r,1);break;
    case 2:case 3:case 4:case 5:case 7:case 8:case 9:case 10:case 11:case 12:case 13:
    case 15:case 18:case 19:case 20:case 21:case 22:case 23:case 24:case 26:
        t->subjects=indices(r,1);t->targets=indices(r,1);break;
    case 17:t->subjects=indices(r,1);break;
    case 27:case 28:case 29:case 30:t->flag_tests=pairs(r);t->targets=indices(r,1);break;
    case 31:case 32:case 33:case 34:case 36:t->subjects=indices(r,1);t->flag_writes=pairs(r);break;
    case 35:t->flag_writes=pairs(r);break;
    default:break;
    }
    p=take(r,4);if(p&&memcmp(p,"[--]",4))r->error=UC_LEVEL_FORMAT;
}
void uc_level_logic_free(UCLevelLogic *l) {
    if(!l)return;
    if(l->triggers)for(uint32_t i=0;i<l->trigger_count;i++){
        free(l->triggers[i].script.scripts);free(l->triggers[i].subjects.data);free(l->triggers[i].targets.data);
    }
    if(l->nodes)for(uint32_t i=0;i<l->node_count;i++){free(l->nodes[i].first.data);free(l->nodes[i].second.data);}
    free(l->triggers);free(l->nodes);memset(l,0,sizeof(*l));
}
UCLevelStatus uc_level_logic_load(const UCLevel *level,UCLevelLogic *out) {
    if(!level||!out||!level->storage||!level->tail.data)return UC_LEVEL_ARGUMENT;
    if(level->version!=16)return UC_LEVEL_UNSUPPORTED;
    if(level->tail.data<level->storage||level->tail.data>level->storage+level->size||
       level->tail.size>level->size-(size_t)(level->tail.data-level->storage))return UC_LEVEL_ARGUMENT;
    UCLevelLogic l={0};Reader r={0};l.level=level;
    r.data=level->storage;r.size=level->size;r.pos=(size_t)(level->tail.data-level->storage);
    for(uint32_t i=1;i<4;i++)if(level->section_counts[i]>LIMIT_RECORDS)return UC_LEVEL_LIMIT;
    /* 2105C0's old named groups are unrelated to the trigger arrays. Retain
     * their bytes in level.tail; current gameplay never consumes them here. */
    for(uint32_t i=0;i<level->section_counts[1]&&!r.error;i++){
        word(&r);uint32_t n=count(&r);take(&r,(size_t)n*12);
    }
    l.trigger_count=level->section_counts[2];l.triggers=array(&r,l.trigger_count,sizeof(*l.triggers));
    for(uint32_t i=0;i<l.trigger_count&&!r.error;i++)trigger(&r,l.triggers+i);
    l.node_count=level->section_counts[3];l.nodes=array(&r,l.node_count,sizeof(*l.nodes));
    for(uint32_t i=0;i<l.node_count&&!r.error;i++){
        UCLevelNode *n=l.nodes+i;n->key=word(&r);floats(&r,n->position,3);n->field10=word(&r);
        n->first=indices(&r,0);n->second=indices(&r,0);
    }
    if(!r.error&&r.pos!=r.size)r.error=UC_LEVEL_FORMAT;
    if(r.error){uc_level_logic_free(&l);return r.error;}
    *out=l;return UC_LEVEL_OK;
}
UCLevelStatus uc_level_load(const void *data, size_t size, UCLevel *out) {
    UCLevel l={0}; Reader r={0}; uint32_t n,i,j;
    if (!out || !data) return UC_LEVEL_ARGUMENT;
    if (size>LIMIT_BYTES) return UC_LEVEL_LIMIT;
    if (size<16) return UC_LEVEL_FORMAT;
    l.storage=malloc(size); if (!l.storage) return UC_LEVEL_MEMORY;
    memcpy(l.storage,data,size); l.size=size; r.data=l.storage; r.size=size; r.allocated=size;
    l.version=word(&r);
    if (l.version!=16) r.error=UC_LEVEL_UNSUPPORTED;
    l.field=word(&r); n=count(&r); l.remap.size=n*8; l.remap.data=take(&r,l.remap.size);
    n=count(&r);
    for (i=0;i<n && !r.error;i++) for (j=0;j<4;j++) {
        uint32_t v=count(&r);
        if (v>LIMIT_RECORDS-l.section_counts[j]) r.error=UC_LEVEL_LIMIT;
        else l.section_counts[j]+=v;
    }
    l.entity_count=l.section_counts[0]; l.entities=array(&r,l.entity_count,sizeof(*l.entities));
    for (i=0;i<l.entity_count && !r.error;i++) entity(&r,l.entities+i);
    l.tail.data=r.data+r.pos; l.tail.size=(uint32_t)(r.size-r.pos);
    if (r.error) { uc_level_free(&l); return r.error; }
    *out=l; return UC_LEVEL_OK;
}
UCLevelStatus uc_level_entity_asset_present(const UCLevel *l,uint32_t index,uint8_t *present) {
    size_t at; uint32_t length;
    if (!l || !present || !l->storage || !l->entities || index>=l->entity_count)
        return UC_LEVEL_ARGUMENT;
    at=l->entities[index].source_offset;
    if (at>l->size || l->size-at<8 || le32(l->storage+at)!=11) return UC_LEVEL_FORMAT;
    length=le32(l->storage+at+4); at+=8;
    if (length>l->size-at) return UC_LEVEL_FORMAT;
    at+=length;
    if (l->size-at<4) return UC_LEVEL_FORMAT;
    length=le32(l->storage+at); at+=4;
    if (length>l->size-at) return UC_LEVEL_FORMAT;
    *present=(uint8_t)(length!=0); return UC_LEVEL_OK;
}
UCLevelStatus uc_level_restart(const UCLevel *l, int32_t number, UCLevelRestart *out) {
    uint32_t i,selected=UINT32_MAX; int32_t lowest=INT32_MAX;
    const UCLevelEntity *e; const uint8_t *p;
    if (!l || !out) return UC_LEVEL_ARGUMENT;
    memset(out,0,sizeof(*out));
    for (i=0;i<l->entity_count;i++) {
        int32_t key; e=l->entities+i;
        if (strcmp(e->class_name,"RestartPoint")) continue;
        if (e->properties.size!=32) return UC_LEVEL_UNSUPPORTED;
        p=e->properties.data; memcpy(&key,p,4);
        if (number!=-1 && key==number) { selected=i; break; }
        if (number==-1 && (selected==UINT32_MAX || key<lowest)) { selected=i; lowest=key; }
    }
    if (selected==UINT32_MAX) return UC_LEVEL_NOT_FOUND;
    e=l->entities+selected; p=e->properties.data; out->entity_index=selected;
    memcpy(&out->number,p,4);
    for (i=0;i<2;i++) {
        out->on_restart[i]=le32(p+4+i*4); out->on_operated[i]=le32(p+12+i*4);
        out->on_post_restart[i]=le32(p+20+i*4);
    }
    out->auto_fade=le32(p+28); memcpy(out->transform,e->transform,64);
    out->yaw=(float)-atan2((double)e->transform[8],(double)e->transform[10]);
    return UC_LEVEL_OK;
}
UCLevelStatus uc_level_node_find(const UCLevelLogic *logic,uint32_t id,const UCLevelNode **out){
    if(!logic||!out||(logic->node_count&&!logic->nodes))return UC_LEVEL_ARGUMENT;
    if(logic->node_count>LIMIT_RECORDS)return UC_LEVEL_LIMIT;
    uint32_t key=id&0xfff0ffffu;
    for(uint32_t i=0;i<logic->node_count;i++)if(logic->nodes[i].key==key){*out=logic->nodes+i;return UC_LEVEL_OK;}
    *out=NULL;return UC_LEVEL_OK;
}
const char *uc_level_status(UCLevelStatus s) {
    static const char *const names[]={"ok","argument","format","unsupported","limit","memory","not found"};
    return (unsigned)s<sizeof(names)/sizeof(*names)?names[s]:"unknown";
}

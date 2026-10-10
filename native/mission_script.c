#include "mission_script.h"
#include <math.h>
#include <string.h>

static uint32_t u32(const uint8_t *p) {
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static int span(UCLevelBytes b, size_t *at, size_t n, const uint8_t **p) {
    if (*at>b.size || n>b.size-*at) return 0;
    *p=b.data+*at; *at+=n; return 1;
}
static int records(UCLevelBytes b, size_t *at, uint32_t count, uint32_t stride, const uint8_t **p) {
    if (*at>b.size || count>(b.size-*at)/stride) return 0;
    return span(b,at,(size_t)count*stride,p);
}
UCMissionStatus uc_mission_group(const UCLevelEntity *e,uint32_t group,UCMissionGroup *out) {
    UCLevelBytes b; const uint8_t *p,*rows=NULL; size_t at=0; uint32_t i,n,c=0;
    if (!e || !out) return UC_MISSION_ARGUMENT;
    b=e->script_records;
    if (!b.data || !span(b,&at,4,&p)) return UC_MISSION_FORMAT;
    n=u32(p);
    for (i=0;i<n;i++) {
        if (!span(b,&at,4,&p) || !records(b,&at,u32(p),8,&p) || !span(b,&at,4,&p) ||
            !span(b,&at,4,&p) || !records(b,&at,u32(p),12,&p) || !span(b,&at,4,&p)) return UC_MISSION_FORMAT;
    }
    if (!span(b,&at,4,&p)) return UC_MISSION_FORMAT;
    n=u32(p); if (group>=n) return UC_MISSION_NOT_FOUND;
    for (i=0;i<=group;i++) {
        if (!span(b,&at,4,&p)) return UC_MISSION_FORMAT;
        c=u32(p);
        if (!records(b,&at,c,12,&rows) || !span(b,&at,4,&p)) return UC_MISSION_FORMAT;
    }
    if (!c) return UC_MISSION_NOT_FOUND;
    out->rows=rows; out->count=c; memcpy(&out->priority,p,4); return UC_MISSION_READY;
}
UCMissionStatus uc_mission_select_group(const UCLevelEntity *e, uint32_t group,
    uint32_t random_percent, uint32_t *script, int32_t *priority) {
    UCMissionGroup view;uint32_t i,selected=0;
    if (!e || !script || !priority || random_percent>99) return UC_MISSION_ARGUMENT;
    *script=0; *priority=0;
    UCMissionStatus status=uc_mission_group(e,group,&view);
    if(status!=UC_MISSION_READY)return status;
    const uint8_t *rows=view.rows;uint32_t c=view.count;
    if (c>1) for (i=0;i<c;i++) {
        int32_t low,high; memcpy(&low,rows+i*12+4,4); memcpy(&high,rows+i*12+8,4);
        if (low<(int32_t)random_percent && high>(int32_t)random_percent) { selected=i; break; }
    }
    selected=u32(rows+selected*12);
    if (selected>=e->script_count) return UC_MISSION_FORMAT;
    *script=selected; *priority=view.priority; return UC_MISSION_READY;
}
UCMissionStatus uc_mission_script_begin(const UCLevel *l, uint32_t entity,
    uint32_t script, UCMissionScript *out) {
    UCLevelBytes b;
    if (!out || !l || entity>=l->entity_count) return UC_MISSION_ARGUMENT;
    memset(out,0,sizeof(*out));
    if (script>=l->entities[entity].script_count) return UC_MISSION_NOT_FOUND;
    b=l->entities[entity].scripts[script].code;
    if (!b.data || b.size<4 || b.size%4) return UC_MISSION_FORMAT;
    out->level=l; out->entity=entity; out->script=script; return UC_MISSION_READY;
}
UCMissionStatus uc_mission_script_resume(UCMissionScript *s, float dt,
    uint8_t *flags, size_t flag_count, UCMissionEnsureEntity ensure, void *context) {
    const UCLevelEntity *e; UCLevelBytes b; uint32_t op,arg; float wait;
    if (!s || !s->level || !isfinite(dt) || dt<0 || s->entity>=s->level->entity_count)
        return UC_MISSION_ARGUMENT;
    if (s->done) return UC_MISSION_DONE;
    if (s->waiting) {
        if (s->wait_remaining>0) { s->wait_remaining-=dt; return UC_MISSION_YIELD; }
        s->waiting=0; return UC_MISSION_YIELD; /*217C1A then2176E0*/
    }
    e=s->level->entities+s->entity;
    if (s->script>=e->script_count) return UC_MISSION_FORMAT;
    b=e->scripts[s->script].code;
    if (s->offset>b.size || b.size-s->offset<4) return UC_MISSION_FORMAT;
    op=u32(b.data+s->offset); s->opcode=op; s->argument=0;
    if (op==UINT32_MAX) { s->done=1; return UC_MISSION_DONE; }
    if (op!=5 && op!=6 && op!=10 && op!=11 && op!=21 && op!=22) return UC_MISSION_UNSUPPORTED;
    if (b.size-s->offset<8) return UC_MISSION_FORMAT;
    arg=u32(b.data+s->offset+4); s->argument=arg;
    if (op==5) {
        if (arg>=e->script_count) return UC_MISSION_FORMAT;
        b=e->scripts[arg].code;
        if (!b.data || b.size<4 || b.size%4) return UC_MISSION_FORMAT;
        s->script=arg; s->offset=0; return UC_MISSION_YIELD;
    }
    if (op==6 || op==21) {
        memcpy(&wait,&arg,4); if (!isfinite(wait)) return UC_MISSION_FORMAT;
        if (wait>0) { s->waiting=1; s->wait_remaining=wait-dt; }
    } else if (op==10 || op==11) {
        if (arg!=UINT32_MAX) {
            if (arg>=s->level->field || arg>=flag_count || !flags) return UC_MISSION_FORMAT;
            flags[arg]=(uint8_t)(op==10);
        }
    } else if (arg!=UINT32_MAX) {
        int result;
        if (arg>=s->level->entity_count) return UC_MISSION_FORMAT;
        if (!ensure) return UC_MISSION_NEEDS_ENTITY;
        result=ensure(context,arg);
        if (result==0) return UC_MISSION_NEEDS_ENTITY;
        if (result!=1) return UC_MISSION_BINDING_FAILED;
    }
    s->offset+=8; return UC_MISSION_YIELD;
}
const char *uc_mission_status(UCMissionStatus s) {
    static const char *const names[]={"ready","yield","done","needs native entity",
        "unsupported opcode","format","argument","not found","native binding failed"};
    return (unsigned)s<sizeof(names)/sizeof(*names)?names[s]:"unknown";
}

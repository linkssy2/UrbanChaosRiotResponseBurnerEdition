#ifndef NV2A_PREPARATION_PROBE_H
#define NV2A_PREPARATION_PROBE_H
/* Diagnostic only. No pointers/resources retained and no execution is skipped.
 * Read snapshots as recurrence evidence, NEVER as replayable draw packets.
 * Included after g_pg. F10 arms one bounded owner-thread capture. */
#define NV_PREP_RECORDS 4096u
typedef struct NvPreparationRecord {
    uint64_t qpc;
    uint32_t frame, reason, mode, indexed, inline_array, query_active;
    uint32_t count, stride, program_start, execution, shader;
    uint32_t methods[2048], program[136][4], constants[192][4];
    uint32_t formats[16], offsets[16], dma[2], textures[4][9];
} NvPreparationRecord;
static void nv_preparation_probe(void) {
    static int enabled=-1,started,finished,key_down;
    static unsigned count;
    static NvPreparationRecord *records;
    if(enabled<0){const char*s=getenv("UCRR_PREPARATION_PROBE");enabled=s&&!strcmp(s,"1");}
    if(!enabled||finished)return;
    int down=(GetAsyncKeyState(VK_F10)&0x8000)!=0;
    if(!started&&down&&!key_down){
        records=(NvPreparationRecord*)calloc(NV_PREP_RECORDS,sizeof(*records));
        if(!records){finished=1;return;}
        started=1;
    }
    key_down=down;
    if(!started)return;
    NvPreparationRecord *r=&records[count];
    LARGE_INTEGER now;QueryPerformanceCounter(&now);r->qpc=(uint64_t)now.QuadPart;
    r->frame=g_pg.stats.frames;r->reason=work_probe.reason;r->mode=g_pg.draw_mode;
    r->indexed=g_pg.indexed_draw;r->inline_array=g_pg.inline_array_draw;
    r->query_active=g_pg.reports.active!=NULL;r->count=g_pg.inline_count;r->stride=g_pg.vert_stride;
    r->program_start=g_pg.transform_program_start;r->execution=g_pg.transform_execution_mode;
    r->shader=g_pg.shader_stage_program;
    memcpy(r->methods,g_pg.trace_methods,sizeof(r->methods));
    memcpy(r->program,g_pg.vertex_program,sizeof(r->program));
    memcpy(r->constants,g_pg.vertex_constants,sizeof(r->constants));
    memcpy(r->formats,g_pg.array_format,sizeof(r->formats));
    memcpy(r->offsets,g_pg.array_offset,sizeof(r->offsets));
    memcpy(r->dma,g_pg.vertex_dma,sizeof(r->dma));
    for(unsigned t=0;t<4;++t){
        r->textures[t][0]=g_pg.tex[t].offset;r->textures[t][1]=g_pg.tex[t].format;
        r->textures[t][2]=g_pg.tex[t].address;r->textures[t][3]=g_pg.tex[t].control0;
        r->textures[t][4]=g_pg.tex[t].control1;r->textures[t][5]=g_pg.tex[t].filter;
        r->textures[t][6]=g_pg.tex[t].host_levels;r->textures[t][7]=g_pg.tex[t].image_rect;
        r->textures[t][8]=g_pg.tex[t].enabled;
    }
    if(++count==NV_PREP_RECORDS){
        char path[96];snprintf(path,sizeof(path),"preparation-probe-%lu.bin",GetCurrentProcessId());
        FILE*f=fopen(path,"wb");
        if(f){uint32_t header[4]={0x50525055u,1u,sizeof(*records),count};
            size_t h=fwrite(header,sizeof(header),1,f),n=fwrite(records,sizeof(*records),count,f);
            int result=fclose(f);
            if(h!=1||n!=count||result)fprintf(stderr,"[PREPARATION-PROBE] incomplete write\n");
        }
        free(records);records=NULL;finished=1;
    }
}
#endif

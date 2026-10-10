#ifndef NV2A_INLINE_COMPACT_H
#define NV2A_INLINE_COMPACT_H
/* Exact quad codec: four 13-word records may differ only at float word5.
 * Retains that word for each corner; no inferred angle, texture or shader.
 * Input stays owned by the decoder and intact for the original fallback. */
static int nv_inline_compact_encode(const uint32_t *data,unsigned words,
    const uint32_t formats[16],const float defaults[16][4],
    uint32_t *records,unsigned capacity,NvVertexSnapshotLayout *layout) {
    unsigned offsets[16];
    if(!data||!records||!layout||!words||words%52||words>39*52||
       nv2a_inline_layout(formats,offsets)!=52||capacity<words/52*17)return 0;
    int corner_float=0;
    for(unsigned a=0;a<16;++a)if(offsets[a]!=UINT32_MAX){
        unsigned bytes=nv2a_vertex_attribute_bytes(formats[a]);
        if(offsets[a]&3u || bytes&3u)return 0;
        if(offsets[a]<=20&&offsets[a]+bytes>20){
            if((formats[a]&15)!=2)return 0;
            corner_float=1;
        }
    }
    if(!corner_float)return 0;
    /* Full admission before publishing any output. */
    for(unsigned q=0;q<words/52;++q)for(unsigned c=1;c<4;++c)
        for(unsigned w=0;w<13;++w)if(w!=5&&data[q*52+w]!=data[q*52+c*13+w])return 0;
    memset(layout,0,sizeof(*layout));layout->stride=68;
    memcpy(layout->defaults,defaults,sizeof(layout->defaults));
    for(unsigned a=0;a<16;++a)if(offsets[a]!=UINT32_MAX){
        layout->attribute[a][0]=offsets[a];layout->attribute[a][1]=formats[a];
    }
    for(unsigned q=0;q<words/52;++q){
        memcpy(records+q*17,data+q*52,52);
        for(unsigned c=0;c<4;++c)records[q*17+13+c]=data[q*52+c*13+5];
    }
    return 1;
}
#endif

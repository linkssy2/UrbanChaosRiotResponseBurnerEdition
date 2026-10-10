#include "texture_sequence.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static uint32_t little32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}
static size_t little16(const uint8_t *p) { return (size_t)p[0] | (size_t)p[1]<<8; }

UCTextureSequenceStatus uc_texture_sequence_decode(const void *data, size_t size,
    UCTextureSequence *out) {
    const uint8_t *p=(const uint8_t*)data;
    size_t at=4, names_size=0;
    uint32_t count;
    char **names;
    char *text;
    if (!out) return UC_SEQUENCE_ARGUMENT;
    memset(out,0,sizeof(*out));
    if (!data) return UC_SEQUENCE_ARGUMENT;
    if (size<4) return UC_SEQUENCE_FORMAT;
    count=little32(p);
    if (!count) return UC_SEQUENCE_FORMAT;
    if (count>4096) return UC_SEQUENCE_LIMIT;
    for (uint32_t i=0;i<count;++i) {
        size_t n;
        if (size-at<2) return UC_SEQUENCE_FORMAT;
        n=little16(p+at); at+=2;
        if (!n) return UC_SEQUENCE_FORMAT;
        if (n>=1024) return UC_SEQUENCE_LIMIT;
        if (n>size-at || memchr(p+at,0,n)) return UC_SEQUENCE_FORMAT;
        at+=n; names_size+=n+1;
    }
    if (at!=size) return UC_SEQUENCE_FORMAT;
    names=(char**)malloc((size_t)count*sizeof(*names)+names_size);
    if (!names) return UC_SEQUENCE_MEMORY;
    text=(char*)(names+count); at=4;
    for (uint32_t i=0;i<count;++i) {
        size_t n=little16(p+at); at+=2;
        names[i]=text; memcpy(text,p+at,n); text[n]=0;
        text+=n+1; at+=n;
    }
    out->count=count; out->names=(const char *const*)names; out->storage=names;
    return UC_SEQUENCE_OK;
}

void uc_texture_sequence_free(UCTextureSequence *sequence) {
    if (sequence) { free(sequence->storage); memset(sequence,0,sizeof(*sequence)); }
}

UCTextureSequenceStatus uc_texture_sequence_select(const UCTextureSequence *sequence,
    const UCTextureSequenceState *state, float renderer_phase, UCTextureSelection *out) {
    uint32_t index;
    if (!sequence || !sequence->storage || !sequence->names || !sequence->count ||
        sequence->count>4096 || !out) return UC_SEQUENCE_ARGUMENT;
    if (!isfinite(renderer_phase) || renderer_phase<0 || renderer_phase>16777216.0f ||
        (state && !isfinite(state->scalar))) return UC_SEQUENCE_DOMAIN;
    if (state && state->scalar==0.0f) {
        if (state->index<0 || (uint32_t)state->index>=sequence->count) return UC_SEQUENCE_DOMAIN;
        index=(uint32_t)state->index;
    } else {
        float phase=fmodf(renderer_phase,1.0f);
        index=(uint32_t)((double)sequence->count*(double)phase)%sequence->count;
    }
    out->index=index; out->name=sequence->names[index];
    return UC_SEQUENCE_OK;
}
UCTextureSequenceStatus uc_texture_sequence_skin_state(int32_t selection,uint32_t count,UCTextureSequenceState*out){
    if(!out||!count||count>4096)return UC_SEQUENCE_ARGUMENT;
    int32_t index=selection%(int32_t)count;if(index<0)return UC_SEQUENCE_DOMAIN;
    *out=(UCTextureSequenceState){index,0};return UC_SEQUENCE_OK;
}

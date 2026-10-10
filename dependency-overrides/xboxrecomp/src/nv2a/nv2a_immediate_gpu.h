#ifndef NV2A_IMMEDIATE_GPU_H
#define NV2A_IMMEDIATE_GPU_H
/* Owned immediate inputs. Experimental GPU admission retains the first CPU
 * position check; later exceptional shader outputs are not fidelity-certified.
 * No simulation, lifetime, particle count, material, or texture is changed. */
typedef struct NvImmediateGpu {
    unsigned count, disabled, start, execution;
    uint32_t clip, program[136][4];
    float constants[192][4], attrs[MAX_INLINE_VERTS][16][4];
} NvImmediateGpu;
/* Only methods whose production decoder updates vertex inputs. State changes
 * and software callbacks still force original CPU materialization. */
static int nv_immediate_attribute_method(uint32_t m) {
    return (m>=NV097_SET_VERTEX_DATA2F_M&&m<NV097_SET_VERTEX_DATA2F_M+128)||
        (m>=NV097_SET_VERTEX_DATA4F_M&&m<NV097_SET_VERTEX_DATA4F_M+256)||
        (m>=NV097_SET_VERTEX_DATA4UB&&m<NV097_SET_VERTEX_DATA4UB+64)||
        (m>=NV097_SET_VERTEX3F&&m<NV097_SET_VERTEX3F+12)||
        (m>=NV097_SET_VERTEX4F&&m<NV097_SET_VERTEX4F+16);
}
static int nv_immediate_same(const NvImmediateGpu *s, const uint32_t p[136][4],
    unsigned start, const float c[192][4], unsigned execution, uint32_t clip) {
    return s->start==start && s->execution==execution && s->clip==clip &&
        !memcmp(s->program,p,sizeof(s->program)) &&
        !memcmp(s->constants,c,sizeof(s->constants));
}
static void nv_immediate_begin(NvImmediateGpu *s) {s->count=0;s->disabled=0;}
static void nv_immediate_snapshot(NvImmediateGpu *s,const uint32_t p[136][4],
    unsigned start,const float c[192][4],unsigned execution,uint32_t clip) {
    s->start=start;s->execution=execution;s->clip=clip;
    memcpy(s->program,p,sizeof(s->program));memcpy(s->constants,c,sizeof(s->constants));
}
static int nv_immediate_expand(const NvImmediateGpu *s,float (*dst)[16][4],unsigned capacity) {
    if(!s->count || s->count%4 || s->count>MAX_INLINE_VERTS || s->count/4>capacity/6)return 0;
    static const unsigned corner[6]={0,1,2,0,2,3};
    for(unsigned q=0;q<s->count/4;++q)for(unsigned k=0;k<6;++k)
        memcpy(dst[q*6+k],s->attrs[q*4+corner[k]],sizeof(s->attrs[0]));
    return 1;
}
#endif

#ifndef UC_HYBRID_PARTICLES_PGRAPH_CONTRACT_H
#define UC_HYBRID_PARTICLES_PGRAPH_CONTRACT_H
#include "hybrid_lighting_projection.h"
#include "nv2a_vertex_fetch.h"
/* Qualified replacement of the executed 4EB15C program and ordinary 4EB270
 * combiner. Object handles, unused upload bits/stages and dynamic constants
 * are not identities. Owner separately checks execution mode/resources/state. */
static int uc_hybrid_particle_layout(const uint32_t formats[16]){
    if(!formats)return 0;
    for(unsigned i=0;i<16;i++){
        unsigned low=formats[i]&255u;
        if(i<5){if(low!=(i==1?0x40u:0x32u))return 0;}
        else if(low&0xF0u)return 0;
    }
    return 1;
}
static int uc_hybrid_particle_program(const uint32_t program[136][4],unsigned start,
    const float constants[192][4],const uint32_t formats[16],const uint32_t methods[2048]){
    static const uint32_t code[17][3]={
        {0x01A00455u,0x28001000u,0x20000000u},{0x02000200u,0x0800106Cu,0x6090F81Cu},
        {0x004E041Au,0x0C005000u,0x2EA00002u},{0x02AE861Au,0xA434706Du,0x38B0F84Eu},
        {0x00A0081Au,0xA4349000u,0x24B00000u},{0x004F201Bu,0x9C016800u,0x2F000000u},
        {0x008F001Bu,0x8CAB686Cu,0x1F000000u},{0x0060001Bu,0x0400106Cu,0x2F000000u},
        {0x0000001Bu,0x0836106Cu,0x20B00000u},{0x00ED001Bu,0x04371800u,0x28200000u},
        {0x00ED201Bu,0x04373800u,0x24200000u},{0x00ED401Bu,0x04375800u,0x22200000u},
        {0x00ED601Bu,0x04377800u,0x20B01800u},{0x02000000u,0x0800106Cu,0x90B0E804u},
        {0x062000FFu,0xC40013FFu,0x10988828u},{0x0047401Au,0xC4355800u,0x20A0E800u},
        {0x0087601Au,0xC400286Au,0xF0B0E801u}};
    static const float fixed[8][4]={
        {-1,-1,0,0},{1,-1,0,0},{1,1,0,0},{-1,1,0,0},
        {0,0,0,1},{1,0,0,1},{1,1,0,1},{0,1,0,1}};
    if(!program||!constants||!methods||start>119||!uc_hybrid_particle_layout(formats))return 0;
    /* These masks are exactly the fields read by the current NV2A evaluator:
     * DWORD0 and reserved bits in DWORD1/3 do not participate in execution. */
    for(unsigned i=0;i<17;i++){
        const uint32_t *w=program[start+i];
        if(((w[1]^code[i][0])&0x0FFFFFFFu)||w[2]!=code[i][1]||
           ((w[3]^code[i][2])&0xFFFFF87Fu))return 0;
    }
    if(memcmp(constants[112],fixed,sizeof fixed))return 0;
    /* Only the two active stages participate. Neither reads C0/C1 or uses mux;
     * final E/F are zero and no final input reads V1+R0, so constant-sharing,
     * mux-selection and final-sum flags cannot affect this program. */
    if((methods[0x1E60/4]&255u)!=2||(methods[0x1E70/4]&0xFFFFFu)!=1||
       methods[0x0288/4]!=0x130C0300u||(methods[0x028C/4]&0xFFFFFF00u)!=0x00001C00u)return 0;
    const uint32_t alpha[2]={0xD4D81010u,0},rgb[2]={0xC8200000u,0xCCC40000u};
    for(unsigned i=0;i<2;i++)
        if(methods[0x0260/4+i]!=alpha[i]||methods[0x0AC0/4+i]!=rgb[i]||
           (methods[0x0AA0/4+i]&0xFFFFFu)!=(i?0u:0xC0u)||
           (methods[0x1E40/4+i]&0xFFFFFu)!=0xC0u)return 0;
    return 1;
}
/* c104..107 are dot-product rows; the shared projection helper takes a
 * row-vector matrix. Transpose in/out, keeping original clip W (also fog). */
static int uc_hybrid_particle_projection(float out[4][4],const float constants[192][4],
    float width,float height,float depth){
    if(!out||!constants)return 0;
    float guest[16],projected[16];
    for(unsigned r=0;r<4;r++)for(unsigned c=0;c<4;c++)guest[r*4+c]=constants[104+c][r];
    if(!uc_hybrid_compose_projection(projected,guest,constants[58],constants[59],width,height,depth))return 0;
    for(unsigned r=0;r<4;r++)for(unsigned c=0;c<4;c++)out[r][c]=projected[c*4+r];
    return 1;
}
/* Last original corner3 record, supplied by the source-exact chunk producer.
 * The PGRAPH owner publishes only after consuming the chunk. No inactive
 * attribute defaults are replaced and a rejected layout leaves out untouched. */
static int uc_hybrid_particle_latch(float out[16][4],const float prior[16][4],
    const uint32_t formats[16],const uint32_t last_raw[13]){
    if(!out||!prior||!last_raw||!uc_hybrid_particle_layout(formats))return 0;
    NvInlinePlan plan;float next[16][4];
    if(!nv2a_inline_prepare(&plan,formats)||plan.stride!=52)return 0;
    memcpy(next,prior,sizeof next);
    if(!nv2a_inline_fetch_prepared(next,&plan,(const uint8_t*)last_raw,52))return 0;
    memcpy(out,next,sizeof next);return 1;
}
#endif

#include "control_model_receiver.h"
#include "point_light_vertex.h"
#include <string.h>

static uint32_t word(const unsigned char *p){uint32_t n;memcpy(&n,p,4);return n;}
static int16_t signed_short(const unsigned char *p){int16_t n;memcpy(&n,p,2);return n;}
typedef struct Indices {uint32_t prior[2],primitive,*out;size_t input,output;unsigned expand;} Indices;
static void index_value(Indices *s,uint32_t n){
    if(s->primitive==5||!s->expand){if(s->out)s->out[s->output]=n;s->output++;}
    else if(s->input>=2){
        if(s->out){unsigned flip=(unsigned)((s->input-2)&1);s->out[s->output]=s->prior[flip];
            s->out[s->output+1]=s->prior[1-flip];s->out[s->output+2]=n;}
        s->output+=3;
    }
    s->prior[0]=s->prior[1];s->prior[1]=n;s->input++;
}
static int decode(const unsigned char *p,size_t size,uint32_t vertices,uint32_t *out,size_t *count,unsigned expand){
    if(size<24||size>16u*1024u*1024u||(size&3)||word(p)!=0x417fcu)return 0;
    Indices s={{0,0},word(p+4),out,0,0,expand};if(s.primitive!=5&&s.primitive!=6)return 0;
    size_t at=8;int odd=0,ended=0,arrays=0;
    while(at<size-4){
        uint32_t h=word(p+at);at+=4;
        if((h&0xe003ffffu)==0x40001800u&&!odd){
            uint32_t words=(h>>18)&2047u;if(words>(size-4-at)/4)return 0;
            for(uint32_t i=0;i<words;i++){
                uint32_t v=word(p+at);at+=4;if((v&65535u)>=vertices||(v>>16)>=vertices)return 0;
                index_value(&s,v&65535u);index_value(&s,v>>16);
            }
            arrays++;
        }else if(h==0x41808u&&!odd&&arrays){
            if(size-4-at<4)return 0;uint32_t v=word(p+at);at+=4;
            if(v>65535u||v>=vertices)return 0;index_value(&s,v);odd=1;
        }else if(h==0x417fcu&&arrays){
            if(size-4-at<4||word(p+at))return 0;at+=4;ended=1;break;
        }else return 0;
    }
    if(!ended||s.input<3||(s.primitive==5&&s.input%3))return 0;
    size_t padding=s.input<316?158-(s.input+1)/2:0;
    if(padding){
        if(at>=size-4)return 0;
        uint32_t h=word(p+at),n=(h>>18)&2047u;at+=4;
        if((h&0xe003ffffu)!=0x40000100u||n!=padding||n!=(size-4-at)/4)return 0;
        for(unsigned i=0;i<n;i++,at+=4)if(word(p+at))return 0;
    }
    if(at!=size-4)return 0;*count=s.output;return 1;
}
int uc_control_model_indices(const void *pb,size_t bytes,uint32_t vertices,uint32_t *out,size_t capacity,size_t *count){
    if(!pb||!vertices||vertices>65536||!count)return 0;
    size_t needed;if(!decode((const unsigned char*)pb,bytes,vertices,NULL,&needed,1)||
        (out&&capacity<needed))return 0;
    if(out&&!decode((const unsigned char*)pb,bytes,vertices,out,&needed,1))return 0;
    *count=needed;return 1;
}
int uc_control_model_indexed(const void *pb,size_t bytes,uint32_t vertices,uint32_t *out,size_t capacity,size_t *count,uint32_t *primitive){
    if(!pb||!vertices||vertices>65536||!count||!primitive)return 0;
    size_t needed;if(!decode((const unsigned char*)pb,bytes,vertices,NULL,&needed,0)||
        (out&&capacity<needed))return 0;
    if(out&&!decode((const unsigned char*)pb,bytes,vertices,out,&needed,0))return 0;
    *count=needed;*primitive=word((const unsigned char*)pb+4);return 1;
}
int uc_control_model_vertex(const void *position10,const void *skin8,const UCModelRenderPalette *palette,UCControlModelVertex *out){
    if(!position10||!skin8||!palette||!out)return 0;
    const unsigned char *p=(const unsigned char*)position10,*skin=(const unsigned char*)skin8;
    UCPointLightVertexInput v={0};float rows[3][12];
    for(unsigned i=0;i<3;i++){
        int16_t n=signed_short(p+2*i),offset=signed_short(skin+2*i);
        if(offset< -96||offset>93)return 0;unsigned row=(unsigned)(offset+96);
        if(!palette->valid[row]||!palette->valid[row+1]||!palette->valid[row+2])return 0;
        v.position[i]=n==INT16_MIN?-1.f:(float)n/32767.f;memcpy(rows[i],palette->rows[row],48);
    }
    v.packed_normal=word(p+6);memcpy(v.weights,skin+6,2);v.skin_rows=(const float(*)[12])rows;
    UCPointLightVertexConstants c={0};for(unsigned i=0;i<4;i++)c.clip_rows[i][i]=1;
    UCPointLightVertexOutput value;if(!uc_point_light_vertex(&v,&c,&value))return 0;
    UCControlModelVertex result;memcpy(result.position,value.clip,12);memcpy(result.normal,value.normal,12);*out=result;return 1;
}

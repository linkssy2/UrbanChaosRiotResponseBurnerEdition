#include "collision_mopp.h"
#include <fenv.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <xmmintrin.h>

enum { MAX_CODE=64*1024*1024, MAX_DEPTH=256, MAX_STEPS=4*1024*1024, MAX_SCALE_SHIFT=32 };
typedef struct Instruction {
    uint32_t length,left,right;
    uint8_t opcode,terminal,split;
} Instruction;
static uint32_t be(const uint8_t *p,uint32_t n) {
    uint32_t value=0;for(uint32_t i=0;i<n;i++)value=(value<<8)|p[i];return value;
}
/* Exact32CF14/32CE94 dispatch families. Every nonterminal edge is forward. */
static UCCollisionStatus decode(const UCCollisionMopp *m,uint32_t at,Instruction *out) {
    if(at>=m->size)return UC_COLLISION_FORMAT;
    Instruction d={0};uint32_t op=m->bytes[at];d.opcode=(uint8_t)op;
    if(op==0||(op>=0x30&&op<=0x4f)){d.length=1;d.terminal=1;}
    else if(op>=0x50&&op<=0x53){d.length=op-0x50+2;d.terminal=1;}
    else if(op>=1&&op<=4)d.length=4;
    else if(op>=5&&op<=7)d.length=op-5+2;
    else if(op==9)d.length=2;
    else if(op==10)d.length=3;
    else if(op==11)d.length=5;
    else if(op>=0x10&&op<=0x1c){d.length=4;d.split=1;}
    else if(op>=0x20&&op<=0x22){d.length=3;d.split=1;}
    else if(op>=0x23&&op<=0x25){d.length=7;d.split=1;}
    else if(op>=0x26&&op<=0x28)d.length=3;
    else if(op>=0x29&&op<=0x2b)d.length=7;
    else if(op>=0x60&&op<=0x63)d.length=2;
    else if(op>=0x64&&op<=0x67)d.length=3;
    else if(op>=0x68&&op<=0x6b)d.length=5;
    else return UC_COLLISION_UNSUPPORTED;
    if(d.length>m->size-at)return UC_COLLISION_FORMAT;
    uint32_t base=at+d.length;d.left=d.right=base;
    if(op>=5&&op<=7)d.left+=be(m->bytes+at+1,d.length-1);
    if(d.split){
        if(op>=0x23&&op<=0x25){d.left+=be(m->bytes+at+3,2);d.right+=be(m->bytes+at+5,2);}
        else d.right+=m->bytes[base-1];
    }
    if(!d.terminal&&(d.left>=m->size||(d.split&&d.right>=m->size)))return UC_COLLISION_FORMAT;
    *out=d;return UC_COLLISION_OK;
}
static UCCollisionStatus validate(const UCCollisionMopp *m,uint32_t at,uint8_t *marks,uint32_t depth,uint32_t *count) {
    if(depth>MAX_DEPTH)return UC_COLLISION_LIMIT;
    for(;;){
        if(marks[at]==1)return UC_COLLISION_OK;
        if(marks[at])return UC_COLLISION_FORMAT;
        Instruction d;UCCollisionStatus status=decode(m,at,&d);if(status)return status;
        for(uint32_t i=0;i<d.length;i++)if(marks[at+i])return UC_COLLISION_FORMAT;
        if(++*count>MAX_STEPS)return UC_COLLISION_LIMIT;
        marks[at]=1;for(uint32_t i=1;i<d.length;i++)marks[at+i]=2;
        if(d.terminal)return UC_COLLISION_OK;
        if(d.split){status=validate(m,d.right,marks,depth+1,count);if(status)return status;}
        at=d.left;
    }
}
typedef struct SafetyNode {uint32_t offset,work;uint16_t depth;uint8_t shift;} SafetyNode;
static const SafetyNode *safety_child(const SafetyNode *nodes,uint32_t count,uint32_t offset) {
    uint32_t low=0,high=count;
    while(low<high){uint32_t mid=low+(high-low)/2;if(nodes[mid].offset<offset)low=mid+1;else high=mid;}
    return low<count&&nodes[low].offset==offset?nodes+low:NULL;
}
/* Reachable instructions form a forward DAG. The original validator's visited
 * mark alone cannot bound a longer path into a shared child, left recursion,
 * or repeated visits to shared subgraphs. Compute costs over BOTH split edges;
 * even two equal child offsets contribute twice to expanded work. */
static UCCollisionStatus certify(const UCCollisionMopp *m,const uint8_t *marks,uint32_t count) {
    SafetyNode *nodes=(SafetyNode*)malloc((size_t)count*sizeof(*nodes));
    if(!nodes)return UC_COLLISION_MEMORY;
    uint32_t used=0;for(uint32_t at=0;at<m->size;at++)if(marks[at]==1)nodes[used++].offset=at;
    UCCollisionStatus status=UC_COLLISION_OK;
    if(used!=count){status=UC_COLLISION_FORMAT;goto done;}
    for(uint32_t i=count;i--;){
        Instruction d;status=decode(m,nodes[i].offset,&d);if(status)goto done;
        uint32_t work=1,depth=0,shift=d.opcode>=1&&d.opcode<=4?d.opcode:0;
        if(!d.terminal){
            const SafetyNode *left=safety_child(nodes,count,d.left);
            const SafetyNode *right=d.split?safety_child(nodes,count,d.right):left;
            if(!left||!right||left<=nodes+i||right<=nodes+i){status=UC_COLLISION_FORMAT;goto done;}
            work+=left->work+(d.split?right->work:0);
            depth=(d.split?1u:0u)+(left->depth>right->depth?left->depth:right->depth);
            shift+=left->shift>right->shift?left->shift:right->shift;
        }
        /* Children are already capped, so this addition cannot wrap. Rejecting
         * any reachable child also rejects its ancestors; no larger memo needed. */
        if(work>MAX_STEPS||depth>MAX_DEPTH||shift>MAX_SCALE_SHIFT){status=UC_COLLISION_LIMIT;goto done;}
        nodes[i].work=work;nodes[i].depth=(uint16_t)depth;nodes[i].shift=(uint8_t)shift;
    }
done:
    free(nodes);return status;
}
UCCollisionStatus uc_collision_mopp_prepare(const UCCollisionAsset *asset,UCCollisionMopp *out) {
    if(!asset||!out||!asset->mopp_bytes||!asset->mopp_size)return UC_COLLISION_ARGUMENT;
    if(asset->mopp_size>MAX_CODE)return UC_COLLISION_LIMIT;
    for(uint32_t i=0;i<4;i++)if(!isfinite(asset->mopp_offset[i]))return UC_COLLISION_ARGUMENT;
    if(asset->mopp_offset[3]<=0)return UC_COLLISION_ARGUMENT;
    UCCollisionMopp m={asset->mopp_bytes,asset->mopp_size,{0}};memcpy(m.info,asset->mopp_offset,sizeof(m.info));
    uint8_t *marks=(uint8_t*)calloc(m.size,1);if(!marks)return UC_COLLISION_MEMORY;
    uint32_t count=0;UCCollisionStatus status=validate(&m,0,marks,0,&count);
    if(!status)status=certify(&m,marks,count);free(marks);
    if(!status)*out=m;return status;
}

typedef struct Transform {float offset[4],scale;uint32_t key,shift;} Transform;
typedef struct Segment {float from[4],to[4];} Segment;
typedef struct Cast {
    const UCCollisionMopp *code;
    const float *from,*to;
    float fraction,inverse_scale;
    UCCollisionMoppTerminal terminal;
    void *context;
    uint32_t steps;
} Cast;
static int finite_segment(const Segment *s) {
    for(uint32_t i=0;i<4;i++)if(!isfinite(s->from[i])||!isfinite(s->to[i]))return 0;
    return 1;
}
/* Packed SSE sequence: each product/subtraction/addition rounds separately. */
static void interpolate(const float a[4],const float b[4],float t,float out[4]) {
    for(uint32_t i=0;i<4;i++){float right=t*b[i],left=1.0f-t;left=left*a[i];out[i]=left+right;}
}
static void shorten(Cast *cast,const Transform *transform,Segment *segment) {
    float endpoint[4];interpolate(cast->from,cast->to,cast->fraction,endpoint);
    for(uint32_t i=0;i<4;i++){
        float p=endpoint[i]-cast->code->info[i];p=p*transform->scale;
        segment->to[i]=p-transform->offset[i];
    }
}
static double projection(const float p[4],uint32_t op) {
    switch(op){
    case 0x13:return (double)p[2]+p[1];
    case 0x14:return (double)p[1]-p[2];
    case 0x15:return (double)p[2]+p[0];
    case 0x16:return (double)p[0]-p[2];
    case 0x17:return (double)p[1]+p[0];
    case 0x18:return (double)p[0]-p[1];
    case 0x19:return ((double)p[2]+p[1])+p[0];
    case 0x1a:return ((double)p[1]+p[0])-p[2];
    case 0x1b:return ((double)p[0]-p[1])+p[2];
    default:return ((double)p[0]-p[1])-p[2]; /* 1C */
    }
}
static UCCollisionStatus traverse(Cast *cast,Transform transform,Segment segment,uint32_t at,uint32_t depth) {
    if(depth>MAX_DEPTH)return UC_COLLISION_LIMIT;
    for(;;){
        if(++cast->steps>MAX_STEPS)return UC_COLLISION_LIMIT;
        if(!finite_segment(&segment)||!isfinite(transform.scale))return UC_COLLISION_LIMIT;
        Instruction d;UCCollisionStatus status=decode(cast->code,at,&d);if(status)return status;
        const uint8_t *p=cast->code->bytes+at;uint32_t op=d.opcode;
        if(d.terminal){
            if(op==0)return UC_COLLISION_OK;
            uint32_t key=op<0x50?op-0x30:be(p+1,d.length-1);float fraction=cast->fraction;
            status=cast->terminal(cast->context,transform.key+key,&fraction);if(status)return status;
            if(!isfinite(fraction)||fraction<0||fraction>cast->fraction)return UC_COLLISION_ARGUMENT;
            cast->fraction=fraction;return UC_COLLISION_OK;
        }
        if(op>=1&&op<=4){
            /* Borrowed staged views have no whole-program certificate. Bound
             * this path before arithmetic, including after private terminals. */
            if(transform.shift+op>MAX_SCALE_SHIFT)return UC_COLLISION_LIMIT;
            transform.shift+=op;
            float scale=(float)(1u<<op);
            for(uint32_t i=0;i<4;i++){
                float offset=i<3?(float)p[1+i]:0;
                float v=segment.from[i]-offset;segment.from[i]=v*scale;
                v=segment.to[i]-offset;segment.to[i]=v*scale;
                v=transform.offset[i]+offset;transform.offset[i]=v*scale;
            }
            transform.scale=(float)((double)scale*transform.scale);at=d.left;continue;
        }
        if(op==9||op==10){transform.key+=be(p+1,d.length-1);at=d.left;continue;}
        if(op==11){transform.key=be(p+1,4);at=d.left;continue;}
        /* Properties are local metadata copies in32C1C0; neither collector-path
         * terminal callback reads them. Preserve decoding, with no fake effect. */
        if((op>=5&&op<=7)||op>=0x60){at=d.left;continue;}
        if(d.split){
            uint32_t axis=999;float upper,lower;double start;float end;
            if(op>=0x10&&op<=0x12){axis=op-0x10;upper=(float)p[1];lower=(float)p[2];}
            else if(op>=0x20&&op<=0x22){axis=op-0x20;lower=(float)p[1];upper=lower+1.0f;}
            else if(op>=0x23&&op<=0x25){axis=op-0x23;upper=(float)p[1];lower=(float)p[2];}
            else {
                int multiplier=op>=0x19?3:2;
                int bias=(op==0x14||op==0x16||op==0x18)?255:(op>=0x1a&&op<=0x1b)?255:op==0x1c?510:0;
                upper=(float)(multiplier*p[1]-bias);lower=(float)(multiplier*p[2]-bias);
            }
            if(axis<3){start=segment.from[axis];end=segment.to[axis];}
            else{start=projection(segment.from,op);end=(float)projection(segment.to,op);}
            if(end<lower&&start<lower){at=d.left;continue;}
            if(start>upper&&end>upper){at=d.right;continue;}
            float su=(float)(start-upper),sl=(float)(start-lower),eu=end-upper,el=end-lower;
            int increasing=su<eu;float a=increasing?su:sl,b=increasing?eu:el;
            Segment first=segment;
            if((double)a*b<0){float t=(float)((double)a/((double)a-b));interpolate(segment.from,segment.to,t,first.to);}
            status=traverse(cast,transform,first,increasing?d.left:d.right,depth+1);if(status)return status;
            a=increasing?sl:su;b=increasing?el:eu;
            if((double)a*b<0){float t=(float)((double)a/((double)a-b));interpolate(segment.from,segment.to,t,segment.from);}
            if(cast->fraction<1){
                shorten(cast,&transform,&segment);
                if(axis<3&&((increasing&&segment.to[axis]<lower)||(!increasing&&segment.to[axis]>upper)))return UC_COLLISION_OK;
            }
            at=increasing?d.right:d.left;continue;
        }
        /*26..2B clips the segment to a slab, without creating another child. */
        uint32_t axis=op<0x29?op-0x26:op-0x29;float lower;double upper;
        if(op<0x29){lower=(float)p[1];upper=p[2];}
        else{
            lower=(float)(((double)be(p+1,3)*cast->inverse_scale)*transform.scale-transform.offset[axis]);
            upper=((double)be(p+4,3)*cast->inverse_scale)*transform.scale-transform.offset[axis];
        }
        float start=segment.from[axis],end=segment.to[axis];int increasing=start<end;
        if((increasing&&(end<lower||start>upper))||(!increasing&&(start<lower||end>upper)))return UC_COLLISION_OK;
        Segment before=segment;float a=(float)((double)start-upper);double b=(double)end-upper;
        if((double)a*b<0){float t=(float)((double)a/((double)a-b));interpolate(before.from,before.to,t,increasing?segment.to:segment.from);}
        double low_start=(double)start-lower,low_end=(double)end-lower;
        if(low_start*low_end<0){float t=(float)(low_start/(low_start-low_end));interpolate(before.from,before.to,t,increasing?segment.from:segment.to);}
        at=d.left;
    }
}
static UCCollisionStatus cast(const UCCollisionMopp *m,const float from[4],const float to[4],
    float *fraction,UCCollisionMoppTerminal terminal,void *context) {
    if(!m||!m->bytes||!m->size||!from||!to||!fraction||!terminal||
        !isfinite(*fraction)||*fraction<0||*fraction>1)return UC_COLLISION_ARGUMENT;
    if((_mm_getcsr()&0xe040u)||fegetround()!=FE_TONEAREST)return UC_COLLISION_UNSUPPORTED;
    for(uint32_t i=0;i<4;i++)if(!isfinite(from[i])||!isfinite(to[i])||!isfinite(m->info[i]))return UC_COLLISION_ARGUMENT;
    if(m->info[3]<=0)return UC_COLLISION_ARGUMENT;
    /* Numeric admission for the shared arithmetic. Prepared programs certify
     * all paths in advance; borrowed staged programs enforce path limits above. With inputs
     * <=2^32, info.w>=2^-32 and certified rescale shift<=32, scale<=2^48 and
     * local offsets<2^43. A clipped interpolation has t in[0,1]; its magnitude
     * grows by at most (1+2^-24)^3 plus subnormal rounding. There are at most
     * two clips/instruction, so 4M steps give <8 cumulative relative growth.
     * Full-ray shortening starts again from the bounded inputs. Segment values
     * stay below2^88; the larger24-bit slab calculation stays below2^106.
     * Float differences and double products/ratios therefore cannot overflow.
     * This deliberately conservative domain is not an Xbox input restriction.
     * Callbacks must retain the immutable inputs and required arithmetic mode;
     * their errors or invalid fractions remain an explicit separate contract. */
    for(uint32_t i=0;i<4;i++)if(fabsf(from[i])>0x1p32f||fabsf(to[i])>0x1p32f||fabsf(m->info[i])>0x1p32f)return UC_COLLISION_LIMIT;
    if(m->info[3]<0x1p-32f)return UC_COLLISION_LIMIT;
    Transform transform={{0},0,0,0};transform.scale=(float)((double)m->info[3]*0x1p-16);
    Cast cast={m,from,to,*fraction,(float)(1.0/(double)m->info[3]),terminal,context,0};Segment segment;
    if(!isfinite(cast.inverse_scale))return UC_COLLISION_LIMIT;
    for(uint32_t i=0;i<4;i++){
        float a=from[i]-m->info[i],b=to[i]-m->info[i];segment.from[i]=a*transform.scale;segment.to[i]=b*transform.scale;
    }
    UCCollisionStatus status=traverse(&cast,transform,segment,0,0);
    if(!status)*fraction=cast.fraction;return status;
}

UCCollisionStatus uc_collision_mopp_cast(const UCCollisionMopp *m,const float from[4],const float to[4],
 float *fraction,UCCollisionMoppTerminal terminal,void *context){
 return cast(m,from,to,fraction,terminal,context);
}
UCCollisionStatus uc_collision_mopp_cast_borrowed_staged(const UCCollisionMopp *m,const float from[4],const float to[4],
 float *fraction,UCCollisionMoppTerminal terminal,void *context){
 if(!m||!m->bytes||!m->size)return UC_COLLISION_ARGUMENT;
 if(m->size>MAX_CODE)return UC_COLLISION_LIMIT;
 return cast(m,from,to,fraction,terminal,context);
}

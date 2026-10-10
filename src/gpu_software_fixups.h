#ifndef UCRR_GPU_SOFTWARE_FIXUPS_H
#define UCRR_GPU_SOFTWARE_FIXUPS_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#ifndef UCRR_FIXUP_WRITE_NOTIFY
#define UCRR_FIXUP_WRITE_NOTIFY(ram,size,address,bytes) ((void)0)
#endif

/* Title-specific adapter for XDK software operations12/13/14. Original
 * 003EBDB0 dispatches to003EF7D0: patch return jump, apply byte fixups, retire
 * each descriptor. This executes on the decoder owner before following the
 * next jump. It does not signal GPU fences or implement other interrupts.
 * Self-modifying descriptor/source lists are deliberately rejected. */
typedef struct UcrrFixupNode {
    uint32_t address, flags, patch, source, source_end, base;
} UcrrFixupNode;
static uint32_t ucrr_fixup_read(const uint8_t *ram,uint32_t a){uint32_t v;memcpy(&v,ram+a,4);return v;}
static void ucrr_fixup_write(uint8_t *ram,uint32_t a,uint32_t v){memcpy(ram+a,&v,4);}
static void ucrr_fixup_store(uint8_t *ram,size_t size,uint32_t a,uint32_t v){
    ucrr_fixup_write(ram,a,v);UCRR_FIXUP_WRITE_NOTIFY(ram,size,a,4);
}
static int ucrr_fixup_range(size_t size,uint32_t va,uint32_t length,uint32_t *a){
    if(va>=0x80000000u && va<0x90000000u)va&=0x0FFFFFFFu;
    if((uint64_t)va+length>size)return 0;
    *a=va;return 1;
}
static int ucrr_fixup_overlap(uint32_t a,uint32_t n,uint32_t b,uint32_t m){return (uint64_t)a+n>b&&(uint64_t)b+m>a;}
static int ucrr_gpu_fixups(uint8_t *ram,size_t size,uint32_t param){
    unsigned operation=param&31u,count=0;uint32_t descriptor;
    UcrrFixupNode nodes[64];
    if(operation<12||operation>14)return 0;
    if(!ram||size<0x3F4294u)return -1;
    if(operation==13){if((param>>5)>=64)return -1;descriptor=0x3F44A4u+20u*(param>>5);}
    else descriptor=(param>>5)|0x80000000u;
    if(operation!=14&&!ucrr_fixup_read(ram,0x3F4290u))return -1;
    /* Preflight the whole chain before writing anything. */
    for(;;){
        uint32_t a,next,p;UcrrFixupNode *node;
        if(count==64||!ucrr_fixup_range(size,descriptor,20,&a)||(a&3u))return -1;
        for(unsigned i=0;i<count;++i)if(nodes[i].address==a)return -1;
        node=&nodes[count++];memset(node,0,sizeof(*node));node->address=a;
        node->flags=ucrr_fixup_read(ram,a);next=ucrr_fixup_read(ram,a+4);
        if(!ucrr_fixup_range(size,ucrr_fixup_read(ram,a+8),4,&node->patch)||
           (node->patch&3u)||!ucrr_fixup_range(size,node->flags&0x0FFFFFFFu,4,&p)||(p&3u))return -1;
        if(node->flags&0x20000000u){
            if(!ucrr_fixup_range(size,ucrr_fixup_read(ram,a+12),4,&node->source)||
               !ucrr_fixup_range(size,ucrr_fixup_read(ram,a+16),0,&node->base))return -1;
            p=node->source;unsigned records=0;
            for(;;){
                uint32_t n,d;
                if(!ucrr_fixup_range(size,p,4,&p)||++records>65536)return -1;
                n=ucrr_fixup_read(ram,p);
                if(n==UINT32_MAX){node->source_end=p+4;break;}
                if(n>8u*1024u*1024u||!ucrr_fixup_range(size,p,8,&p))return -1;
                uint64_t dest=(uint64_t)node->base+ucrr_fixup_read(ram,p+4);
                if(dest>UINT32_MAX||!ucrr_fixup_range(size,(uint32_t)dest,n,&d)||
                   (uint64_t)p+8+n>size)return -1;
                p+=8+n;
            }
        }
        if(next==0||next==UINT32_MAX)break;
        descriptor=next;
    }
    /* Exclude metadata/source self-modification, which would invalidate the
     * preflight. Ordinary overlapping destination patches retain their order. */
    for(unsigned i=0;i<count;++i){
        UcrrFixupNode *n=&nodes[i];uint32_t p=n->source;
        for(unsigned phase=0;;++phase){
            uint32_t d=n->patch,bytes=4;
            if(phase){
                if(!n->source_end||p==n->source_end-4)break;
                bytes=ucrr_fixup_read(ram,p);d=n->base+ucrr_fixup_read(ram,p+4);p+=8+bytes;
            }
            if(ucrr_fixup_overlap(d,bytes,0x3F4290u,4))return -1;
            for(unsigned j=0;j<count;++j){
                if(ucrr_fixup_overlap(d,bytes,nodes[j].address,20)||
                   (nodes[j].source_end&&ucrr_fixup_overlap(d,bytes,nodes[j].source,nodes[j].source_end-nodes[j].source)))return -1;
            }
        }
    }
    if(operation!=14)ucrr_fixup_store(ram,size,0x3F4290u,ucrr_fixup_read(ram,0x3F4290u)-1);
    for(unsigned i=0;i<count;++i){
        UcrrFixupNode *n=&nodes[i];uint32_t p=n->source;
        ucrr_fixup_store(ram,size,n->patch,(n->flags&0x0FFFFFFFu)+1);
        while(n->source_end&&p<n->source_end-4){
            uint32_t bytes=ucrr_fixup_read(ram,p),d=n->base+ucrr_fixup_read(ram,p+4);
            memcpy(ram+d,ram+p+8,bytes);
            if(bytes)UCRR_FIXUP_WRITE_NOTIFY(ram,size,d,bytes);
            p+=8+bytes;
        }
        ucrr_fixup_store(ram,size,n->address+4,UINT32_MAX);
        if(n->flags&0x80000000u)ucrr_fixup_store(ram,size,n->address,n->flags&0xBFFFFFFFu);
    }
    return 1;
}
#endif

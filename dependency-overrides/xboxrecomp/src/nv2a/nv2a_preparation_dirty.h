#ifndef NV2A_PREPARATION_DIRTY_H
#define NV2A_PREPARATION_DIRTY_H
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
/* Include before PGRAPH initialization/writers; no guest or host state reads.
 * Dirty domains defer one exact source comparison to the next draw. Repeated
 * writes do not force a new compiler lookup. Constants/geometry are excluded. */
#if UCRR_PREPARATION_TRANSACTION
enum { NV_TX_PREFIX=1, NV_TX_MATERIAL=2, NV_TX_RAW=4, NV_TX_ALL=7 };
static struct { int option; unsigned dirty; uint64_t epoch; } nv_tx_dirty={-1,NV_TX_ALL,1};
static int nv_tx_enabled(void){
    if(nv_tx_dirty.option<0){const char *v=getenv("UCRR_PREPARATION_TRANSACTION");
        nv_tx_dirty.option=v&&!strcmp(v,"1");}
    return nv_tx_dirty.option;
}
static unsigned nv_tx_method_domain(unsigned m){
    if((m>=0x300&&m<=0x30C)||(m>=0x33C&&m<=0x348)||
       (m>=0x354&&m<=0x378)||m==0x32C||m==0x39C||m==0x3A0||m==0x2A8)return NV_TX_PREFIX;
    if(m>=0x1B00&&m<0x1C00){unsigned at=m&0x3F;
        if(at==8||at==12||at==20)return NV_TX_MATERIAL;}
    if((m>=0x260&&m<0x280)||(m>=0xAC0&&m<0xAE0)||
       (m>=0xAA0&&m<0xAC0)||(m>=0x1E40&&m<0x1E60)||
       m==0x1E60||m==0x288||m==0x28C||m==0x1E70||m==0x1E78)return NV_TX_RAW;
    return 0;
}
static void nv_tx_dirty_range(unsigned method,unsigned count,unsigned incrementing){
    if(!nv_tx_enabled())return;
    unsigned domains=0;
    for(unsigned i=0;i<count;++i)domains|=nv_tx_method_domain(method+(incrementing?4*i:0));
    if(domains){nv_tx_dirty.dirty|=domains;++nv_tx_dirty.epoch;
        if(!nv_tx_dirty.epoch)++nv_tx_dirty.epoch;}
}
static void nv_tx_dirty_reset(void){
    nv_tx_dirty.dirty=NV_TX_ALL;++nv_tx_dirty.epoch;
    if(!nv_tx_dirty.epoch)++nv_tx_dirty.epoch;
}
#else
#define nv_tx_dirty_range(m,n,i) ((void)0)
#define nv_tx_dirty_reset() ((void)0)
#endif
#endif

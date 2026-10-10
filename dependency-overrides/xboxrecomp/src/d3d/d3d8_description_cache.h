#ifndef UCRR_DESCRIPTION_CACHE_H
#define UCRR_DESCRIPTION_CACHE_H
/* Exact structural descriptions only: no handles or resource-validity claims. */
typedef struct DescriptionKey {uint32_t words[37];} DescriptionKey;
typedef struct DescriptionEntry {unsigned valid;DescriptionKey key;NV2ACombinerState state;} DescriptionEntry;
typedef struct DescriptionCache {DescriptionEntry entries[64];unsigned hits,misses;} DescriptionCache;
static void description_key(const uint32_t*m,DescriptionKey*k){
    memcpy(k->words,m+0x260/4,32);memcpy(k->words+8,m+0xAC0/4,32);
    memcpy(k->words+16,m+0xAA0/4,32);memcpy(k->words+24,m+0x1E40/4,32);
    k->words[32]=m[0x1E60/4];k->words[33]=m[0x288/4];k->words[34]=m[0x28C/4];
    k->words[35]=m[0x1E70/4];k->words[36]=m[0x1E78/4];
}
static void description_prepare(DescriptionCache*c,const uint32_t*m,NV2ACombinerState*out){
    DescriptionKey key;description_key(m,&key);
    /* Hash chooses a slot only. Every structural word is compared on a hit. */
    unsigned hash=2166136261u;
    for(unsigned i=0;i<37;++i)hash=(hash^key.words[i])*16777619u;
    DescriptionEntry*e=&c->entries[(hash^(hash>>16))&63];
    if(e->valid&&!memcmp(&key,&e->key,sizeof(key))){*out=e->state;++c->hits;}
    else{d3d8_combiners_from_nv097(m,out);e->state=*out;e->key=key;e->valid=1;++c->misses;}
    /* Dynamic colors always read fresh, including replication flags. */
    for(unsigned i=0;i<8;++i){out->c0[i]=m[0xA60/4+((out->flags&0x10)?i:0)];out->c1[i]=m[0xA80/4+((out->flags&0x100)?i:0)];}
    out->final_c0=m[0x1E20/4];out->final_c1=m[0x1E24/4];
}
#endif

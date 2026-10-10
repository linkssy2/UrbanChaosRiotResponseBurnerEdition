#ifndef D3D8_PERSISTENT_UP_H
#define D3D8_PERSISTENT_UP_H
/* Exact owned-byte cache, independent of dynamic ring DISCARD generations.
 * First sight stores probation bytes; second identical sight creates immutable
 * GPU storage. No pointer-based identity or shader/output caching. */
#define PU_SLOTS 64
#define PU_MAX_BYTES 65536u
typedef struct PersistentUPEntry { unsigned bytes,key; unsigned char *data; ID3D11Buffer *buffer; } PersistentUPEntry;
static struct {int initialized,enabled;PersistentUPEntry entry[PU_SLOTS];uint64_t hits,misses,saved,created,failures;} pu;
static uint64_t pu_start_hits,pu_start_misses,pu_start_saved;
static void pu_capture_start(void){pu_start_hits=pu.hits;pu_start_misses=pu.misses;pu_start_saved=pu.saved;}
static unsigned pu_key(const void*data,unsigned n){const unsigned char*p=data;unsigned h=n*2166136261u;unsigned at[3]={0,n/2-8,n-16};for(unsigned j=0;j<3;++j)for(unsigned i=0;i<16;++i)h=(h^p[at[j]+i])*16777619u;return h;}
static ID3D11Buffer *pu_find(ID3D11Device*dev,const void*data,unsigned bytes){
    if(!pu.initialized){const char*v=getenv("UCRR_PERSISTENT_UP");pu.enabled=v&&!strcmp(v,"1");pu.initialized=1;}
    if(!pu.enabled||!data||bytes<64||bytes>PU_MAX_BYTES)return NULL;
    unsigned key=pu_key(data,bytes);PersistentUPEntry*e=&pu.entry[key%PU_SLOTS];
    if(e->data&&e->bytes==bytes&&e->key==key&&!memcmp(e->data,data,bytes)){
        if(!e->buffer){D3D11_BUFFER_DESC d={0};d.ByteWidth=bytes;d.Usage=D3D11_USAGE_IMMUTABLE;d.BindFlags=D3D11_BIND_VERTEX_BUFFER;D3D11_SUBRESOURCE_DATA init={data,0,0};
            if(FAILED(ID3D11Device_CreateBuffer(dev,&d,&init,&e->buffer))){++pu.failures;return NULL;}++pu.created;}
        ++pu.hits;pu.saved+=bytes;return e->buffer;
    }
    ++pu.misses;
    unsigned char *copy=malloc(bytes);if(!copy){++pu.failures;return NULL;}memcpy(copy,data,bytes);
    if(e->buffer)ID3D11Buffer_Release(e->buffer);free(e->data);*e=(PersistentUPEntry){bytes,key,copy,NULL};return NULL;
}
static void pu_dump(const wchar_t*base){if(!pu.enabled)return;wchar_t path[MAX_PATH];swprintf(path,MAX_PATH,L"%ls.persistent-up.csv",base);FILE*f=_wfopen(path,L"w");if(!f)return;fprintf(f,"lifetime_hits,lifetime_misses,lifetime_saved_bytes,created,failures,window_hits,window_misses,window_saved_bytes\n%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu\n",(unsigned long long)pu.hits,(unsigned long long)pu.misses,(unsigned long long)pu.saved,(unsigned long long)pu.created,(unsigned long long)pu.failures,(unsigned long long)(pu.hits-pu_start_hits),(unsigned long long)(pu.misses-pu_start_misses),(unsigned long long)(pu.saved-pu_start_saved));fclose(f);}
static void pu_shutdown(void){for(unsigned i=0;i<PU_SLOTS;++i){if(pu.entry[i].buffer)ID3D11Buffer_Release(pu.entry[i].buffer);free(pu.entry[i].data);}memset(&pu,0,sizeof(pu));}
#endif

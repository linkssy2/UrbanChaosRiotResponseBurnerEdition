#include "texture_resources.h"
#include "material_resources.h"
#include "archive_lookup.h"
#include "resource_handlers.h"
#include <cstring>
#include <memory>
#include <new>
#include <string>
#include <vector>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib,"bcrypt")

struct TextureNode {
    UCTextureResource view{};UCTextureAsset image{};UCTextureSequence sequence{};
    UCMaterialResource material_view{};UCMaterialAsset material{};
    std::vector<const UCTextureResource *> frames;
    ~TextureNode(){uc_texture_free(&image);uc_texture_sequence_free(&sequence);uc_material_free(&material);}
};
struct TextureStream {UCArchiveBytes bytes{};~TextureStream(){uc_archive_bytes_free(&bytes);}};
struct UCTextureResources {
    UCArchiveLookup lookup{};UCResourceCache *cache=nullptr;UCResourceHandlerServices services{};
    UCArchiveLookup *cache_provider=nullptr;UCArchiveLookup *const *graphics_provider=nullptr;
    struct Handler{UCTextureResources *owner;UCResourceHandlerKind kind;} handlers[4]{};
    UCResourceLoader loaders[4]{};UCTextureResourcesStatus status=UC_TEXTURE_RESOURCES_OK;
    UCNativeMaterialCatalog catalog{};
    std::vector<std::unique_ptr<TextureNode>> nodes;std::vector<TextureNode *> roots;
    ~UCTextureResources(){if(cache)uc_resource_cache_close(cache);uc_material_catalog_free(&catalog);}
};
static bool supported(const char *name){
    if(!name)return false;size_t n=0;for(;name[n];++n)if(n==1023||static_cast<unsigned char>(name[n])>127)return false;
    const char *dot=std::strrchr(name,'.');if(!dot)return false;char suffix[8]{};size_t i=0;
    while(dot[i]&&dot[i]!='='){if(i==sizeof suffix-1)return false;char c=dot[i];suffix[i]=c>='A'&&c<='Z'?static_cast<char>(c+32):c;++i;}
    return !std::strcmp(suffix,".tga")||!std::strcmp(suffix,".dds")||!std::strcmp(suffix,".bmdds")||!std::strcmp(suffix,".ifl");
}
static bool material_name(const char *name){
    if(!name)return false;size_t n=0;for(;name[n];++n)if(n==1023||static_cast<unsigned char>(name[n])>127)return false;
    const char *dot=std::strrchr(name,'.');if(!dot)return false;char suffix[8]{};size_t i=0;
    while(dot[i]&&dot[i]!='='){if(i==sizeof suffix-1)return false;char c=dot[i];suffix[i]=c>='A'&&c<='Z'?static_cast<char>(c+32):c;++i;}
    return !std::strcmp(suffix,".shader")||!std::strcmp(suffix,".csf")||!std::strcmp(suffix,".skcsf");
}
static int cache_preflight(void *ctx,const UCResourceCache *,const char *name){
    auto &o=*static_cast<UCTextureResources *>(ctx);if(!supported(name)&&!(o.catalog.storage&&material_name(name))&&!o.status)o.status=UC_TEXTURE_RESOURCES_UNSUPPORTED;return !o.status;
}
static int handler_preflight(void *ctx,UCResourceHandlerKind,void *,const char *name,const char *,uint32_t,UCResourceCache *cache){
    auto &o=*static_cast<UCTextureResources *>(ctx);return cache==o.cache&&cache_preflight(ctx,cache,name);
}
static void *open_stream(void *ctx,void *provider,const char *name,uint32_t type){
    auto &o=*static_cast<UCTextureResources *>(ctx);auto *lookup=static_cast<UCArchiveLookup *>(provider);
    if(!lookup||(type!=1&&!(o.catalog.storage&&type==2))){o.status=UC_TEXTURE_RESOURCES_UNSUPPORTED;return nullptr;}
    size_t index;if(uc_archive_lookup(lookup,name,&index)){o.status=UC_TEXTURE_RESOURCES_ARCHIVE;return nullptr;}if(index==SIZE_MAX)return nullptr;
    auto *stream=new(std::nothrow)TextureStream;if(!stream){o.status=UC_TEXTURE_RESOURCES_MEMORY;return nullptr;}
    if(uc_archive_read(lookup->archive,index,UC_AR_LOCAL_PAYLOAD,&stream->bytes)!=UC_AR_OK){delete stream;o.status=UC_TEXTURE_RESOURCES_ARCHIVE;return nullptr;}return stream;
}
static void *material_provider(void *ctx){auto &o=*static_cast<UCTextureResources *>(ctx);if(o.catalog.storage&&o.graphics_provider&&*o.graphics_provider)return *o.graphics_provider;o.status=UC_TEXTURE_RESOURCES_UNSUPPORTED;return nullptr;}
static void *texture_allocator(void *ctx,UCResourceCache *){return ctx;}
static void *allocate(void *ctx,UCResourceObjectKind kind){
    auto &o=*static_cast<UCTextureResources *>(ctx);if(kind>UC_RESOURCE_SEQUENCE_OBJECT&&!(kind==UC_RESOURCE_MATERIAL_OBJECT&&o.catalog.storage)){o.status=UC_TEXTURE_RESOURCES_UNSUPPORTED;return nullptr;}
    try{auto node=std::make_unique<TextureNode>();auto *p=node.get();o.nodes.push_back(std::move(node));return p;}catch(const std::bad_alloc&){o.status=UC_TEXTURE_RESOURCES_MEMORY;return nullptr;}
}
static void *construct(void *ctx,UCResourceObjectKind kind,void *storage,const void *input,void *aux){
    auto &o=*static_cast<UCTextureResources *>(ctx);auto &node=*static_cast<TextureNode *>(storage);const auto &bytes=static_cast<const TextureStream *>(input)->bytes;
    if(kind==UC_RESOURCE_IMAGE_OBJECT){
        if(aux!=ctx){o.status=UC_TEXTURE_RESOURCES_ARGUMENT;return nullptr;}
        auto status=uc_texture_decode(bytes.data,bytes.size,&node.image);
        if(status!=UC_TEXTURE_OK){o.status=status==UC_TEXTURE_MEMORY?UC_TEXTURE_RESOURCES_MEMORY:UC_TEXTURE_RESOURCES_UNSUPPORTED;return nullptr;}
        node.view.image=&node.image;return &node;
    }
    if(kind==UC_RESOURCE_MATERIAL_OBJECT&&aux==o.cache){
        auto status=uc_material_decode(bytes.data,bytes.size,&node.material);
        if(status!=UC_MATERIAL_OK){o.status=status==UC_MATERIAL_MEMORY?UC_TEXTURE_RESOURCES_MEMORY:UC_TEXTURE_RESOURCES_UNSUPPORTED;return nullptr;}
        static const uint8_t header[]={104,0,0,0,3,0,0,1,255,255,255,1,0,0},blood_header[]={104,0,0,0,3,2,0,1,255,255,255,1,0,0},glass_header[]={172,0,69,0,3,0,0,1,255,255,255,1,0,0};auto &m=node.material;
        const bool glass=!std::memcmp(m.header,glass_header,14);
        if((!glass&&std::memcmp(m.header,header,14)&&std::memcmp(m.header,blood_header,14))||!m.dependencies[0].kind||!supported(m.dependencies[0].name)||
           m.passes[0].dependency_count!=1||m.passes[0].dependency_indices[0]){o.status=UC_TEXTURE_RESOURCES_UNSUPPORTED;return nullptr;}
        uint8_t hash[32];BCRYPT_ALG_HANDLE alg=nullptr;
        if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0){o.status=UC_TEXTURE_RESOURCES_UNSUPPORTED;return nullptr;}
        auto hashed=BCryptHash(alg,nullptr,0,bytes.data,static_cast<ULONG>(bytes.size),hash,sizeof hash);BCryptCloseAlgorithmProvider(alg,0);
        if(hashed<0){o.status=UC_TEXTURE_RESOURCES_UNSUPPORTED;return nullptr;}
        const UCNativeMaterial *d=nullptr;
        for(uint32_t i=0;i<o.catalog.count;++i)if(!std::memcmp(o.catalog.materials[i].source_sha256,hash,32)){d=o.catalog.materials+i;break;}
        if(!d||d->kind!=UC_NATIVE_MATERIAL_TEXTURE_VERTEX_COLOR||d->unsupported_reason||d->blend_mode!=UC_NATIVE_MATERIAL_BLEND_ALPHA||
           d->vertex_exposure!=1||d->constant_argb!=0xffffffffu||d->cull_mode!=(glass?1:0)||d->address_u||d->address_v||d->depth_bias||d->geometry||
           d->alpha_mode||d->color_write||d->texture_selection||std::strcmp(d->texture_name,m.dependencies[0].name)){
            o.status=UC_TEXTURE_RESOURCES_UNSUPPORTED;return nullptr;
        }
        void *texture=nullptr;auto result=uc_resource_cache_get(o.cache,m.dependencies[0].name,&texture);
        if(result||o.status){if(!o.status)o.status=result==UC_RESOURCE_MEMORY?UC_TEXTURE_RESOURCES_MEMORY:UC_TEXTURE_RESOURCES_CACHE;return nullptr;}
        auto *t=static_cast<TextureNode *>(texture);
        node.material_view={&m,d,t?&t->view:nullptr,static_cast<float>(m.header[6])*(1.f/255.f),static_cast<float>(m.header[10])*(1.f/255.f)};
        return &node;
    }
    if(kind!=UC_RESOURCE_SEQUENCE_OBJECT||aux!=o.cache){o.status=UC_TEXTURE_RESOURCES_ARGUMENT;return nullptr;}
    auto status=uc_texture_sequence_decode(bytes.data,bytes.size,&node.sequence);
    if(status!=UC_SEQUENCE_OK){o.status=status==UC_SEQUENCE_MEMORY?UC_TEXTURE_RESOURCES_MEMORY:UC_TEXTURE_RESOURCES_UNSUPPORTED;return nullptr;}
    for(uint32_t i=0;i<node.sequence.count;++i)if(!supported(node.sequence.names[i])){o.status=UC_TEXTURE_RESOURCES_UNSUPPORTED;return nullptr;}
    try{node.frames.resize(node.sequence.count);}catch(const std::bad_alloc&){o.status=UC_TEXTURE_RESOURCES_MEMORY;return nullptr;}
    for(uint32_t i=0;i<node.sequence.count;++i){void *frame=nullptr;auto result=uc_resource_cache_get(o.cache,node.sequence.names[i],&frame);
        if(result||o.status){if(!o.status)o.status=result==UC_RESOURCE_MEMORY?UC_TEXTURE_RESOURCES_MEMORY:UC_TEXTURE_RESOURCES_CACHE;return nullptr;}node.frames[i]=frame?&static_cast<TextureNode *>(frame)->view:nullptr;}
    node.view.sequence=&node.sequence;node.view.frames=node.frames.data();return &node;
}
static void release_stream(void *,void *stream,uint32_t){delete static_cast<TextureStream *>(stream);}
static void *load(void *ctx,void *provider,const char *name,const char *extension,uint32_t n,UCResourceCache *cache){
    auto &h=*static_cast<UCTextureResources::Handler *>(ctx);void *result=nullptr;
    if(uc_resource_handler_load(h.kind,&h.owner->services,provider,name,extension,n,cache,&result)&&!h.owner->status)h.owner->status=UC_TEXTURE_RESOURCES_UNSUPPORTED;
    return result;
}
static UCTextureResourcesStatus create(const UCArchive *archive,uint16_t tag,const char *const *names,size_t count,const void *catalog,size_t catalog_bytes,UCTextureResources **out,
    UCArchiveLookup *cache_provider=nullptr,UCArchiveLookup *const *graphics_provider=nullptr){
    if((!archive&&!cache_provider)||!names||!out||!count||count>65536)return UC_TEXTURE_RESOURCES_ARGUMENT;
    for(size_t i=0;i<count;++i)if(!supported(names[i])&&!(catalog&&catalog_bytes&&material_name(names[i])))return UC_TEXTURE_RESOURCES_UNSUPPORTED;
    try{
        auto o=std::make_unique<UCTextureResources>();
        if(!cache_provider&&uc_archive_lookup_init(&o->lookup,archive,tag))return UC_TEXTURE_RESOURCES_ARCHIVE;
        o->cache_provider=cache_provider?cache_provider:&o->lookup;o->graphics_provider=graphics_provider?graphics_provider:&o->cache_provider;
        if(catalog&&uc_material_catalog_decode(catalog,catalog_bytes,&o->catalog)!=UC_MATERIAL_OK)return UC_TEXTURE_RESOURCES_UNSUPPORTED;
        o->services={o.get(),handler_preflight,open_stream,material_provider,texture_allocator,allocate,construct,release_stream};
        for(unsigned i=0;i<4;++i){o->handlers[i]={o.get(),static_cast<UCResourceHandlerKind>(i)};o->loaders[i]={load,&o->handlers[i]};}
        UCResourceCacheServices h={o.get(),cache_preflight,o->loaders,4};auto status=uc_resource_cache_create(65536,&h,&o->cache);
        if(status)return status==UC_RESOURCE_MEMORY?UC_TEXTURE_RESOURCES_MEMORY:UC_TEXTURE_RESOURCES_CACHE;
        uc_resource_cache_set_provider(o->cache,o->cache_provider);o->roots.reserve(count);
        for(size_t i=0;i<count;++i){void *resource=nullptr;status=uc_resource_cache_get(o->cache,names[i],&resource);
            if(o->status)return o->status;if(status)return status==UC_RESOURCE_MEMORY?UC_TEXTURE_RESOURCES_MEMORY:UC_TEXTURE_RESOURCES_CACHE;o->roots.push_back(static_cast<TextureNode *>(resource));}
        *out=o.release();return UC_TEXTURE_RESOURCES_OK;
    }catch(const std::bad_alloc&){return UC_TEXTURE_RESOURCES_MEMORY;}
}
UCTextureResourcesStatus uc_texture_resources_create(const UCArchive *a,uint16_t tag,const char *const *n,size_t count,UCTextureResources **out){return create(a,tag,n,count,nullptr,0,out);}
UCTextureResourcesStatus uc_material_resources_create(const UCArchive *a,uint16_t tag,const char *const *n,size_t count,const void *catalog,size_t bytes,UCMaterialResources **out){
    if(!catalog||!bytes)return UC_TEXTURE_RESOURCES_ARGUMENT;return create(a,tag,n,count,catalog,bytes,out);
}
UCTextureResourcesStatus uc_material_resources_create_with_providers(UCArchiveLookup *cache_provider,UCArchiveLookup *const *graphics_provider,
    const char *const *names,size_t count,const void *catalog,size_t bytes,UCMaterialResources **out){
    if(!cache_provider||!cache_provider->archive||!graphics_provider||!*graphics_provider||!(*graphics_provider)->archive||!catalog||!bytes)return UC_TEXTURE_RESOURCES_ARGUMENT;
    return create(nullptr,0,names,count,catalog,bytes,out,cache_provider,graphics_provider);
}
size_t uc_texture_resources_count(const UCTextureResources *o){return o?o->roots.size():0;}
const UCTextureResource *uc_texture_resources_at(const UCTextureResources *o,size_t index){auto *p=o&&index<o->roots.size()?o->roots[index]:nullptr;return p&&!p->material_view.asset?&p->view:nullptr;}
const UCMaterialResource *uc_material_resources_at(const UCMaterialResources *o,size_t index){auto *p=o&&index<o->roots.size()?o->roots[index]:nullptr;return p&&p->material_view.asset?&p->material_view:nullptr;}
UCTextureResourcesStatus uc_material_resources_get(UCMaterialResources *o,const char *name,const UCMaterialResource **out){
    if(!o||!out||!o->catalog.storage||!material_name(name))return UC_TEXTURE_RESOURCES_ARGUMENT;
    if(o->status)return o->status;
    void *resource=nullptr;auto status=uc_resource_cache_get(o->cache,name,&resource);
    if(!o->status&&status)o->status=status==UC_RESOURCE_MEMORY?UC_TEXTURE_RESOURCES_MEMORY:UC_TEXTURE_RESOURCES_CACHE;
    if(o->status)return o->status;
    auto *p=static_cast<TextureNode *>(resource);*out=p?&p->material_view:nullptr;return UC_TEXTURE_RESOURCES_OK;
}
size_t uc_texture_resources_cursor(const UCTextureResources *o){return o?o->cache_provider->cursor:SIZE_MAX;}
void uc_texture_resources_close(UCTextureResources *o){delete o;}

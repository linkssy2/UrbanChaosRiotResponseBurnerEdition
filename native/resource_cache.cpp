#include "resource_cache.h"
#include <cstring>
#include <map>
#include <new>
#include <string>
struct UCResourceCache {
    std::map<std::string,void *> entries;
    UCResourceCacheServices services;
    size_t capacity;
    unsigned depth=0;
    void *provider=nullptr;
};
UCResourceCacheStatus uc_resource_cache_create(size_t capacity,const UCResourceCacheServices *h,UCResourceCache **out){
    if(!h||!out||!h->preflight||(h->count&&!h->loaders))return UC_RESOURCE_ARGUMENT;
    if(!capacity||capacity>65536||h->count>65536)return UC_RESOURCE_LIMIT;
    for(size_t i=0;i<h->count;i++)if(!h->loaders[i].load)return UC_RESOURCE_ARGUMENT;
    auto *cache=new(std::nothrow)UCResourceCache;if(!cache)return UC_RESOURCE_MEMORY;
    cache->capacity=capacity;cache->services=*h;*out=cache;return UC_RESOURCE_OK;
}
UCResourceCacheStatus uc_resource_cache_set_provider(UCResourceCache *cache,void *provider){
    if(!cache)return UC_RESOURCE_ARGUMENT;cache->provider=provider;return UC_RESOURCE_OK;
}
UCResourceCacheStatus uc_resource_cache_get(UCResourceCache *cache,const char *original,void **out){
    if(!cache||!original||!out)return UC_RESOURCE_ARGUMENT;
    size_t length=0;for(;length<1024&&original[length];length++)if(static_cast<unsigned char>(original[length])>=128)return UC_RESOURCE_ARGUMENT;
    if(length==1024||cache->depth==64)return UC_RESOURCE_LIMIT;
    try{
        std::string key;key.reserve(length);
        for(size_t i=0;i<length;i++){
            unsigned char c=static_cast<unsigned char>(original[i]);
            if(c=='/'||c=='\\'){if(key.empty()||key.back()!='\\')key+='\\';}
            else key+=static_cast<char>(c>='A'&&c<='Z'?c+32:c);
        }
        auto found=cache->entries.find(key);
        if(found==cache->entries.end()&&cache->entries.size()==cache->capacity)return UC_RESOURCE_LIMIT;
        if(!cache->services.preflight(cache->services.context,cache,original))return UC_RESOURCE_ARGUMENT;
        if(found!=cache->entries.end()){*out=found->second;return UC_RESOURCE_OK;}
        auto &entry=cache->entries.emplace(key,nullptr).first->second;
        struct Active {unsigned &depth;explicit Active(unsigned &d):depth(d){++depth;}~Active(){--depth;}} active(cache->depth);
        if(cache->provider){
            auto dot=key.find_last_of('.');const char *extension=key.c_str()+(dot==std::string::npos?0:dot);
            const char *equal=std::strchr(extension,'=');const uint32_t n=equal?static_cast<uint32_t>(equal-extension):0x7FFFFFFF;
            for(size_t i=0;i<cache->services.count;i++){
                const auto &handler=cache->services.loaders[i];
                entry=handler.load(handler.context,cache->provider,original,extension,n,cache);
                if(entry)break;
            }
        }
        *out=entry;return UC_RESOURCE_OK;
    }catch(const std::bad_alloc&){return UC_RESOURCE_MEMORY;}
}
UCResourceCacheStatus uc_resource_cache_close(UCResourceCache *cache){
    if(!cache)return UC_RESOURCE_ARGUMENT;if(cache->depth)return UC_RESOURCE_BUSY;delete cache;return UC_RESOURCE_OK;
}

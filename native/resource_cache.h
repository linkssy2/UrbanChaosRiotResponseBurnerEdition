#ifndef UC_NATIVE_RESOURCE_CACHE_H
#define UC_NATIVE_RESOURCE_CACHE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCResourceCache UCResourceCache;
typedef enum UCResourceCacheStatus {UC_RESOURCE_OK,UC_RESOURCE_ARGUMENT,UC_RESOURCE_LIMIT,UC_RESOURCE_MEMORY,UC_RESOURCE_BUSY} UCResourceCacheStatus;
typedef void *(*UCResourceHandler)(void *context,void *provider,const char *original,
    const char *extension,uint32_t extension_length,UCResourceCache *same_cache);
typedef struct UCResourceLoader {UCResourceHandler load;void *context;} UCResourceLoader;
typedef struct UCResourceCacheServices {
    void *context;
    int (*preflight)(void *,const UCResourceCache *,const char *original);
    /* Actual singleton's registered tree iteration order, immutable for the
     * whole active call tree. Neither registration order nor archive order is
     * inferred. All callbacks must complete and preserve these owners. */
    const UCResourceLoader *loaders;
    size_t count;
} UCResourceCacheServices;
/* Bounded readable22B280 lookup owner. ASCII names<=1023bytes, current C-locale
 * ASCII lowercase; collapse consecutive slash/backslash to one backslash.
 * An entry is published NULL before handlers; existing NULL is a cached miss,
 * including recursive same-key requests. New names may recurse to depth64.
 * Provider is read before each callback; set_provider is valid inside them.
 * Extension starts at last dot, or key start; length ends at first '=' after
 * that point, or INT32_MAX when absent. Original spelling is passed separately.
 * First nonnull callback result wins; NULL results are stored after each call.
 * Preflight must admit complete nested capacity/allocation/service effects.
 * A nested/handler failure after entry publication is not retryable unchanged.
 * Requests/output and service storage are valid, stable and disjoint from the
 * cache; callbacks do not throw. This owner is single-threaded.
 * No archive resolution, handler registration, shader parsing, material release
 * or original constructor/destructor effects are supplied by this owner. */
UCResourceCacheStatus uc_resource_cache_create(size_t capacity,const UCResourceCacheServices *,UCResourceCache **);
UCResourceCacheStatus uc_resource_cache_get(UCResourceCache *,const char *original,void **resource);
UCResourceCacheStatus uc_resource_cache_set_provider(UCResourceCache *,void *provider);
/* Frees only native keys/nodes. Borrowed resources belong to completing loaders.
 * Rejects while active; never invalidates live callbacks or releases resources. */
UCResourceCacheStatus uc_resource_cache_close(UCResourceCache *);
#ifdef __cplusplus
}
#endif
#endif

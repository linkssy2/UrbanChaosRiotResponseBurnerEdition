#ifndef UC_NATIVE_RESOURCE_HANDLERS_H
#define UC_NATIVE_RESOURCE_HANDLERS_H
#include "resource_cache.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Fixed retail registry pointer order:4EAB5C/60/64/68. */
typedef enum UCResourceHandlerKind {
    UC_RESOURCE_TEXTURE, UC_RESOURCE_MATERIAL, UC_RESOURCE_MODEL, UC_RESOURCE_STRING
} UCResourceHandlerKind;
typedef enum UCResourceObjectKind {
    UC_RESOURCE_IMAGE_OBJECT, UC_RESOURCE_SEQUENCE_OBJECT,
    UC_RESOURCE_MATERIAL_OBJECT, UC_RESOURCE_MODEL_OBJECT, UC_RESOURCE_STRING_OBJECT
} UCResourceObjectKind;
typedef struct UCResourceHandlerServices {
    void *context;
    int (*preflight)(void *,UCResourceHandlerKind,void *provider,const char *name,
        const char *extension,uint32_t length,UCResourceCache *);
    void *(*open)(void *,void *provider,const char *name,uint32_t type);
    void *(*material_provider)(void *); /* CURRENT graphics+34, via virtual3C. */
    void *(*texture_allocator)(void *,UCResourceCache *); /* CURRENT cache+3C. */
    void *(*allocate)(void *,UCResourceObjectKind);
    /* Actual typed constructor, not a successful stand-in. Returns the cache
     * interface: original image/sequence/material owner+4, model/string owner.
     * Input is the opened stream, except STRING receives the original name.
     * Auxiliary is captured texture allocator for IMAGE, same cache for
     * SEQUENCE/MATERIAL/MODEL, NULL for STRING. Allocation may return NULL;
     * then construction is skipped but an opened stream is still released. */
    void *(*construct)(void *,UCResourceObjectKind,void *storage,const void *input,void *auxiliary);
    void (*release_stream)(void *,void *stream,uint32_t delete_flag);
} UCResourceHandlerServices;
/* Complete ordinary96450/90F10/91000/89960 routing and lifetime sequence with
 * required completing services. Success0 includes extension/open/allocation
 * misses (NULL result); invalid1 leaves result/effects unchanged. ASCII names
 * and extension <=1023 bytes, length<=INT32_MAX, stable borrowed inputs/services,
 * no exceptions. The exact bounded comparison includes length0/prefix matches.
 * Preflight must admit all subsequently current providers/allocators and real
 * constructors before effects; it must not mutate them. Stream callbacks may
 * change the current texture allocator. Allocation/construction/cleanup must
 * preserve the captured stream and cache until their final uses. Shader open
 * uses CURRENT material_provider; other stream opens use supplied provider.
 * No archive search, resource decoding, GPU allocation, or resource destructor
 * is substituted here. Resources returned by actual constructors stay owned
 * by those services; this function releases only its temporary input stream. */
int uc_resource_handler_load(UCResourceHandlerKind,const UCResourceHandlerServices *,
    void *provider,const char *name,const char *extension,uint32_t length,
    UCResourceCache *,void **resource);
#ifdef __cplusplus
}
#endif
#endif

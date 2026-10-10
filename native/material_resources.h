#ifndef UC_NATIVE_MATERIAL_RESOURCES_H
#define UC_NATIVE_MATERIAL_RESOURCES_H
#include "texture_resources.h"
#include "material_asset.h"
#include "archive_lookup.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCTextureResources UCMaterialResources;
typedef struct UCMaterialResource {
    const UCMaterialAsset *asset;
    const UCNativeMaterial *descriptor;
    const UCTextureResource *texture; /* Actual nullable dependency result. */
    float bias,emission;
} UCMaterialResource;
/* CPU resident exact104-byte decal /172-byte static glass families plus images/IFL.
 * Blood decal source flag0x200 remains in asset; draw owner must honor its live
 * profile gate (source8FD9E). Loading a material does not authorize its draw.
 * All ordered roots and nested dependencies use ONE cache and archive cursor.
 * Material provider is explicitly the same supplied archive; general provider
 * search/startup is not inferred. Catalog is copied and each material payload's
 * SHA256 must match a qualified native descriptor before binding its dependency.
 * The same catalog/texture names must be independently qualified offline.
 * Original90EA0/90AD0 resource binding and scalar metadata are represented;
 * Xbox shader handles/context stamp are replaced by the typed PC descriptor,
 * not simulated. No GPU creation/draw occurs here. The renderer must prepare
 * these exact families and reject missing/sequence dependencies if unsupported.
 * Roots may mix material and texture names; cached misses stay NULL. Existing
 * texture owner limits apply. Parameter materials/other shader families reject.
 * Output owns catalog, material data and texture graph after archive close.
 * Failure destroys the private partial owner and leaves *out unchanged. */
UCTextureResourcesStatus uc_material_resources_create(const UCArchive *,uint16_t tag,
    const char *const *ordered_names,size_t count,const void *catalog,size_t catalog_bytes,
    UCMaterialResources **out);
/* Same owner/cache, with actual borrowed provider identities: texture/IFL opens
 * use cache_provider; uncached material opens read CURRENT *graphics_provider
 * at the original90F10 frontier. Nested dependencies still use this SAME cache.
 * Both providers use the existing original8C830 cursor. No cursor copy/reset,
 * guessed archive fallback or cache invalidation occurs when graphics changes.
 * Supplied providers/slot remain live until close; their immutable archives
 * outlive every lookup. The slot may change between calls, never concurrently.
 * Existing cached resources/misses retain their original identity. Only packed
 * local-payload archives are admitted; raw-map/SAF routing stays upstream.
 * On failure the private cache is destroyed, but borrowed cursor effects remain.
 * Retained decoded views own their bytes, as in the single-provider API. */
UCTextureResourcesStatus uc_material_resources_create_with_providers(UCArchiveLookup *cache_provider,
    UCArchiveLookup *const *graphics_provider,const char *const *ordered_names,size_t count,
    const void *catalog,size_t catalog_bytes,UCMaterialResources **out);
const UCMaterialResource *uc_material_resources_at(const UCMaterialResources *,size_t index);
/* Actual22B280 lookup through this SAME cache, including cold dependencies,
 * aliases and cached NULL misses. Original supplied archive must still be open
 * and immutable for every call; prepared views alone do not require it open.
 * Does not append a root. Existing views remain stable. Errors after cache
 * publication poison the owner; do not retry or use it after such an error.
 * Bad arguments reject before effects and leave output unchanged. */
UCTextureResourcesStatus uc_material_resources_get(UCMaterialResources *,const char *name,
    const UCMaterialResource **out);
/* texture_resources_count/at/cursor/close also accept this same owner;
 * texture_resources_at returns NULL for material roots. */
#ifdef __cplusplus
}
#endif
#endif

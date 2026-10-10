#ifndef UC_NATIVE_TEXTURE_RESOURCES_H
#define UC_NATIVE_TEXTURE_RESOURCES_H
#include "texture_asset/texture_asset.h"
#include "texture_sequence.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCTextureResources UCTextureResources;
typedef struct UCTextureResource {
    /* Exactly one of image/sequence is nonnull. An IFL retains every ordered
     * resolved frame, including original NULL misses/cycle references. */
    const UCTextureAsset *image;
    const UCTextureSequence *sequence;
    const struct UCTextureResource *const *frames;
} UCTextureResource;
typedef enum UCTextureResourcesStatus {
    UC_TEXTURE_RESOURCES_OK,UC_TEXTURE_RESOURCES_ARGUMENT,
    UC_TEXTURE_RESOURCES_UNSUPPORTED,UC_TEXTURE_RESOURCES_MEMORY,
    UC_TEXTURE_RESOURCES_ARCHIVE,UC_TEXTURE_RESOURCES_CACHE
} UCTextureResourcesStatus;
/* Transactional CPU resident owner. Requests execute in caller-supplied order,
 * using one initially fresh actual archive cursor and one resource cache; IFL
 * names resolve recursively through that SAME cache. Archive/tag/order are
 * explicit inputs, not inferred startup/language/provider-search policy.
 * Borrowed immutable archive must outlive creation only. Outputs own all image
 * bytes, IFL names and graph nodes until close. Failure leaves output unchanged
 * and destroys the entire private partial owner. Genuine misses yield NULL.
 * Domain:1..65536 roots, ASCII resource names<=1023bytes, exact .tga/.dds/.bmdds/
 * .ifl suffix before any '=' parameter, local payloads, existing texture/IFL
 * decoder domains; <=65536 cache entries and <=64 active nested lookups.
 * No lookups for unprepared names after creation. No GPU object/upload, general
 * provider search, material/model/string constructor, frame fallback or renderer
 * admission is implied. A renderer must reject missing/nonimage selected frames.
 */
UCTextureResourcesStatus uc_texture_resources_create(const UCArchive *,uint16_t selection_tag,
    const char *const *ordered_names,size_t count,UCTextureResources **);
size_t uc_texture_resources_count(const UCTextureResources *);
const UCTextureResource *uc_texture_resources_at(const UCTextureResources *,size_t root_index);
size_t uc_texture_resources_cursor(const UCTextureResources *);
void uc_texture_resources_close(UCTextureResources *);
#ifdef __cplusplus
}
#endif
#endif

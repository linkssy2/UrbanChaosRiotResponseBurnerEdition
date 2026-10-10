#ifndef UC_NATIVE_TEXTURE_ASSET_H
#define UC_NATIVE_TEXTURE_ASSET_H
#include "archive_reader.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Packed engine .tga resources, not desktop TGA. The supported domain is
 * power-of-two 2D textures. No guest/GPU runtime or graphics API is required. */
typedef enum UCTextureFormat {
    UC_TEXTURE_BC1_UNORM = 1, UC_TEXTURE_BC2_UNORM,
    UC_TEXTURE_BC3_UNORM, UC_TEXTURE_BGRA8_UNORM, UC_TEXTURE_P8_UINT
} UCTextureFormat;
typedef enum UCTextureStatus {
    UC_TEXTURE_OK, UC_TEXTURE_ARGUMENT, UC_TEXTURE_TRUNCATED,
    UC_TEXTURE_FORMAT, UC_TEXTURE_UNSUPPORTED, UC_TEXTURE_LIMIT,
    UC_TEXTURE_MEMORY, UC_TEXTURE_ARCHIVE
} UCTextureStatus;
#define UC_TEXTURE_MAX_MIPS 13u
typedef struct UCTextureMip {
    uint32_t width, height, row_pitch, slice_pitch;
    const uint8_t *data;
} UCTextureMip;
typedef struct UCTextureAsset {
    UCTextureFormat format;
    uint32_t width, height, mip_count, source_selector, source_flags;
    size_t storage_bytes;
    uint8_t *storage;
    UCTextureMip mips[UC_TEXTURE_MAX_MIPS];
} UCTextureAsset;

/* Output must initially be zero or already freed. Failure leaves it zero.
 * Successful output owns all mip data independently of the input/archive.
 * source_flags preserves header bytes20..23; only byte21 has a recovered
 * meaning (cube selector); the other bytes are not interpreted as flags.
 * BC blocks remain byte exact. ARGB8 becomes linear BGRA8, without changing
 * channel bytes, color space, or alpha. P8 becomes linear one-byte indices;
 * its palette is separate live draw state, never guessed or baked into this
 * resource. Consumers must explicitly admit P8 plus its actual palette.
 * No generated/omitted mip levels. */
UCTextureStatus uc_texture_decode(const void *bytes, size_t size, UCTextureAsset *out);
UCTextureStatus uc_texture_load(const UCArchive *archive, size_t index,
    UCArchiveSource source, UCTextureAsset *out);
void uc_texture_free(UCTextureAsset *texture);
const char *uc_texture_status(UCTextureStatus status);
#ifdef __cplusplus
}
#endif
#endif

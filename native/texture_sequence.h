#ifndef UC_NATIVE_TEXTURE_SEQUENCE_H
#define UC_NATIVE_TEXTURE_SEQUENCE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct UCTextureSequence {
    uint32_t count;
    const char *const *names;
    void *storage;
} UCTextureSequence;
typedef struct UCTextureSequenceState { int32_t index; float scalar; } UCTextureSequenceState;
#define UC_TEXTURE_SEQUENCE_STATE_DEFAULT {0, 1.0f}
typedef struct UCTextureSelection { uint32_t index; const char *name; } UCTextureSelection;
typedef enum UCTextureSequenceStatus {
    UC_SEQUENCE_OK, UC_SEQUENCE_ARGUMENT, UC_SEQUENCE_FORMAT,
    UC_SEQUENCE_LIMIT, UC_SEQUENCE_MEMORY, UC_SEQUENCE_DOMAIN
} UCTextureSequenceStatus;

/* Owns exact ordered resource names from an IFL payload. Output must be zero or
 * freed; failure leaves it zero. Admission limits: 1..4096 entries, 1..1023 name
 * bytes, no embedded NUL, no trailing data. Names are resource keys, never paths
 * to open. No case/slash normalization, resource resolution, or texture decode.
 * Before rendering, the resource owner must resolve/validate EVERY named texture
 * and retain it. Missing/unsupported resources must not become frame0 fallbacks.
 * Treat all successful fields/storage as read-only until free. */
UCTextureSequenceStatus uc_texture_sequence_decode(const void *data, size_t size,
    UCTextureSequence *out);
void uc_texture_sequence_free(UCTextureSequence *sequence);

/* Exact period1/offset0 selection family only. renderer_phase is the original
 * renderer-manager scalar input, NOT skeletal clip phase or an invented timer.
 * Qualified finite domain: 0 <= renderer_phase <= 2^24; finite state.scalar;
 * positive immutable count. Default nearest-even FP, denormals enabled.
 * State scalar equal to either signed zero selects state.index (must be in range).
 * Any other finite scalar, or absent state, selects trunc(count * phase)%count,
 * where phase is the float remainder of renderer_phase / 1 + 0 by 1. The product
 * is evaluated in double: all admitted count/float products are exact, matching
 * the original x87 conversion domain. The scalar is a branch input, not a rate.
 * Output unchanged on failure; list and optional per-instance state never change.
 * Returned name is borrowed until sequence free. Caller binds that resolved
 * texture; no shared mutable selected index or GPU/resource callbacks are used. */
UCTextureSequenceStatus uc_texture_sequence_select(const UCTextureSequence *sequence,
    const UCTextureSequenceState *state, float renderer_phase, UCTextureSelection *out);
/* 65360 with the407C0 skin selector: stop animation, use signed selection%count.
 * Admits the existing1..4096 frame bound; negative remainder is rejected before
 * publication rather than reading outside the texture list. No resource lookup. */
UCTextureSequenceStatus uc_texture_sequence_skin_state(int32_t selection,uint32_t count,UCTextureSequenceState *out);
#ifdef __cplusplus
}
#endif
#endif

#ifndef UC_NATIVE_AUDIO_GROUP_H
#define UC_NATIVE_AUDIO_GROUP_H
#include "audio_asset.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCAudioGroupSeed {
 const char *name; /* Borrowed immutable ampz string, never a guessed event name. */
 int16_t gain,pitch,secondary_gain;
} UCAudioGroupSeed;
typedef struct UCAudioGroup {
 const struct UCAudioGroup *parent; /* Stable shared group; no private per-voice clone. */
 int16_t gain,secondary_gain,pitch; /* Original+32,+34,+36. */
 int16_t envelope_gain,auxiliary_gain,auxiliary_pitch; /* +38,+3E,+40. */
 uint32_t dirty; /* Original+5C, setter sets bit4. */
} UCAudioGroup;
typedef struct UCAudioGroupMix {int16_t gain,pitch,secondary_gain;} UCAudioGroupMix;
/* Source2506C0 descriptor from one complete20-byte amct record and ampz data.
 * Names<=255 bytes; input bytes immutable for the lifetime of the borrowed name.
 * Remaining record fields belong to the loader/property owner, not this leaf. */
UCAudioStatus uc_audio_group_seed(const void *record,size_t record_size,
 const void *names,size_t names_size,UCAudioGroupSeed *out);
/* Fresh scalar2482E0+248130 projection. Registry name interning/copying, parent
 * retains, intrusive lists, property publication and destruction stay with the
 * group owner. Parent and its chain remain alive and unchanged during calls.
 * Group identity/name lookup MUST be shared by bank owners before this call.
 * <=64 groups including the new/current node; cycles fail before writes. */
UCAudioStatus uc_audio_group_initialize(UCAudioGroup *,const UCAudioGroupSeed *,
 const UCAudioGroup *parent);
/* Full247A80 mode1+244200 numerical path. Finite RN input, DAZ/FTZ disabled;
 * clamp0..1 first,
 * then original signed16 conversion/comparison, including tiny-positive wrap.
 * Sets dirty|4. Does not propagate updates to existing voices or consume dirty.
 * The caller must complete those original effects before using changed sound. */
UCAudioStatus uc_audio_group_volume(UCAudioGroup *,float volume);
/* Exact247CC0/247CF0/247D30 parent-first calculations, including each level's
 * clamp and signed16 return. No whole-chain sum/clamp substitution. Pure.
 * Caller guarantees every nonnull pointer readable; cycles/depth are checked.
 * Inputs/output disjoint. Errors preserve destination. No hidden allocation. */
UCAudioStatus uc_audio_group_mix(const UCAudioGroup *,UCAudioGroupMix *);
#ifdef __cplusplus
}
#endif
#endif

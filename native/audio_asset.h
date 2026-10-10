#ifndef UC_NATIVE_AUDIO_ASSET_H
#define UC_NATIVE_AUDIO_ASSET_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum UCAudioStatus {UC_AUDIO_OK, UC_AUDIO_ARGUMENT, UC_AUDIO_FORMAT,
 UC_AUDIO_UNSUPPORTED, UC_AUDIO_LIMIT, UC_AUDIO_MEMORY} UCAudioStatus;
typedef struct UCAudioAsset {
 int16_t *samples; /* Owned, interleaved PCM. Includes each block's padded tail. */
 uint32_t sample_rate, channels, frame_count, authored_frames, encoded_bytes;
 uint32_t loop_start_bytes, loop_length_bytes; /* Original wsmp units. */
 uint32_t loop_start_frame, loop_frame_count;
} UCAudioAsset;
/* Resident RIFF/WAVE, Xbox ADPCM tag 0x69, mono/stereo, 36 bytes/channel,
 * 64 frames/block. Preserves fact as metadata; the original resident loader
 * submits the data extent, not fact. wsmp: at most one forward block-aligned loop.
 * WAVS/external streams, other codecs and unknown chunks fail explicitly.
 * <=64MiB input and <=64MiB decoded PCM. Output must be zero/freed first.
 * Failure leaves output empty; success retains no input pointers.
 * Decode arithmetic matches the existing APU compatibility decoder, not an
 * independently captured Xbox hardware result. No device, mixing or playback. */
UCAudioStatus uc_audio_asset_load(const void *, size_t, UCAudioAsset *);
typedef struct UCAudioStreamInfo {
 uint32_t file_offset, file_bytes, data_offset, encoded_bytes;
} UCAudioStreamInfo;
/* Explicit PC preload of a nonlooped WAVS descriptor and its exact rsaf span.
 * No file search, inferred SAF routing, streaming queue or playback. Caller
 * owns the already-selected backing file. GUID/fmt/fact/name and strm extents
 * must match its WAVS resource. Optional pdat must exactly match the backing
 * pdat and full data's prefix; it is not prepended/replayed. Reuses the decoder.
 * Descriptor <=64KiB, span/decoded PCM <=64MiB; failure clears output. */
UCAudioStatus uc_audio_stream_info(const void *, size_t, UCAudioStreamInfo *);
UCAudioStatus uc_audio_stream_asset_load(const void *descriptor, size_t descriptor_size,
 const void *span, size_t span_size, UCAudioAsset *);
void uc_audio_asset_free(UCAudioAsset *);
#ifdef __cplusplus
}
#endif
#endif

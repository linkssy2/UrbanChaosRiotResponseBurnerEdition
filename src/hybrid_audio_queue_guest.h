#ifndef UC_HYBRID_AUDIO_QUEUE_GUEST_H
#define UC_HYBRID_AUDIO_QUEUE_GUEST_H
#include <stdint.h>
/* Callback-free, phase-local queue operations. Zero declines before any guest
 * write or CPU change. Original command execution/locking remains the owner. */
int uc_hybrid_audio_queue(uint32_t original_address);
#endif

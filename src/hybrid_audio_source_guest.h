#ifndef UC_HYBRID_AUDIO_SOURCE_GUEST_H
#define UC_HYBRID_AUDIO_SOURCE_GUEST_H
/* Complete original 246510 destructor. Returns zero before any guest/CPU/FP
 * effect when unsupported. Success has returned through the original ABI;
 * its reference-count/deleting wrappers and every downstream callback remain. */
int uc_hybrid_audio_source_destroy(void);
#endif

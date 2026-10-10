#ifndef UC_FEEDBACK_MANAGER_H
#define UC_FEEDBACK_MANAGER_H
#include "feedback_sample.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCFeedbackManager UCFeedbackManager;
typedef struct UCFeedbackOutput { float *amplitude; } UCFeedbackOutput;
typedef struct UCFeedbackManagerClose {
    void *owner;
    int (*preflight)(void *);
    void (*unregister_output)(void *,uint32_t,UCFeedbackOutput *);
    void (*release_output)(void *,uint32_t,UCFeedbackOutput *);
    void (*detach_position)(void *,const UCFeedbackPositionObserver *);
    void (*detach_player)(void *,const UCFeedbackPlayerObserver *);
} UCFeedbackManagerClose;
/* Represented67EF0 construction: one authoritative existing timed/spatial
 * queue owner, two initially EMPTY unsigned-identity observer sets, borrowed
 * world, depth0/suppress0 and four enabled bytes1. Four stable typed
 * output handles refer to the SAME queue.output floats, never a second copy.
 * Their amplitude is source-unwritten until68080 first clears it. Native zero
 * initialization is not evidence for a source-defined initial amplitude.
 * Native vector/inline storage replaces original tree/header/output allocation;
 * no heap layout, internal allocator observation, original NULL/SEH equivalence.
 * Manager is address-stable, exclusive and nonreentrant. world may be NULL in
 * constructor but must be live for any operation which actually reads it. */
int uc_feedback_manager_create(void *world,UCFeedbackMemory,UCFeedbackManager **);
UCShotFeedback *uc_feedback_manager_queue(UCFeedbackManager *);
void *uc_feedback_manager_world(const UCFeedbackManager *);
UCFeedbackOutput *uc_feedback_manager_output(UCFeedbackManager *,uint32_t);
/* Actual685B0/68630 unique insert and68CF0/68D90 erase; unsigned uintptr order
 * is the actual receiver pointer identity, never a truncated guest address.
 * Receivers and their fields remain live/immutable until removal or close.
 * No receiver callback is invoked by insert/erase. Duplicate insert is quiet.
 * The returned borrowed view is valid until the next membership mutation/close;
 * it directly references the two authoritative vectors, no independent cache.
 * capacity65536 per set; native allocation failure leaves membership intact. */
int uc_feedback_manager_position(UCFeedbackManager *,const UCFeedbackPositionObserver *,int add,int *changed);
int uc_feedback_manager_player(UCFeedbackManager *,const UCFeedbackPlayerObserver *,int add,int *changed);
const UCFeedbackObservers *uc_feedback_manager_observers(UCFeedbackManager *);
/* Actual68020 signed clamp after wrapping32-bit increment/decrement,68050 reset.
 * Nonzero enable decrements depth, zero increments it. No query gate is inferred
 * from this counter;68080 separately consumes it for physical output policy. */
int uc_feedback_manager_enable(UCFeedbackManager *,uint8_t enable);
int uc_feedback_manager_reset(UCFeedbackManager *);
uint32_t uc_feedback_manager_depth(const UCFeedbackManager *);
/* Source68E50 external order: unregister all four output identities (even if
 * never registered), then detach position receivers ascending, then player
 * receivers ascending, then clear spatial before timed queues. Required effects
 * complete and preserve all owner/receiver membership and lifetimes throughout;
 * output registration/driver is a separate real owner, never a successful stub.
 * Host releases its inline/vector storage rather than mimicking guest trees.
 * Preflight is pure; successful domain has completing callbacks/allocator frees.
 * Return0 complete,1 invalid/pre-effect,2 allocation/committed contract failure.
 * The separate release_output service completes the source output destructor's
 * remaining bindings; its inline descriptor stays live through this call.
 * No world tick, output registration/admission, device rumble or gameplay claim. */
int uc_feedback_manager_close(UCFeedbackManager *,const UCFeedbackManagerClose *);
#ifdef __cplusplus
}
#endif
#endif

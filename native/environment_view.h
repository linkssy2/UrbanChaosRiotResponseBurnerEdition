#ifndef UC_ENVIRONMENT_VIEW_H
#define UC_ENVIRONMENT_VIEW_H
#include "environment_exposure.h"
#include "environment_brightness.h"
#ifdef __cplusplus
extern "C" {
#endif
/* One actual view's small publication state; exposure/profile/tone and shared
 * meter storage remain separately owned and are borrowed, never duplicated. */
typedef struct UCEnvironmentView {
    float brightness;
    unsigned refresh_profile,pending;
} UCEnvironmentView;
/* Explicit initial view brightness. Fresh actor refresh skips its first copy.
 * This function does not choose a heap-derived meter seed or image history. */
int uc_environment_view_reset(UCEnvironmentView *,UCEnvironmentExposure *,
    const UCExposureProfile *,float initial_brightness);
/* Exact CCF80 refresh/copy gates then existing49D00; final LUT preparation is
 * the explicit admitted pre-render phase. manager_flags bit800 suppresses only
 * measurement copy. profile publication itself does not set refresh_profile.
 * Disjoint initialized owners, stable inputs, finite supported scalar domain.
 * Success opens one view phase; repeated begin is rejected before mutation. */
int uc_environment_view_begin(UCEnvironmentView *,UCEnvironmentExposure *,
    UCExposureToneProvider *,const UCExposureProfile *,const UCExposureInput *,
    uint32_t manager_flags,UCExposurePublication *);
/* Call once AFTER the admitted render. image is the stable completed prior
 * slot1 storage, not the target currently being drawn. Caller owns its identity,
 * synchronization and viewport. A skipped sample permits NULL image/viewport.
 * Input scale is read from the current tone owner, preserving later changes.
 * Zero/nonzero source gates are passed as normalized booleans. Failure leaves
 * every caller object unchanged; successful end closes the phase. No callbacks,
 * allocation, surface rotation, GPU or full215060 scheduler is implemented. */
int uc_environment_view_end(UCEnvironmentView *,int32_t *retained,
    const UCExposureToneProvider *,const UCExposureImage *,const UCExposureViewport *,
    unsigned sample_enabled,unsigned glow_present,unsigned guard_enabled,
    int32_t guard_count);
#ifdef __cplusplus
}
#endif
#endif

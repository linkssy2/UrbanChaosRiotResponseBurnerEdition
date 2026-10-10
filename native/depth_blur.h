#ifndef UC_DEPTH_BLUR_H
#define UC_DEPTH_BLUR_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCDepthBlurRequest { float amplitude,start,end; } UCDepthBlurRequest;
typedef struct UCDepthBlurSettings { float start,end,amplitude;uint8_t enabled; } UCDepthBlurSettings;
typedef struct UCDepthBlurPlan { float scene[4][5],composite[4][8],sigma; } UCDepthBlurPlan;
/*48E10 and48E80 with the actual timed-request list empty: the pain controller
 * submits directly, never allocates timed nodes. Output setters execute only
 * for a present renderobject; disabled retains its prior range/amplitude. */
void uc_depth_blur_reset(UCDepthBlurRequest *);
int uc_depth_blur_request(UCDepthBlurRequest *,float start,float end,float amplitude);
int uc_depth_blur_publish(UCDepthBlurRequest *,UCDepthBlurSettings *renderobject);
/*920B0 full-view downsample/composite corners. Source scratch floor(w/2,h/2),
 * A7710 sigma1.25, actual depth coordinates in source pixel units. */
int uc_depth_blur_plan(unsigned width,unsigned height,UCDepthBlurPlan *);
#ifdef __cplusplus
}
#endif
#endif

#ifndef UC_UI_MOTION_H
#define UC_UI_MOTION_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/*273410/273730 retained fields, excluding the original vtable. The actual
 * 640-wide UI domain is deliberate. mode0..7 retains the original entry/exit
 * direction, opacity and scale policy; notifications use mode2, scale1.
 * Set finishing=1 for the source+1C outro. No implicit deletion is performed. */
typedef struct UCUIMotion {
 float base_x,phase,in_offset,out_offset,scale_delta,alpha_factor;
 uint8_t finishing,padding[3];
 float x,y,progress,blur,offset,scale,alpha;
} UCUIMotion;
int uc_ui_motion_begin(UCUIMotion *,const float position[2],float delay,uint32_t mode,float scale);
int uc_ui_motion_step(UCUIMotion *,float delta);
/*3A7870/3A78A0/3A78D0/3A7900 actual notification placement, in original pixels.*/
typedef struct UCUINotificationPositions {float panel[2],edge[2],title[2],body[2],icon[2];} UCUINotificationPositions;
void uc_ui_notification_positions(UCUINotificationPositions *);
#ifdef __cplusplus
}
#endif
#endif

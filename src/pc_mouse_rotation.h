#ifndef UCRR_PC_MOUSE_ROTATION_H
#define UCRR_PC_MOUSE_ROTATION_H
/* Original BCE60/BFD86: pitch +688 bounded by +/-37C, yaw +68C.
 * Mouse counts are angular displacement, never velocity or normalized sticks. */
static int ucrr_pc_rotate(float*pitch,float*yaw,float limit,long dx,long dy,unsigned sensitivity){
 if(!isfinite(*pitch)||!isfinite(*yaw)||!isfinite(limit)||limit<=0||limit>3.141593f)return 0;
 double radians=0.00025*(sensitivity/100.0);
 double p=*pitch-dy*radians,y=*yaw-dx*radians;
 if(p>limit)p=limit;if(p< -limit)p=-limit;
 if(!isfinite(y))return 0;
 *pitch=(float)p;*yaw=(float)y;return 1;
}
#endif

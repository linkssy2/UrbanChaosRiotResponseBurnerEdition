#ifndef UCRR_PC_MOUSE_CAMERA_H
#define UCRR_PC_MOUSE_CAMERA_H
#include "pc_mouse_rotation.h"
extern int xbox_MouseActive(void);
extern int xbox_MouseTakeRelative(long*,long*,unsigned*);
extern void sub_000BEF80_gen(void);
int ucrr_pc_mouse_look_active(uint32_t object){
 return object>=0x10000u&&object<g_xbox_total_ram&&g_xbox_total_ram-object>=0x694u&&
        MEM32(object+0x368u)==0&&xbox_MouseActive();
}
static void ucrr_pc_apply_look(uint32_t object){
 if(!ucrr_pc_mouse_look_active(object))return;
 long dx=0,dy=0;unsigned sensitivity=0;
 if(!xbox_MouseTakeRelative(&dx,&dy,&sensitivity)||(!dx&&!dy))return;
 float pitch=MEMF(object+0x688u),yaw=MEMF(object+0x68Cu);
 if(!ucrr_pc_rotate(&pitch,&yaw,MEMF(object+0x37Cu),dx,dy,sensitivity))return;
 MEMF(object+0x688u)=pitch;MEMF(object+0x68Cu)=yaw;
 static unsigned reports;
 if(reports<32){++reports;fprintf(stderr,"[PC-MOUSE-LOOK] object=%08X dx=%ld dy=%ld sensitivity=%u pitch=%g yaw=%g\n",object,dx,dy,sensitivity,pitch,yaw);}
}
#endif

#ifndef UCRR_MENU_POINTER_TARGETS_H
#define UCRR_MENU_POINTER_TARGETS_H
#include <math.h>
#include <stdint.h>
typedef struct {uint32_t owner,row,index;float x,y,w,h;uint32_t action,table;} UCRR_MENU_TARGET;
static int ucrr_menu_target_hit(const UCRR_MENU_TARGET*t,float x,float y){
 return isfinite(x)&&isfinite(y)&&isfinite(t->x)&&isfinite(t->y)&&
  isfinite(t->w)&&isfinite(t->h)&&t->w>0&&t->h>0&&
  x>=t->x&&x<t->x+t->w&&y>=t->y&&y<t->y+t->h;
}
#endif

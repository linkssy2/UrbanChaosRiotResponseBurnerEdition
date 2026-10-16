#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
static unsigned char ram[0x20000];
static size_t g_xbox_total_ram=sizeof(ram);
#define MEM32(a) (*(uint32_t*)(ram+(a)))
#define MEMF(a) (*(float*)(ram+(a)))
static int active=1;static long pending_x,pending_y;
int xbox_MouseActive(void){return active;}
int xbox_MouseTakeRelative(long*x,long*y,unsigned*s){*x=pending_x;*y=pending_y;*s=400;pending_x=pending_y=0;return active;}
#include "../src/pc_mouse_camera.h"
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){float p=0,y=0,p2=0,y2=0;
 CHECK(ucrr_pc_rotate(&p,&y,1.5f,100,50,400));
 for(int i=0;i<10;++i)CHECK(ucrr_pc_rotate(&p2,&y2,1.5f,10,5,400));
 CHECK(fabsf(p-p2)<1e-6f&&fabsf(y-y2)<1e-6f); /* No delta-time input: partition invariant. */
 CHECK(fabsf(p+0.05f)<1e-6f&&fabsf(y+0.1f)<1e-6f);
 CHECK(ucrr_pc_rotate(&p,&y,1.5f,2000,-10000,400));CHECK(p==1.5f&&y< -2); /* No stick turn cap. */
 p=NAN;CHECK(!ucrr_pc_rotate(&p,&y,1.5f,1,1,400));p=0;CHECK(!ucrr_pc_rotate(&p,&y,-1,1,1,400));
 uint32_t object=0x10000;MEMF(object+0x37c)=1.5f;MEMF(object+0x690)=0.7f;
 pending_x=100;pending_y=-20;ucrr_pc_apply_look(object);
 CHECK(fabsf(MEMF(object+0x688)-0.02f)<1e-6f&&fabsf(MEMF(object+0x68c)+0.1f)<1e-6f&&MEMF(object+0x690)==0.7f);
 y=MEMF(object+0x68c);ucrr_pc_apply_look(object);CHECK(MEMF(object+0x68c)==y);
 active=0;pending_x=100;ucrr_pc_apply_look(object);CHECK(MEMF(object+0x68c)==y);
 active=1;MEM32(object+0x368)=1;ucrr_pc_apply_look(object);CHECK(MEMF(object+0x68c)==y);
 CHECK(!ucrr_pc_mouse_look_active(0)&&!ucrr_pc_mouse_look_active(0xfffffff0)&&!ucrr_pc_mouse_look_active(sizeof(ram)-4));
 puts("PASS direct angular mouse: signs, partition invariance, no turn cap, pitch limit, roll preservation, single consumption and gates");return 0;}

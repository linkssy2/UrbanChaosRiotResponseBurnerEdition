#ifndef UC_BRUTE_CHASE_H
#define UC_BRUTE_CHASE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Original1D2A60 retained sidestep state and whole1D2D50 state2. Fields name
 * observed consumers; comments retain original owner offsets. */
typedef struct UCBruteSide {
 uint8_t left,near_line; /*F4,10C*/
 float remaining,duration,near_time,away_time,cooldown; /*F8,FC,104,108,110*/
} UCBruteSide;
typedef struct UCBruteChase {
 uint8_t entered,charge_enabled,charge,special_close,target_moved,near_line;
 int32_t target;
 float elapsed,distance,facing_dot,seen_time;
 float target_position[3],position[3],aim[3],destination[3]; /*144,actor40,17C,128*/
 UCBruteSide side;
} UCBruteChase;
typedef struct UCBruteChaseServices {
 void *owner;double delta;
 void (*retarget)(void*,const float[3]); /*21D20*/
 uint8_t (*random_boolean)(void*); /*2D3800*/
 void (*movement_mode)(void*,uint8_t); /*bodyAC->4*/
 void (*say)(void*,int32_t,int32_t); /*DED50*/
 void (*pair)(void*,int32_t,int32_t,float,int32_t); /*DEDD0*/
 void (*transition)(void*,int32_t); /*1D1130*/
 int (*visible)(void*,int32_t); /*E05C0(target,-1,0,zero,0)*/
 int (*charge_allowed)(void*); /*1D1D20*/
 void (*aim)(void*,const float[3]); /*graph18 mode0,nullcallback*/
 void (*stop_aim)(void*); /*graph1C*/
 uint8_t (*attention)(void*); /*DEF80*/
 void (*look)(void*,const float[3]);void (*stop_look)(void*);
 int32_t (*nav_status)(void*); /*1EA00*/
 void (*navigation_busy)(void*,const float[3]); /*whole1D1EF0*/
 void (*nav_stop)(void*); /*1E830*/
 void (*acquire)(void*,uint8_t,uint8_t); /*1D1180*/
 int (*evade_allowed)(void*); /*1D17D0*/
 void (*waypoint)(void*,float[3]); /*1E910*/
 uint8_t (*near_goal)(void*); /*1E950*/
 void (*move)(void*,const float[3],int32_t,uint8_t); /*graph80*/
 int (*route_clear)(void*,const float[3]); /*32D60(point,0)*/
 double (*difficulty)(void*,float,float); /*168B0,channel0*/
 double (*difficulty_range)(void*,float,float,float,float); /*168D0,channel0*/
 int (*nav_final)(void*); /*1EA90*/
 int (*target_line)(void*,int32_t,float); /*332C0*/
 int32_t (*player)(void*); /*338B0(0)*/
 int (*target_world)(void*,int32_t,float[16]); /*1F8FC0;0 success*/
} UCBruteChaseServices;
int uc_brute_side_reset(UCBruteSide*,const UCBruteChaseServices*); /*1D2320*/
/*0 success; *active is the originalBoolean return.*/
int uc_brute_side_step(UCBruteChase*,const UCBruteChaseServices*,float distance,uint8_t*active);
int uc_brute_chase_step(UCBruteChase*,const UCBruteChaseServices*);
#ifdef __cplusplus
}
#endif
#endif

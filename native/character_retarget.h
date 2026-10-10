#ifndef UC_NATIVE_CHARACTER_RETARGET_H
#define UC_NATIVE_CHARACTER_RETARGET_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCCharacterRouteSplice {uint32_t begin,end;uint8_t append_node;} UCCharacterRouteSplice;
/*20C50 retains [begin,end), then optionally endpoint, then raw goal. Node IDs
 * are original2104E0 stable node ordinals; -1 means an untagged endpoint.
 * clear completes1F3E0(from,to,0,0) then every1FC00 in manager order, without
 * agent checks.1 splice,0 no splice,-1 invalid/query failure; output unchanged
 * otherwise. The owner resolves213510 and performs original route/detour reset. */
typedef int (*UCCharacterSpliceClear)(void*,uint32_t route_point);
int uc_character_route_splice(const int32_t *nodes,uint32_t count,uint32_t cursor,
    int32_t endpoint,UCCharacterSpliceClear,void*,UCCharacterRouteSplice*);
/*1E950: stopped/searching/arrived are near. Following/failed use the strict
 * XZ goal-to-current-waypoint distance, retaining x87 subtraction precision.
 * Returns1 near,0 distant,-1 unsupported state or nonfinite required data. */
int uc_character_route_near(int status,float radius,const float goal[3],const float waypoint[3]);
#ifdef __cplusplus
}
#endif
#endif

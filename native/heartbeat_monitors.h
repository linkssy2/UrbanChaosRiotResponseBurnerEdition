#ifndef UC_HEARTBEAT_MONITORS_H
#define UC_HEARTBEAT_MONITORS_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCHeartbeatMonitorSlot {uint32_t widget;int32_t actor,references;} UCHeartbeatMonitorSlot;
typedef struct UCHeartbeatMonitors {UCHeartbeatMonitorSlot slots[4];float slide;} UCHeartbeatMonitors;
typedef struct UCHeartbeatMonitorServices {
 void*owner;int(*bind)(void*,uint32_t,int32_t,const char*,const char*,const char*);int(*clear)(void*,uint32_t);
 int(*active)(void*,uint32_t);int(*exists)(void*,int32_t);int(*step)(void*,uint32_t,float);
} UCHeartbeatMonitorServices;
void uc_heartbeat_monitors_init(UCHeartbeatMonitors*);
int uc_heartbeat_monitors_show(UCHeartbeatMonitors*,int32_t,const char*,const UCHeartbeatMonitorServices*);
int uc_heartbeat_monitors_join(UCHeartbeatMonitors*,int32_t,const char*,const char*,const char*,const UCHeartbeatMonitorServices*);
int uc_heartbeat_monitors_remove(UCHeartbeatMonitors*,int32_t,const UCHeartbeatMonitorServices*);
int uc_heartbeat_monitors_step(UCHeartbeatMonitors*,float,const UCHeartbeatMonitorServices*);
/*Whole2D5140 easing used by717B0, before its30px row multiplication.*/
double uc_heartbeat_monitors_ease(float);
#ifdef __cplusplus
}
#endif
#endif

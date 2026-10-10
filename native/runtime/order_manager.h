#pragma once
#include "order_client.h"
#include <vector>
extern "C" {
#include "../order_manager.h"
}
struct NativeOrderLook {int32_t object=-1,type=0;float point[3]{};};
struct NativeOrderSprite {
 uint32_t id=0;int32_t client=-1,command=0,slot=-1;const char *material=nullptr;
 uint32_t align_x=0,align_y=0;float color[3]{1,1,1},alpha=0;
};
struct NativeOrderManagerServices {
 void *owner=nullptr;
 //Source156E60: actual parent Look point and collision/selectable-item owners.
 int (*look)(void *,int32_t parent,NativeOrderLook &)=nullptr;
 //1F7AA0 player/parent booleans; client action booleans use the actual client.
 int (*property)(void *,int32_t,const char *,uint8_t fallback,uint8_t *)=nullptr;
 int (*control)(void *,uint8_t *)=nullptr;
 int (*gameplay)(void *,uint8_t *)=nullptr;
 int (*input)(void *,uint32_t player,uint32_t query,uint32_t mode,double *)=nullptr;
 //Original128 query to registered veto entities. Required only when nonempty.
 int (*veto)(void *,int32_t,const char *,int32_t sender,uint8_t *handled,uint8_t *accepted)=nullptr;
 int (*sound)(void *)=nullptr; //Banks\Panel.amp "Order"
 int (*forced)(void *)=nullptr; //actual HUD/tutorial owner5152B0 byte127 publication
 int (*prompt)(void *,uint32_t source_candidate_ordinal)=nullptr;
 int (*action_feedback)(void *,const char *source_material)=nullptr; //70D70
};
/* Original159290 owner with actual dynamic registration and event handshake.
 * Root must connect real input/look/sound and draw the borrowed sprite state;
 * none is silently replaced with an empty successful callback. */
class NativeOrderManager {
 struct Impl;std::unique_ptr<Impl> p_;
 static int step(void *);int close_detached();
public:
 NativeMissionEntity entity;
 NativeOrderManager();~NativeOrderManager();
 NativeOrderManager(const NativeOrderManager&)=delete;
 NativeOrderManager&operator=(const NativeOrderManager&)=delete;
 int construct(NativeMissionWorld &,int32_t parent,uint32_t player,const NativeOrderManagerServices &);
 int update();int late();int remove();
 int integer(const char *,int32_t *,uint8_t *found)const;
 int vector(const char *,float[3],uint8_t *found)const;
 const std::vector<NativeOrderSprite>&sprites()const;
 const UCOrderManager&selection()const;
 static NativeOrderManager *find(NativeMissionWorld &,int32_t id);
};

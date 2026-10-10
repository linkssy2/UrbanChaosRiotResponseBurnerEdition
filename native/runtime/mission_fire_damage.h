#pragma once
#include "mission_world.h"
#include "navigation_boxes.h"
/* Real dynamic1A5A50/1A6000/1A5BD0 owner. Required player callbacks execute
 * actual Player6C rules; this owner never substitutes health subtraction.
 * Fresh offline domain only. Services owner/delta/remove are rebound through
 * this concrete entity so timer expiry retires the same registered allocation. */
class NativeMissionFireDamage {
 NativeMissionWorld*world_=nullptr;NativeNavigationBoxes*navigation_=nullptr;
 UCFireDamageSphere state_{};UCFireDamageServices services_{};bool removed_=false,box_=false;int failure_=0;
public:
 NativeMissionEntity entity;
 int construct(NativeMissionWorld&,int32_t parent,NativeNavigationBoxes&,const UCFireDamageServices&,
     float radius,float seconds,int32_t owner,int32_t session,int32_t unique);
 int update();int remove();int close_detached();
 const UCFireDamageSphere&state()const{return state_;}
};

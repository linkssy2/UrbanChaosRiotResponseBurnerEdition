#pragma once
#include "mission_world.h"
extern "C" {
#include "../order_client.h"
}
struct NativeOrderAction {int32_t id;const char *name;int32_t slot;const char *icon;uint8_t contextual;};
struct NativeOrderClientCallbacks {
 void *owner=nullptr;
 int (*accept)(void *,int32_t manager,const float position[3],uint8_t *)=nullptr;
 int (*attached)(void *)=nullptr;
 int (*choose)(void *,int32_t command)=nullptr;
 int (*completed)(void *)=nullptr;
 //1F7A70("Position") needs the real manager property owner, never a guessed
 //player ID/transform. Return0 and found0 for an absent source property.
 int (*manager_position)(void *,int32_t,float[3],uint8_t *found)=nullptr;
 int (*integer)(void *,const char *,int32_t *,uint8_t *found)=nullptr;
 int (*vector)(void *,const char *,float[3],uint8_t *found)=nullptr;
 //Native ownership suffix, after registry detach and subscription release.
 void (*closed)(void *)=nullptr;
 //Concrete subclass virtual24 prefix, before40D80 notifies its manager.
 int (*removing)(void *)=nullptr;
};
/* Real dynamic OrderClient base. NativeMissionWorld owns its binding/lifetime;
 * callbacks are borrowed from its concrete Medic/HealOrderClient subclass.
 * The subclass stays alive until this client's deferred retirement completes.
 * No OrderManager input/selection or subclass property behavior is fabricated. */
class NativeOrderClient {
 NativeMissionWorld *world_=nullptr;NativeOrderClientCallbacks callbacks_{};
 UCEntitySubscriptions *subscriptions_=nullptr;UCOrderClient state_{};
 bool removed_=false;int failure_=0;
 UCOrderClientServices services();
 static int step(void *);
 int retire();int close_detached();
 NativeOrderClient()=default;
public:
 NativeMissionEntity entity;
 ~NativeOrderClient();
 NativeOrderClient(const NativeOrderClient&)=delete;
 NativeOrderClient&operator=(const NativeOrderClient&)=delete;
 static int create(NativeMissionWorld &,int32_t parent,const char *actual_class,
                   const NativeOrderClientCallbacks &,NativeOrderClient **);
 static NativeOrderClient *find(NativeMissionWorld &,int32_t id);
 int update();int remove();int complete(int32_t command);
 int advertise(const NativeOrderAction &);
 int integer(const char *,int32_t *,uint8_t *found)const;
 int vector(const char *,float[3],uint8_t *found)const;
 int32_t manager()const{return state_.manager;}
 int32_t pending()const{return state_.pending;}
 bool ready()const{return world_&&!removed_&&!failure_;}
};

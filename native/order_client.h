#ifndef UC_ORDER_CLIENT_H
#define UC_ORDER_CLIENT_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Original40F20/40E10 and40D10..40DD0. Entity registration, listener lifetime
 * and the concrete subclass remain owned by the enclosing runtime entity. */
typedef struct UCOrderClient {int32_t manager,pending;uint8_t advertise;} UCOrderClient;
typedef struct UCOrderClientServices {
 void *owner;
 int (*alive)(void *,int32_t,uint8_t *);
 int (*position)(void *,int32_t,float[3],uint8_t *);
 int (*accept)(void *,int32_t,const float[3],uint8_t *);
 int (*attached)(void *);
 int (*choose)(void *,int32_t);
 int (*completed)(void *);
 int (*send)(void *,int32_t,uint32_t,const void *,uint32_t,int32_t);
 int (*broadcast)(void *,uint32_t,const void *,uint32_t,int32_t);
 int (*remove)(void *);
 int (*retire)(void *);
} UCOrderClientServices;
void uc_order_client_init(UCOrderClient *);
/* Completing callbacks run synchronously and may mutate this same state.
 * Missing service or callback failure stops with committed failure2; no retry.
 * Unknown event returns1 before effects. Source payloads are int32 except the
 * one-byte acceptance response129; sender carries the real generation ID. */
int uc_order_client_event(UCOrderClient *,const UCOrderClientServices *,uint32_t event,const void *payload,int32_t sender);
int uc_order_client_update(UCOrderClient *,const UCOrderClientServices *,int32_t id,int32_t parent);
int uc_order_client_complete(const UCOrderClient *,const UCOrderClientServices *,int32_t id,int32_t command);
int uc_order_client_remove(const UCOrderClient *,const UCOrderClientServices *,int32_t id);
#ifdef __cplusplus
}
#endif
#endif

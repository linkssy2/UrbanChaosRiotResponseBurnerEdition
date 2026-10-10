#ifndef UC_FADE_H
#define UC_FADE_H
#include "entity_events.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCFade {
    uint32_t mode,white;
    float alpha,rate;
    uint8_t *completion;
    int32_t completion_owner;
    uint8_t held;
} UCFade;
typedef struct UCFadeServices {
    const UCEntityEventRegistry *registry;
    void *owner;
    int (*push_white)(void *); /*515114 deque push1,5E020*/
    int (*pop_white)(void *);  /*515114 deque pop,5D9E0*/
} UCFadeServices;
/* Category1 fade object: whole223180/222B50/222C80. Its manager map, drawing,
 * viewport category0, and global deque lifetime are caller-owned. Fresh state
 * is {mode=2,alpha=rate=0,completion_owner=-1}. Borrowed completion storage must
 * belong to the matching live registry generation and survive these calls.
 * Services are synchronous/nonreentrant. 0 success,1 rejected before effects,
 * 2 committed service failure: retire the owner, never retry its operation.
 * Strict crossing completes a fade; equality does not. White fade-up starts
 * from1.1 when alpha>=1, preserving the original extra fully opaque interval. */
int uc_fade_request(UCFade *,const UCFadeServices *,uint32_t mode,float seconds,
    uint8_t *completion,int32_t owner,uint32_t white);
int uc_fade_step(UCFade *,const UCFadeServices *,float seconds);
int uc_fade_close(UCFade *,const UCFadeServices *);
#ifdef __cplusplus
}
#endif
#endif

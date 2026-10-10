#ifndef UC_BULLET_OBSERVERS_H
#define UC_BULLET_OBSERVERS_H
#include "bullet_scenery.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCBulletObservers UCBulletObservers;
typedef struct UCBulletObserver {
    void *owner;
    int (*segment)(void *,const UCBulletSegment *);
} UCBulletObserver;
/* Actual52FD0 empty owner /531A0 unique pointer registration /53430 removal /
 * 53050 clear /53080 ascending unsigned-pointer broadcast. Native pointer
 * storage replaces the original tree; descriptor identity IS its ordering key.
 * Capacity is preadmitted host storage,1..65536; no source OOM equivalence.
 * Owner owns only its array; registered descriptors and targets are borrowed.
 * Callbacks must complete actual receiver+4 work, retain owner/descriptors and
 * immutable membership/packet through dispatch. World effects are allowed;
 * descriptor mutation, reentry and removing the current source iterator are not.
 * No fabricated successful receiver is installed; an actually empty constructed
 * set takes the genuine quiet53080 branch without reading the packet.
 * Return0 complete,1 pre-effect invalid/full/busy,2 partial receiver failure.
 * Failure2 is not retryable; previous receiver effects persist. Close/clear do
 * not destroy borrowed targets. Native allocation/free layout is not guest ABI. */
int uc_bullet_observers_create(uint32_t capacity,UCBulletObservers **);
int uc_bullet_observers_add(UCBulletObservers *,const UCBulletObserver *,uint8_t *inserted);
int uc_bullet_observers_remove(UCBulletObservers *,const UCBulletObserver *,uint8_t *removed);
int uc_bullet_observers_clear(UCBulletObservers *);
int uc_bullet_observers_dispatch(UCBulletObservers *,const UCBulletSegment *);
int uc_bullet_observers_close(UCBulletObservers *);
const UCBulletObserver *const *uc_bullet_observers_view(const UCBulletObservers *,uint32_t *count);
#ifdef __cplusplus
}
#endif
#endif

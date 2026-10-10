#include "bullet_history.h"
#include <fenv.h>
#include <float.h>
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
int uc_bullet_history_init(UCBulletHistory *h) {
    if(!h)return 1;
    h->player_shot_age=FLT_MAX;h->count=0;
    for(unsigned i=0;i<200;++i) {
        h->entries[i].owner_id=UINT32_MAX;
        memset(h->entries[i].from,0,sizeof h->entries[i].from);
        memset(h->entries[i].to,0,sizeof h->entries[i].to);
    }
    return 0;
}
int uc_bullet_history_append(UCBulletHistory *h,const UCBulletHistoryEntry *e,
                            const uint32_t *player) {
    if(!h||!e||h->count>200)return 1;
    if(h->count<200)memcpy(&h->entries[h->count++],e,sizeof *e);
    if(player&&*player==e->owner_id)h->player_shot_age=0;
    return 0;
}
int uc_bullet_history_advance(UCBulletHistory *h,float delta) {
    float next;
    if(!h||h->count>200||!isfinite(h->player_shot_age)||!isfinite(delta)
       ||(_mm_getcsr()&0xE040u)||fegetround()!=FE_TONEAREST)return 1;
    next=(float)((double)delta+h->player_shot_age);
    if(!isfinite(next))return 1;
    h->count=0;h->player_shot_age=next;return 0;
}
